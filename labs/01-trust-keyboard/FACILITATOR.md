# Facilitator Guide — Trust Me, I'm a Keyboard

**Do not distribute this file to participants.**

## Overview

This lab uses a LILYGO T-Dongle-S3 running ESP-IDF firmware that enumerates as a USB HID keyboard. When it is plugged into a Lubuntu machine, the LCD shows `READY` and the LED turns green. The device does nothing until the participant presses the physical `BOOT` button.

When working correctly, the button press triggers this sequence:

1. Send `Ctrl+Alt+T`
2. Wait 1.5 seconds for a terminal to open
3. Type `cat ~/flag.txt`
4. Press Enter

Participants receive a deliberately broken firmware project. Their job is to find the bugs, fix, and rebuild with `idf.py build flash`, and confirm the flag appears on the host.

The lab should be somewhat friendly at the code level. The deeper learning goal is understanding why USB HID devices are trusted so quickly, what a keyboard really sends to the host, and what defenses are realistic.

## Learning Goals

By the end of the station, participants should be able to:

- Recognize that USB keyboards send HID scancodes and modifiers, not characters
- Explain why the host trusted the device immediately with no driver prompt
- Describe at least a few realistic defenses against HID keystroke injection

## Lab At A Glance

- Hardware: LILYGO T-Dongle-S3
- Host: Lubuntu / LXQt
- Working device identity: `Trust Demo Keyboard`
- State machine: `INIT -> READY -> TYPING -> DONE`
- LED colors:
  - `INIT`: blue
  - `READY`: green
  - `TYPING`: red
  - `DONE`: purple
- Working payload: `cat ~/flag.txt`
- Broken output participants will see: `cAy ~/flAg/yxy`
- Total time: about 20 minutes

## Pre-Event Checklist

Run these steps on each lab machine before the event.

### 1. Provision the machine

From the repository root:

```bash
./provision.sh
```

If you prefer to run the steps separately:

```bash
./setup-base.sh
./setup-labs.sh trust-keyboard
```

What this does:

- Installs ESP-IDF tooling and dependencies
- Adds udev access for Espressif USB devices
- Adds the user to `dialout` and `plugdev`
- Builds the lab once to warm the build cache
- Creates `~/flag.txt`

If user groups changed, log out and back in before flashing devices.

### 2. Confirm the flag file exists

The setup script should create:

```bash
~/flag.txt
```

Expected contents:

```text
FLAG{HID_TRUST_IS_BLIND}
```

### 3. Flash each device with the broken firmware

From the repository root:

```bash
cd labs/01-trust-keyboard
idf.py set-target esp32s3
idf.py build flash
```

If needed, specify the port explicitly:

```bash
idf.py -p /dev/ttyACM0 build flash
```
It may not be at this exact port, verify with ls /dev/tty*

### 4. Verify host behavior

On the Lubuntu machine:

- Confirm `Ctrl+Alt+T` opens a terminal manually
- Plug in the dongle normally
- Confirm the LCD shows `READY`
- Confirm the LED is green
- Press `BOOT`
- Confirm the broken firmware types `cAy ~/flAg/yxy`

This confirms the puzzle is still intact.

### 5. Verify USB enumeration

Use either of these checks:

```bash
lsusb
```

or:

```bash
dmesg -w
```

You should see the device enumerate as a HID keyboard. The product string is `Trust Demo Keyboard`, and host logs may also include the manufacturer string `BSides Lab`.

### 6. Quick station reset check

Before participants arrive, confirm:

- The repo opens at `labs/01-trust-keyboard`
- `main/hid_keyboard.c` is present and editable
- `idf.py` works in a terminal
- The device can be re-flashed from that machine
- Plugging in and pressing `BOOT` reproduces the broken output

## Timing Guide

| Phase | Target Time | Facilitator Notes |
|------|------:|---|
| Intro and setup | 1-2 min | Hand out the device, point participants at the lab directory, explain the goal |
| Puzzle | 2-5 min | Let them observe the broken typing, find the keymap issue, rebuild, and flash |
| Investigation and discussion | 5-8 min | Ask the three required questions after they solve it |
| Optional stretch | 2-4 min | Use the host-evidence question or extra prompts if the table is moving fast |
| Total | ~20 min | Adjust pacing based on table traffic |

## Intended Solve Path

1. Participant plugs the dongle into the Lubuntu host
2. They see `READY` on the LCD and a green LED
3. They press `BOOT` and observe the wrong command being typed
4. They inspect `main/hid_keyboard.c`
5. They find the `s_ascii_keymap[]` table
6. They correct the bad mappings
7. They run `idf.py build flash`
8. They test again and confirm the flag appears
9. They explain why the bug mattered and why the OS trusted the device

## Answer Key

The puzzle is in `main/hid_keyboard.c`, inside `s_ascii_keymap[]`.

There are **three intentional defects**:

1. The `.` and `/` mappings are swapped
2. Lowercase `a` incorrectly uses the Shift modifier
3. Lowercase `t` is mapped to `HID_KEY_Y`

That produces:

```text
cAy ~/flAg/yxy
```

instead of:

```text
cat ~/flag.txt
```

### Correct Fix

```diff
/* swap these two back */
- /* 0x2E '.'  */ { _N, HID_KEY_SLASH },
- /* 0x2F '/'  */ { _N, HID_KEY_PERIOD },
+ /* 0x2E '.'  */ { _N, HID_KEY_PERIOD },
+ /* 0x2F '/'  */ { _N, HID_KEY_SLASH },

/* lowercase 'a' should not use Shift */
- { _S, HID_KEY_A }, { _N, HID_KEY_B }, { _N, HID_KEY_C },
+ { _N, HID_KEY_A }, { _N, HID_KEY_B }, { _N, HID_KEY_C },

/* lowercase 't' should map to T, not Y */
- { _N, HID_KEY_S }, { _N, HID_KEY_Y }, { _N, HID_KEY_U },
+ { _N, HID_KEY_S }, { _N, HID_KEY_T }, { _N, HID_KEY_U },
```

