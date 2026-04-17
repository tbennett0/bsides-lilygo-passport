<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [CyberPassport Participant Handout](#cyberpassport-participant-handout)
  - [Lab 04: Pull the Screen: VNC Remote View Attack](#lab-04-pull-the-screen-vnc-remote-view-attack)
  - [Scenario](#scenario)
  - [Your Goal](#your-goal)
  - [What You Can Use](#what-you-can-use)
  - [Rules](#rules)
  - [Starting Point](#starting-point)
  - [Quick Concept: How Did This Happen?](#quick-concept-how-did-this-happen)
  - [Checkpoints — Are You On the Right Track?](#checkpoints--are-you-on-the-right-track)
  - [Completion](#completion)
  - [Bonus: Think Like a Defender](#bonus-think-like-a-defender)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

# CyberPassport Participant Handout
## Lab 04: Pull the Screen: VNC Remote View Attack

**Category:** Red Team Ridge | **Difficulty:** Mixed | **Target Time:** 15–25 minutes

---

## Scenario

A USB device has been plugged into the victim workstation at this station. The moment it connected, it began injecting keystrokes at high speed — indistinguishable from a physical keyboard — to execute a series of silent commands on the Windows machine. Those commands installed a lightweight C# agent, compiled with .NET Native AOT, which immediately started a built-in VNC (Virtual Network Computing) server running on the victim's machine.

---

## Your Goal

Complete the lab by:

1. **Connecting to the LilyGo's Wi-Fi hotspot** from your device
2. **Opening the web dashboard** at the IP address shown on the station card
3. **Locating and opening the VNC panel** to view the victim's live desktop
4. **Reading and recording the flag** displayed on the victim's screen
5. **Showing the flag to a volunteer** and explaining one key step of how the attack worked

---

## What You Can Use

- The LilyGo T-Dongle S3 device (already plugged into the victim machine)
- Your own laptop, tablet, or phone with a Wi-Fi radio and browser
- The station materials (Wi-Fi password card, any printed reference guides)
- AI assistants, search engines, documentation, or online resources
- Help from a volunteer (no shame — progressive hints are available)

---

## Rules

- **DO NOT touch the victim workstation's keyboard, mouse, or monitor**
- **DO NOT unplug the LilyGo dongle** from the victim machine during the lab
- **DO NOT attempt to access systems outside the lab station network**
- **STAY WITHIN the provided lab environment** — do not try to connect to external hosts

---

## Starting Point

1. Do not touch the victim machine. The dongle is already plugged in and running.
2. On your device, open Wi-Fi settings and look for a hotspot that resembles a phone tethering connection.
3. Connect to it. When your device warns there is no internet, choose to stay connected anyway.
4. Open a browser and navigate to the web interface address shown on the station card.
5. Explore the interface — you are looking for something that shows you the victim's screen.

---

## Quick Concept: How Did This Happen?

1. The LilyGo enumerates as a USB keyboard and injects keystrokes at machine speed to install the agent
2. The agent (dns_checker.exe) is a .NET AOT-compiled native executable — no runtime required, no managed headers, cannot be obfuscated post-compile
3. It copies itself to C:\Users\Public\Documents\.sys_update\ and creates a scheduled task for reboot persistence
4. The agent starts a built-in VNC server and connects back to the dongle over USB-serial only — no network socket, no outbound connection visible to monitoring tools
5. The dongle bridges that USB-serial VNC stream to its own Wi-Fi hotspot
6. You view the victim's live desktop at http://4.3.2.1:8080 — your browser never talks directly to the victim machine

---

## Checkpoints — Are You On the Right Track?

You're making progress if you've checked these:

- [ ] You found and successfully joined the LilyGo's Wi-Fi hotspot
- [ ] Your device shows as connected to the Wi-Fi (check Wi-Fi settings)
- [ ] You can reach and load the web dashboard in your browser
- [ ] You found a panel or section for VNC / Screen Viewer / Remote Desktop
- [ ] You see the victim's live desktop appearing in the VNC viewer (even if it's loading)
- [ ] You can clearly read text or content on the victim's screen
- [ ] You've identified and written down the flag

---

## Completion

To finish the station, show a volunteer:

- **The flag or completion code** you found on the victim screen
- A brief explanation of one step in how the attack worked

**Need a hint? Ask a volunteer.** We have three progressive hints — no judgment for using them.

---

## Bonus: Think Like a Defender

After completing the lab, consider:

- **USB Restrictions:** What could an organization do to prevent rogue USB devices from injecting keystrokes on unlocked machines? (Hint: BIOS settings, Windows policies, USB driver whitelisting)
- **Network Visibility:** Why is USB-serial exfiltration harder to detect than a traditional network-based backdoor? (Hint: network monitoring tools only see network traffic, not USB serial)
- **Behavioral Detection:** If you were managing this victim machine, what would you look for in Task Scheduler, Event Viewer, or Process Monitor to spot this attack after the fact? (Hint: unexpected scheduled tasks, unsigned executables in temp directories, USB device connections)
- **Persistence:** The agent created a scheduled task. How long would it survive if the victim rebooted the machine?

---


*This is a contained training simulation. All devices, the agent, and the flag exist only within this station. For educational use inside the station environment only.*

**Created:** April 15, 2026  
**Last Updated:** April 15, 2026
