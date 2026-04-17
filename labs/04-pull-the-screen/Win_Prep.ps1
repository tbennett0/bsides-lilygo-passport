# Win_Prep.ps1
# Purpose: Compile Agent OR Reset Victim Station (or both)
# Usage: 
#   .\Win_Prep.ps1                 : Resets the station, then asks if you want to Setup (Compile).
#   .\Win_Prep.ps1 -Action Reset   : Only resets the station.
#   .\Win_Prep.ps1 -Action Setup   : Only compiles the agent (assumes drive is ready).
#   .\Win_Prep.ps1 -Install        : Copies self to System32 and adds to PATH for easy access.

param(
    [ValidateSet("Reset", "Setup", "Install")]
    [string]$Action = "Auto",
    [string]$RepoOwner = "i-am-shodan",
    [string]$RepoName = "USBArmyKnife"
)

# --- Colors ---
$Red = "`e[31m"
$Green = "`e[32m"
$Yellow = "`e[33m"
$Blue = "`e[34m"
$Reset = "`e[0m"

function Write-Step { Write-Host "$Blue[STEP]$Reset `n$args`n" }
function Write-Success { Write-Host "$Green[OK]$Reset `n$args`n" }
function Write-Warn { Write-Host "$Yellow[WARN]$Reset `n$args`n" }
function Write-ErrorMsg { Write-Host "$Red[ERR]$Reset `n$args`n"; exit 1 }

