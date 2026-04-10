# Firmware Reverse Engineering

**Category:** Intelligence Collection & Comms  
**Difficulty:** Intermediate  
**Target Time:** 15-20 minutes

## Scenario

You are looking at a LILYGO T-Dongle-S3 that has been flashed with suspicious firmware. The device taunts you on its screen but never reveals its secret. Intelligence suggests there is a flag hidden somewhere in the firmware image. Your job is to extract it.

## Your Goal

Complete the lab by:

- Dumping the firmware from the device using `esptool`
- Analysing the binary to find the encrypted flag and its key
- Decrypting the flag

## What You Can Use

- The LilyGo device and station materials
- Your own laptop/phone, if allowed
- AI, search engines, and documentation
- Tools like Ghidra, Binary Ninja, CyberChef, a hex editor, or plain Python
- Help from a volunteer if you get stuck

## Rules

- Do not unplug or reflash the device — other participants need it
- You may copy the dumped firmware to your own machine for analysis
- Any RE tool or approach is fair game

## Starting Point

1. Plug the device into a USB port
2. Confirm the LCD shows a message (press BOOT to cycle taunts)
3. Put the device into download mode if needed (hold BOOT while plugging in)
4. Identify the serial port:

```bash
ls /dev/ttyACM* /dev/ttyUSB*
```

5. Dump the firmware:

```bash
esptool.py --port /dev/ttyACM0 read_flash 0 0x400000 firmware.bin
```

6. Open the dump in your RE tool of choice

## Checkpoints

You are probably on the right track if:

- You have a `firmware.bin` file on disk
- You can find two short byte arrays near each other in the binary — one is a key, the other is ciphertext
- You recognise the encryption as a simple XOR

## Completion

To finish the station, show a volunteer:

- The decrypted flag

**Need a hint? Ask a volunteer.**
