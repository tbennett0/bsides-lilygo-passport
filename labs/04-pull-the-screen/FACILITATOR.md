<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [CyberPassport Builder + Volunteer Guide](#cyberpassport-builder--volunteer-guide)
  - [Lab 04: Pull the Screen: VNC Remote View Attack](#lab-04-pull-the-screen-vnc-remote-view-attack)
  - [Core Lesson](#core-lesson)
  - [Participant Outcome](#participant-outcome)
  - [Scenario Summary](#scenario-summary)
  - [Completion Condition](#completion-condition)
  - [Intended Solve Path](#intended-solve-path)
  - [Lab Materials](#lab-materials)
  - [Build Workflow (Pre-Event Preparation)](#build-workflow-pre-event-preparation)
    - [Overview of the Build Pipeline](#overview-of-the-build-pipeline)
    - [Prerequisites](#prerequisites)
      - [macOS Machine](#macos-machine)
      - [Windows Build/Prep Machine](#windows-buildprep-machine)
      - [Victim Windows Machine](#victim-windows-machine)
  - [Lab Preparation Workflow (Pre-Attack Preparation)](#lab-preparation-workflow-pre-attack-preparation)
    - [Step 1 — Disable Windows Defender and Virus Protection on the victim machine](#step-1--disable-windows-defender-and-virus-protection-on-the-victim-machine)
    - [Step 2 — Prepare the Micro-SD card (do once, verify before each event)](#step-2--prepare-the-micro-sd-card-do-once-verify-before-each-event)
    - [Step 3 — Set the flag on the victim desktop](#step-3--set-the-flag-on-the-victim-desktop)
    - [Step 4 — Verify the full chain works before opening](#step-4--verify-the-full-chain-works-before-opening)
    - [Resetting Between Participants](#resetting-between-participants)
  - [Quick Verification Checklist](#quick-verification-checklist)
  - [Common Failure Points](#common-failure-points)
    - [Hint Ladder (Progressive Disclosure)](#hint-ladder-progressive-disclosure)
    - [Tracking & Logging](#tracking--logging)
  - [Volunteer Notes](#volunteer-notes)
  - [Stretch Goal](#stretch-goal)
    - [Extended Engagement (Stretch Questions)](#extended-engagement-stretch-questions)
  - [Resources](#resources)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# CyberPassport Builder + Volunteer Guide
## Lab 04: Pull the Screen: VNC Remote View Attack

**Category:** Red Team Ridge | **Difficulty:** Mixed | **Target Time:** 15–25 minutes

---

## Core Lesson

Demonstrate how a rogue USB device can silently exfiltrate a live desktop view via VNC (hopefully) without installing traditional malware.

---

## Participant Outcome

By the end of the lab, participants should be able to:

- Flash the LilyGo dongle with the USBArmyKnife firmware
- Observe a live remote desktop captured and streamed by the LilyGo USB dongle
- Explain what VNC is and why unprotected VNC access is dangerous

---

## Scenario Summary

A rogue USB device can silently install a lightweight agent on a Windows machine, which starts a built-in VNC server communicating only over USB serial — never over the network. The LilyGo T-Dongle S3 running USBArmyKnife relays that screen data over its own Wi-Fi hotspot, letting an attacker view the victim's desktop from a phone or laptop without touching a keyboard or installing conventional software.

---

## Completion Condition

A participant completes the station when they:

- Successfully connect to the LilyGo web interface from their device
- View the victim's live desktop via the built-in VNC relay
- Read and submit the correct flag visible on the victim screen

---

## Intended Solve Path

1. Participant plugs the LilyGo dongle into the victim Windows machine (and not pushing boot button)
2. Dongle auto-runs `autorun.ds`: injects keystrokes, installs agent, builds `agentInstalled` onto root of Micro-SD card
3. Agent starts its built-in VNC server and communicates back to dongle via USB serial
4. Participant connects their device to the **iPhone14** Wi-Fi (password: `password`)
5. Participant navigates to `http://4.3.2.1:8080` and opens the VNC panel
6. Participant reads the flag displayed on the victim desktop and submits it

> **Architecture note:** The agent's VNC server communicates ONLY over USB serial — never over Wi-Fi or a network socket. No VNC software needs to be pre-installed on the victim. The dongle bridges USB serial to its own Wi-Fi hotspot, making the screen available at `http://4.3.2.1:8080`.

---

## Lab Materials

- 1 Windows victim workstation (Windows 10 or 11, 64-bit)
- 1 LilyGo T-Dongle S3 flashed with USBArmyKnife firmware
- 1 micro-SD card formatted FAT32 containing: `autorun.ds` and `agent.img`
- `agent.img` contents: `dns_checker.exe` (renamed from `Agent.exe`)[^name], `in1.bat`, `turbojpeg.dll`, `vcruntime140.dll`
- 1 participant device with a browser (phone or laptop)
- LilyGo dongle's iPhone14 hotspot
- Flag displayed on the victim desktop (wallpaper, open document, or slideshow)

[^name]: > **Obfuscation note:** [Changing the name](https://github.com/t3l3machus/PowerShell-Obfuscation-Bible#Rename-Objects) of attack files helps avoid being caught. Real-life hackers tend to use multiple techniques.

---

## Build Workflow (Pre-Event Preparation)

> **CRITICAL:** Windows Defender and Virus Protection MUST be disabled on the victim machine before the event opens. The agent will be quarantined silently if Defender is running — the attack will appear to work but the agent will never connect.

### Overview of the Build Pipeline

The complete build spans possibly three machines:

| Machine | Tool | Task |
|---------|------|------|
| **macOS** (Build Lead) | `./mac-setup.sh` | Firmware download, image preparation, final Micro-SD card formatting |
| **Windows[^build]** (Compiler) | `Win_Prep.ps1` | Agent.exe compilation with Native AOT (later renamed to `dns_checker.exe`) |
| **Windows** (Victim) | N/A | Defender disable, flag setup, verification |

[^build]: The Windows machine used to compile the attack payload can also be prepped to be the victim machine. Due to timing issues in the VM System and the physical LilyGo USB dongle, the Victim machine cannot be a VM (except if the attack files are manually placed in the right folder before inserting the LilyGo).

### Prerequisites

#### macOS Machine

Ensure you have:

- `brew` ([Installation Instructions](https://docs.brew.sh/Installation))

```bash
# Required tools
xcode-select --install                # Xcode Command Line Tools
brew install python3 gh git           # Python, GitHub CLI, Git
gh auth login                         # Authenticate with GitHub (required!)
pip3 install esptool                  # ESP32 flashing tool
```

#### Windows Build/Prep Machine

Ensure you have:

```powershell

# Many installs (including Win_Prep.sh) require Administrator access, so open PowerShell As Administrator

# Win_Prep installs these (better to have pre-installed to avoid delays)

# PowerShell 7
$PSVersionTable.PSVersion 		# Check PowerShell version
winget install --id Microsoft.PowerShell --source winget

# .NET 8.0 SDK
winget install Microsoft.DotNet.SDK.8

# Microsoft Visual Studio 2022 C++ Build Tools
winget install -e --id Microsoft.VisualStudio.2022.BuildTools --override "--passive --wait --add Microsoft.VisualStudio.Workload.VCTools;includeRecommended"

# GitHub CLI
winget install --id GitHub.cli --source winget

# Git CLI
winget install --id Git.Git --source winget
```

> **Note:** Installation could take up to 30 minutes total. Plan accordingly.

#### Victim Windows Machine

Prepare a Windows 10 or 11 (64-bit) machine with:
- Administrator access
- Unrestricted USB ports
- Defender and Virus Protection disabled (see below)
- A USB-A port for the LilyGo

## Lab Preparation Workflow (Pre-Attack Preparation)

### Step 1 — Disable Windows Defender and Virus Protection on the victim machine

1. Settings → Privacy & Security → Windows Security → Virus & threat protection
2. Click **Manage settings**
3. Turn **OFF** Real-time protection
4. Turn **OFF** Tamper Protection (required before other settings can be changed)
5. If a third-party AV is installed, open it and pause or disable its shield

### Step 2 — Prepare the Micro-SD card (do once, verify before each event)

The Micro-SD card root must contain exactly these files:

| File | Purpose |
|------|---------|
| `autorun.ds` | DuckyScript payload — runs automatically on plug-in |
| `agent.img` | FAT32 disk image containing below files |
| *agent.img*/`dns_checker.exe` | executable that performs the attack |
| *agent.img*/`in1.bat` | Command-line script that starts after `autorun.ds` |
| *agent.img*/`turbojpeg.dll` | library file that helps perform the attack |
| *agent.img*/ `vcruntime140.dll` | library file that helps perform the attack |

### Step 3 — Set the flag on the victim desktop

1. Create a text file, wallpaper, or open document on the victim desktop showing a clearly readable flag, such as saving [Nancy Reagan "Just Say No"](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Ftse4.mm.bing.net%2Fth%2Fid%2FOIP.Pcnux0Hyo9o9HbnX5uP6HQHaD4%3Fpid%3DApi&f=1&ipt=57ad1dda7136e289e90b9261862645943dfd18078a13d1baddc506244a37f3f5&ipo=images) as the desktop's background image.
2. The flag must be visible without clicking anything — the VNC stream shows the desktop as-is.

### Step 4 — Verify the full chain works before opening

1. Delete the `agentInstalled` file from the Micro-SD card root if it exists (left from a previous run).
2. Plug the dongle into the victim machine. Watch the dongle screen for LOG messages.
3. Connect your device to **iPhone14** (password: `password`). Click **Stay Connected** when prompted about no internet.
4. Open `http://4.3.2.1:8080` in your browser. Navigate to the VNC panel.
5. Confirm the victim desktop is visible and the flag is readable.

---

### Resetting Between Participants

**Time:** 2–3 minutes per participant

1. **Unplug LilyGo** from victim machine
2. **Delete `agentInstalled` marker** from SD card root:
   ```bash
   # On macOS, if card is mounted as USBAGENT:
   rm /Volumes/USBAGENT/agentInstalled 2>/dev/null || true
   ```
   **OR** swap a pre-wiped SD card (faster under time pressure)
3. **Reinsert SD card** into LilyGo
4. **Plug LilyGo** back into victim machine
5. **Wait 5–10 seconds** for agent to auto-install
6. **Clear participant device's browser session:**
   - Close the VNC tab or hard-refresh (Ctrl+Shift+R)
   - Clear cookies/cache if needed
7. **Quick verification:**
   - Open `http://4.3.2.1:8080` again
   - Confirm VNC shows Connected and victim desktop is visible
   - Confirm flag is still readable
   
---

## Quick Verification Checklist

Before the station opens, confirm all of the following:

- [ ] Windows Defender real-time protection is OFF on victim machine
- [ ] Tamper Protection is OFF
- [ ] Anti-Virus Protection is OFF
- [ ] Micro-SD card has `autorun.ds` and `agent.img` at root (no `agentInstalled` file)
- [ ] Dongle screen shows **Device is running** (not a crash dump)
- [ ] iPhone14 Wi-Fi is visible and joinable (password: `password`)
- [ ] `http://4.3.2.1:8080` loads the dashboard
- [ ] VNC panel shows **Connected** and victim desktop is visible
- [ ] Flag is clearly readable in the VNC stream

---

## Common Failure Points

| Issue | Fix |
|:-------|:-----|
| Agent never shows Connected | Almost certainly Windows Defender. Check Protection History in Windows Security for quarantined `dns_checker.exe`. Disable Defender and delete the `agentInstalled` file before retrying. As last resort, deploy the "manual fix" from the main setup file[^manual]. |
| Dongle screen shows crash dump (hex addresses) | Micro-SD card filesystem issue. Format card as plain FAT32, copy files directly — do not use `dd` to write a raw image to the card itself. |
| Dongle shows: `valid Micro-SD not found` | Micro-SD card not detected. Reseat card. Check card is FAT32. Try a different card. |
| `cmd` window flashes but agent never connects | Defender quarantined agent silently, or `in1.bat` ran but agent crashed. Check Defender. Also delete the `agentInstalled` file from Micro-SD and retry. |
| VNC shows Connecting but never Connected | Refresh the page — the noVNC client sometimes needs a second load. Known upstream issue. |
| Can't reach `http://4.3.2.1:8080` | Device not connected to iPhone14 hotspot, or OS silently switched back to regular Wi-Fi. Reconnect and confirm staying on that network. |
| `autorun.ds` says payload already running | Normal — the script auto-runs on plug-in. Do not run it again from the UI. |
| Drive letter not found (D–H) | The script searches drives D through H. If the `agent.img` virtual drive lands on a different letter, the install fails. Unplug other USB storage devices. |
| Flag not visible | The Victim machine must have a slideshow or looped video showing the flag. |

[^manual]: Open the setup document under [README.md "Victim Attack Workflow"](/README.md#manual-attack-fix) for the full process of how the attack should work, and the manual fix for if it doesn't work

---

### Hint Ladder (Progressive Disclosure)

When a participant asks for help, give hints in order. Do not skip steps.

**Hint 1 — Wi-Fi Connection**

> "The LilyGo device is already doing the hard work of installing the agent. Your job is to find its Wi-Fi hotspot and connect to it. Check your Wi-Fi settings for a network that looks like a phone tethering connection."

**Hint 2 — Web Dashboard**

> "Once you're on the Wi-Fi, open a browser and navigate to the IP address on your station card. You should see a dashboard with several panels. Look for something related to screen viewing or VNC."

**Hint 3 — VNC Panel**

> "The dashboard has a VNC panel (might be a button, tab, or section). Click on it. You might see 'Connecting' for a few seconds — that's normal. If it stays 'Connecting,' try refreshing the page."

**Hint 4 — Reading the Flag**

> "You should now see the victim's desktop in your browser. The flag is displayed somewhere on that desktop. Look at the wallpaper, open windows, taskbar, or anywhere else text might appear. Read it carefully."

**Hint 5 — Conceptual Help (Optional)**

> "The LilyGo device is acting as a bridge. When it plugged in, it injected keystrokes (like a keyboard) to install an agent on the victim machine. That agent started a screen-sharing server that only talks back to the dongle over USB — not Wi-Fi. The dongle then makes that screen available over its own Wi-Fi hotspot so you can view it from your device."

---

### Tracking & Logging

Have the **Scribe** maintain a log:

| Participant | Start Time | Flag | Correct? | Time (min) | Notes |
|-------------|-----------|------|----------|-----------|-------|
| Alice      | 14:00     | FLAG{...} | ✓ | 12 | Needed Hint 2 |
| Bob        | 14:15     | FLAG{...} | ✓ | 8  | Fast; asked about persistence |
| Carol      | 14:25     | (incorrect) | ✗ | 18 | Misread flag; tried again, got it |

This helps identify common pain points and track overall success.

---

## Volunteer Notes

- Beginners may not know what VNC is. One-liner: "It lets you see another computer's screen over the network — the dongle creates that connection for you."
- The agent communicates over USB serial, not Wi-Fi. Defenders cannot detect this with network monitoring alone — that's a great discussion point.
- If the participant asks why the `cmd` window appeared, that's the HID injection — the dongle typed the install command as if it were a keyboard.
- Do not reveal the flag or point to where it is. Give hints 1 and 2 first.
- Keep participants within the station network. They should not attempt to connect to anything outside.

---

## Stretch Goal

Ask: "If you were defending this machine, how would you stop this attack?" Discussion points: USB port locks, USB device whitelisting via Group Policy, endpoint agents that detect HID injection patterns, disabling AutoRun, monitoring for unexpected scheduled tasks or new executables in user temp directories.

---

### Extended Engagement (Stretch Questions)

If a participant finishes quickly and wants to dig deeper:

1. **Agent Persistence:** "Why does the agent survive a reboot? What happens if the victim runs Task Scheduler?"
2. **Detection:** "How would an EDR (Endpoint Detection & Response) tool spot this attack?"
3. **Exfiltration:** "The agent is sending screen data over USB. What if it also exfiltrated files or keylogging data?"
4. **Defense:** "If you were defending this machine, what would you do? (USB port whitelist, BIOS password, hardware security key, etc.)"
5. **Hardware:** "The LilyGo is an ESP32-S3 microcontroller. Why is it so cheap and easy to turn into an attack tool?"

These discussions reinforce the learning and can be fascinating for experienced participants.

---

## Resources

- **USBArmyKnife Repository:** https://github.com/i-am-shodan/USBArmyKnife
- **LilyGo T-Dongle S3:** https://github.com/Xinyuan-LilyGO/T-Dongle-S3
- **DuckyScript Documentation:** https://github.com/hak5/duckyscript-vm
- **noVNC:** https://novnc.com/
- **.NET Native AOT:** https://learn.microsoft.com/en-us/dotnet/core/deploying/native-aot/
- **ESP32-S3 Datasheet:** https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf

---

*This is a contained training simulation. All devices, the agent, and the flag exist only within this station. For educational use inside the station environment only.*


**Created:** April 15, 2026  
**Last Updated:** April 15, 2026