# --- Helper: Check Admin Rights ---
function Test-Admin {
    $currentUser = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    return $currentUser.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# --- Action: Reset Station ---
function Do-Reset {
    Write-Step "Starting Station Reset..."
    
    if (-not (Test-Admin)) {
        Write-ErrorMsg "This script must be run as Administrator to reset the station. Right-click PowerShell and select 'Run as Administrator'."
    }

    $TargetDir = "C:\Users\Public\Documents\.sys_update"
    $TaskNames = @("SystemUpdateService", "Security Script")
    $AgentExe = "dns_checker.exe"

    # 1. Stop Processes
    Write-Step "Stopping agent processes ($AgentExe)..."
    $Processes = Get-Process -Name $AgentExe -ErrorAction SilentlyContinue
    if ($Processes) {
        foreach ($proc in $Processes) { Stop-Process -Id $proc.Id -Force }
        Write-Success "Agent processes stopped."
    } else {
        Write-Step "No running agent processes found."
    }

    # 2. Remove Directory
    Write-Step "Removing attack directory: $TargetDir"
    if (Test-Path $TargetDir) {
        Remove-Item -Path $TargetDir -Recurse -Force
        Write-Success "Directory removed."
    } else {
        Write-Step "Directory not found (already clean)."
    }

    # 3. Remove Tasks
    foreach ($tName in $TaskNames) {
        Write-Step "Checking for task: $tName..."
        $Task = Get-ScheduledTask -TaskName $tName -ErrorAction SilentlyContinue
        if ($Task) {
            Unregister-ScheduledTask -TaskName $tName -Confirm:$false
            Write-Success "Task '$tName' removed."
        } else {
            Write-Step "Task '$tName' not found."
        }
    }

    # 4. Cleanup
    Write-Step "Clearing clipboard..."
    Set-Clipboard -Value "$null"
    Write-Step "Flushing DNS cache..."
    ipconfig /flushdns | Out-Null

    Write-Success "Station reset complete. Ready for next user."
    Write-Host "Please unplug and replug the LilyGo dongle."
}

# --- Action: Setup (Compile) ---
function Do-Setup {
    Write-Step "Starting Agent Compilation..."
    
    # 1. Find SD Card
    Write-Step "Scanning for drive labeled 'SETUPDISK'..."
    $SdDrive = Get-Volume | Where-Object { $_.FileSystemLabel -eq "SETUPDISK" }
    
    if (-not $SdDrive) {
        Write-ErrorMsg "Drive 'SETUPDISK' not found! Please insert the USB/SD card from the Mac (formatted by mac-setup.sh)."
    }
    
    $SdPath = $SdDrive.DriveLetter + ":"
    $StagingPath = "$SdPath\_STAGING"
    
    if (-not (Test-Path $StagingPath)) {
        Write-ErrorMsg "Staging folder not found on drive ($StagingPath). Did you run 'mac-setup.sh setup' on the Mac first?"
    }
    
    Write-Success "Found drive at: $SdPath"
    
    # 2. Prerequisites
    Write-Step "Checking prerequisites..."
    if ($PSVersionTable.PSVersion.Major -lt 7) {
        Write-Warn "PowerShell 7+ required. Installing via winget..."
        winget install --id Microsoft.PowerShell --source winget --silent
        Write-Success "PowerShell 7 installed. Please restart this script in the new PowerShell 7 window."
        exit 0
    }
    
    if (-not (Get-Command "gh" -ErrorAction SilentlyContinue)) {
        Write-Warn "GitHub CLI not found. Installing..."
        winget install --id GitHub.cli --source winget --silent
    }
    
    if (-not (Get-Command "git" -ErrorAction SilentlyContinue)) {
        Write-Warn "Git CLI not found. Installing..."
        winget install --id Git.Git --source winget --silent
    }
    
    $dotnetVersion = $null
    try { $dotnetVersion = dotnet --version 2>$null } catch {}
    $needsInstall = $false
    if (-not $dotnetVersion) { 
        $needsInstall = $true 
        Write-Warn ".NET SDK not found."
    } else {
        $major = [int]($dotnetVersion.Split('.')[0])
        if ($major -lt 8) { 
            $needsInstall = $true 
            Write-Warn ".NET SDK version $dotnetVersion detected. Version 8.0+ is required."
        }
    }
    
    if ($needsInstall) {
        Write-Step "Installing .NET 8.0 SDK..."
        $url = "https://dot.net/v1/dotnet-install.ps1"
        $path = ".\dotnet-install.ps1"
        
        try {
            Invoke-WebRequest -Uri $url -OutFile $path
            & .\dotnet-install.ps1 -Channel 8.0 -InstallDir $env:LOCALAPPDATA\dotnet -NoPath
            $env:Path = "$env:LOCALAPPDATA\dotnet;" + $env:Path
            Remove-Item $path
            Write-Success ".NET 8.0 SDK installed."
        } catch {
            Write-ErrorMsg "Failed to install .NET SDK. Install manually from: https://dotnet.microsoft.com/download"
        }
    }
    
    # Check for C++ Build Tools
    $VcVarsPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat"
    if (-not (Test-Path $VcVarsPath)) {
        Write-Warn "C++ Build Tools (vcvarsall.bat) not found at standard path."
        Write-Step "Attempting to install Microsoft Visual Studio 2022 Build Tools..."
        winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools;includeRecommended"
        
        # Wait for installation to finish (winget --wait usually handles this, but good to be safe)
        Write-Success "Installation initiated. This will take a few minutes, so please wait and then restart this script."
        exit 0
    } else {
        Write-Success "C++ Build Tools verified (vcvarsall.bat found)."
    }

    # 3. Build
    Write-Step "Cloning repository..."
    $CloneDir = "$env:TEMP\USBArmyKnifeClone"
    if (Test-Path $CloneDir) { 
        Remove-Item $CloneDir -Recurse -Force -ErrorAction SilentlyContinue 
    }
    
    try {
        git clone "https://github.com/$RepoOwner/$RepoName.git" $CloneDir | Out-Null
        Write-Success "Repository cloned."
    } catch {
        Write-ErrorMsg "Failed to clone repository. Check your internet connection."
    }
    
    Write-Step "Compiling with Native AOT..."
    Push-Location "$CloneDir\tools\Agent"
    
    Write-Step "Restoring dependencies..."
    dotnet restore | Out-Null
    
    Write-Step "Publishing as Native AOT Executable..."
    $buildOut = dotnet publish -r win-x64 --self-contained true -c Release /p:OutputType=Exe /p:PublishAot=true 2>&1
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host $buildOut
        Write-ErrorMsg "Build failed. Check output above."
    }
    
    Pop-Location
    
    $PublishPath = "$CloneDir\tools\Agent\bin\Release\net8.0-windows\win-x64\publish"
    
    if (-not (Test-Path $PublishPath)) { 
        Write-ErrorMsg "Publish path not found: $PublishPath" 
    }
    
    # 4. Copy and Rename Files
    Write-Step "Copying files to drive ($SdPath)..."
    Get-ChildItem $PublishPath | Copy-Item -Destination $StagingPath -Force
    
    # Rename ONLY the Executable
    $Exe = Get-ChildItem $StagingPath -Filter "*.exe" | Where-Object { $_.Name -ne "dns_checker.exe" } | Select-Object -First 1
    if ($Exe) {
        Move-Item $Exe.FullName -Destination "$StagingPath\dns_checker.exe" -Force
        Write-Success "Renamed executable to dns_checker.exe."
    } else {
        Write-ErrorMsg "No .exe found to rename."
    }
    
    # DO NOT RENAME DLLs. Leave them as turbojpeg.dll, vcruntime140.dll, etc.
    Write-Step "Leaving DLLs with original names (required for AOT)."
    
    # Create Marker
    New-Item -ItemType File -Path "$StagingPath\.WINDOWS_DONE" -Force | Out-Null
    
    # Cleanup
    Remove-Item $CloneDir -Recurse -Force -ErrorAction SilentlyContinue

    # Eject Setup Disk
    $disk = Get-Volume -FileSystemLabel "SETUPDISK" | Select-Object -First 1
    if ($disk) {
        $disk.DriveLetter | ForEach-Object { Remove-Item -Path "$($_):" -Force -ErrorAction SilentlyContinue }
        # Alternative method using MountVol (more reliable for ejection)
        MountVol "$($disk.DriveLetter):" /D
        Write-Host "Ejected 'SETUPDISK' successfully."
    } else {
        Write-Host "Disk 'SETUPDISK' not found."
    }
    
    Write-Success "Compilation complete. Files written to drive."
    Write-Host ""
    Write-Host "------------------------------------------------------------"
    Write-Host "Next Steps:"
    Write-Host "  1. Remove SETUPDISK from this computer"
    Write-Host "  2. Insert it back into the Mac."
    Write-Host "  3. Run: ./mac-setup.sh resume"
    Write-Host "------------------------------------------------------------"
    Write-Host ""
}