### What "solved" looks like

After the fix and a successful flash:

- Plugging in the device shows `READY`
- Pressing `BOOT` opens a terminal
- The device types `cat ~/flag.txt`
- The flag appears in the terminal window

## Hint Ladder

Give hints verbally and only as needed. The goal is to keep momentum without removing the puzzle.

### Hint 1: Observation

Use after about 2 minutes if they have not narrowed down the problem.

> "Watch exactly what comes out when you press BOOT. Which characters are wrong, and which ones are still correct?"

### Hint 2: Location

Use if they are not sure where to start in the code.

> "The payload string itself is fine. Look at how the firmware turns characters into key presses."

### Hint 3: Mechanism

Use if they found the right file but do not understand what they are looking at.

> "USB keyboards do not send letters. They send keycodes and modifiers. There is a table in the code that maps printable characters to those values."

### Hint 4: Specific

Use as the last resort.

> "Check `s_ascii_keymap[]` in `main/hid_keyboard.c`. Compare the entries for the wrong characters against nearby entries and the intended command."

## Required Discussion Questions

These are the core assessment questions. A participant does not need perfect terminology, but they should be able to explain the mechanism, the trust model, and at least a few defensive ideas.

### 1. What did you fix, and why did it cause the wrong characters to appear?

- Good answer: The firmware had wrong entries in the key mapping table, so it sent the wrong keycodes or modifiers for some characters.
- Great answer: The device sends HID usage IDs and modifiers, not text. The host translates those into characters using its own keyboard layout, so a wrong mapping in firmware produces the wrong output on screen.

### 2. What class of USB device is this, and why did the OS trust it immediately?

- Good answer: It is a USB HID keyboard, so the OS used a built-in driver automatically.
- Great answer: HID is a standard, heavily trusted device class because people need keyboards and mice to use a system. The host accepts the device's claim and loads generic support without a driver-install prompt or user approval.

### 3. What would make this attack harder or stop it in a managed environment?

- Good answer: USB device control, allowlists, blocking unknown keyboards, and physical port restrictions.
- Great answer: Specific measures like USBGuard on Linux, enterprise device control policies, limiting new HID enrollment, port blockers, user awareness, and process controls on sensitive systems all help because this attack relies on the host accepting a new keyboard at all.

## Optional Stretch Question

Use this if a participant finishes early or you want to connect the demo to host-side visibility.

### 4. Where on the host can you observe that a new USB keyboard appeared, and what is not visible by default?

- Good answer: `dmesg -w`, `journalctl -k -f`, and `lsusb` can show that a new HID device appeared and identify it as a keyboard.
- Great answer: Those sources show device arrival, identity, and kernel/HID enumeration, but they usually do not show the actual keystrokes or the exact injected command. Defenders often detect the effects instead, such as a terminal launch, suspicious process behavior, shell history, or audit data if extra logging is enabled.

## Extra Facilitator Prompts

Use these only if the table is advanced or you want to extend the conversation. They are no longer required for completion.

- Different keyboard layout: Would the same firmware work on AZERTY, and why not?
- EDR: Would an endpoint tool reliably catch keystroke injection, or only its side effects?
- Identity spoofing: Could an attacker rename the device or spoof VID/PID values, and how much would that help?

## What To Do If Things Go Wrong

### Device does not enumerate

Check these in order:

1. Unplug and replug the dongle
2. Try a different USB port
3. Confirm the board is getting power and the display or LED comes on
4. Watch `dmesg -w` while plugging it in
5. Reflash it in download mode:

```bash
cd labs/01-trust-keyboard
idf.py -p /dev/ttyACM0 flash
```

6. If flashing fails with permissions, confirm the user logged out and back in after provisioning
7. If nothing appears at all, suspect a bad port, bad board, or host USB issue

### Wrong characters still appear after the participant "fixed" it

Common causes:

1. They edited the wrong file or wrong entries in `s_ascii_keymap[]`
2. The firmware built successfully but was not actually flashed to the device
3. The board was left in the old state and never retested after flashing
4. The host keyboard layout is not US QWERTY
5. A stale build artifact is being reused

Suggested recovery steps:

```bash
cd labs/01-trust-keyboard
rm -rf build
idf.py build flash
```

Then test again on a US-layout Lubuntu machine.

### Terminal does not open

Check:

1. Press `Ctrl+Alt+T` manually on the host
2. Confirm the desktop environment still uses that shortcut
3. Make sure the host has focus and is not on a locked screen or dialog
4. Increase `TERMINAL_OPEN_DELAY_MS` if the terminal opens too slowly
5. As a fallback for demos, open a terminal manually and disable the shortcut step in `menuconfig`, or temporarily explain that the keyboard still types into the focused window

### Flashing fails

Check:

1. `idf.py` is available in the shell
2. The port is correct, usually `/dev/ttyACM0`
3. The user has serial permissions
4. The device is in download mode when required

If needed, reopen the terminal or run:

```bash
source ~/.bashrc
```

## Volunteer Notes

- Beginners often look for the payload string first; redirect them toward the keymap
- Advanced participants may jump straight to USB trust and defenses; let them, but still have them prove the fix
- If a table is stuck, ask them to compare expected output with actual output character by character
- Keep the conversation grounded in defensive understanding, not "how to weaponize it"

## Reset Between Participants

1. Reflash the broken firmware if the previous participant solved it
2. Confirm `~/flag.txt` still exists
3. Plug in the device and verify it still types the broken command
4. Close extra terminal windows so the next participant starts clean
