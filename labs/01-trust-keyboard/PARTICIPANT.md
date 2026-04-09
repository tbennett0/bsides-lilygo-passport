# Trust Me, I'm a Keyboard

**Category:** Blue Team Boulevard  
**Difficulty:** Mixed  
**Target Time:** 20 minutes

## Scenario

You are looking at a LILYGO T-Dongle-S3 programmed to act like a USB keyboard. That matters because operating systems usually trust keyboards immediately, with no "install driver?" prompt and no warning.

This lab gives you a deliberately broken firmware project. Your job is to fix it and then explain what that teaches us about USB trust.

## What You're Looking At

When the device is plugged in normally:

- The LCD should show `READY`
- The LED should turn green
- Nothing happens until you press the physical `BOOT` button

When the firmware is working correctly, pressing `BOOT` should:

1. Open a terminal with `Ctrl+Alt+T`
2. Type `cat ~/flag.txt`
3. Press Enter
4. Show the flag in the terminal

## Your Goal

Complete the lab by:

- Finding the firmware bug that makes the device type the wrong command
- Fixing it in `main/hid_keyboard.c`
- Rebuilding and flashing the device
- Verifying that the flag appears when you press `BOOT`

## Starting Point

From the repository root:

```bash
cd labs/01-trust-keyboard
```

Open:

```text
main/hid_keyboard.c
```

Look for the code that maps printable characters to keyboard output.

## Build And Flash

From `labs/01-trust-keyboard`, run:

```bash
idf.py build flash
```

If your station needs an explicit port, try:

```bash
idf.py -p /dev/ttyACM0 build flash
```

If flashing is not working, ask a volunteer for help.

## Verify Your Fix

After flashing:

1. Plug the dongle into the Lubuntu machine normally
2. Wait for `READY` on the screen
3. Press the physical `BOOT` button
4. Watch the terminal

You are done when the device types the correct command and the flag appears.

## Checkpoints

You are probably on the right track if:

- You can reproduce the broken typing before you change anything
- You find the character-to-keyboard mapping in `main/hid_keyboard.c`
- After flashing, the typed command matches `cat ~/flag.txt`

## Investigation Questions

After you fix it, be ready to discuss:

1. What does a USB keyboard actually send to the host?
2. Why did the operating system trust this device immediately?
3. What controls could make this kind of attack harder in the real world?

### Optional Stretch

If you finish early, explore this too:

4. Where on the host can you observe that a new USB keyboard appeared, and what is *not* visible by default?

## Completion

To finish the station, show a volunteer:

- The corrected device behavior
- The flag appearing in the terminal
- Your short explanation of what the lab demonstrates

**Need a hint? Ask a volunteer.**