# --- Action: Install (Add to PATH) ---
function Do-Install {
    Write-Step "Installing Win_Setup to System PATH..."
    
    if (-not (Test-Admin)) {
        Write-ErrorMsg "This script must be run as Administrator to install to System PATH."
    }
    
    $ScriptPath = $MyInvocation.MyCommand.Path
    $DestDir = "C:\Windows\System32\WinSetup"
    
    if (-not (Test-Path $DestDir)) {
        New-Item -ItemType Directory -Path $DestDir -Force | Out-Null
        Write-Host "Created staging folder: $StagingPath"
    }

    Copy-Item -Path $ScriptPath -Destination "$DestDir\Win_Prep.ps1" -Force
    Write-Success "Copied script to $DestDir"
    
    # Add to PATH
    $CurrentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
    if ($CurrentPath -notlike "*$DestDir*") {
        [Environment]::SetEnvironmentVariable("Path", "$CurrentPath;$DestDir", "Machine")
        Write-Success "Added $DestDir to System PATH"
        Write-Host "You may need to restart PowerShell for changes to take effect."
    } else {
        Write-Success "Path already contains $DestDir"
    }
    
    Write-Success "Installation complete. You can now run 'Win_Setup' from any PowerShell window."
}

# --- Main Logic ---

if ($Action -eq "Install") {
    Do-Install
    exit 0
}

if ($Action -eq "Reset") {
    Do-Reset
    exit 0
}

if ($Action -eq "Setup") {
    Do-Setup
    exit 0
}

# Default: Auto Mode (Reset then Ask)
Write-Step "Running in Auto Mode: Resetting Station First..."
Do-Reset

Write-Host ""
$Response = Read-Host "Station reset complete. Would you like to proceed with Full Setup (Compile)? (Y/N)"
if ($Response -eq "Y" -or $Response -eq "y") {
    Do-Setup
} else {
    Write-Host "Full Setup skipped. Script finished."
}