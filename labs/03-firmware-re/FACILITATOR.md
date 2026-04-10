# Facilitator Guide — Firmware Reverse Engineering

**Do not distribute this file to participants.**

## Overview

This lab uses a LILYGO T-Dongle-S3 flashed with firmware that contains an XOR-encrypted flag. The device displays taunting messages on the LCD but never reveals the flag at runtime. Participants must dump the flash with `esptool`, find the key and ciphertext in a reverse engineering tool, and decrypt the flag.

The challenge is intentionally straightforward once you know the approach. The learning goal is the workflow: firmware extraction, binary analysis, and identifying simple cryptographic patterns.

## Learning Goals

By the end of the station, participants should be able to:

- Use `esptool` to read firmware from an ESP32 device
- Navigate a firmware binary in a disassembler or hex editor
- Identify XOR encryption by recognising a key and ciphertext in constant data
- Decrypt a XOR-encoded string using CyberChef, Python, or similar

## Lab At A Glance

- Hardware: LILYGO T-Dongle-S3
- Host: Lubuntu / any Linux with esptool and Python
- LCD shows: taunting messages ("SECRETS inside...", "Nice try", "Dump me", etc.)
- LED: purple at boot
- Flag: `hacker_escalate` (XOR key: `voodoo` / `0x76 0x6f 0x6f 0x64 0x6f 0x6f`)
- Total time: about 15-20 minutes

## Answer Key

### XOR Key (6 bytes)

```
0x76 0x6f 0x6f 0x64 0x6f 0x6f
```

ASCII: `voodoo`

### Encrypted Flag (15 bytes)

```
0x1e 0x0e 0x0c 0x0f 0x30 0x1b 0x1e 0x0a 0x30 0x14 0x03 0x0e 0x18 0x0a 0x1b
```

### Decrypted Flag

```
hacker_escalate
```

### Decryption in CyberChef

Recipe: **XOR** with key `voodoo` (UTF-8), input as hex:

```
1e0e0c0f301b1e0a30140300e180a1b
```

### Decryption in Python

```python
key = bytes([0x76, 0x6f, 0x6f, 0x64, 0x6f, 0x6f])
enc = bytes([0x1e, 0x0e, 0x0c, 0x0f, 0x30, 0x1b,
             0x1e, 0x0a, 0x30, 0x14, 0x03, 0x0e,
             0x18, 0x0a, 0x1b])
print(''.join(chr(e ^ key[i % len(key)]) for i, e in enumerate(enc)))
```

## Pre-Event Checklist

### 1. Provision the machine

From the repository root:

```bash
./provision.sh
```

Or separately:

```bash
./setup-base.sh
./setup-labs.sh firmware-re
```

### 2. Flash the device

```bash
cd labs/03-firmware-re
idf.py set-target esp32s3
idf.py build flash
```

If needed, specify the port:

```bash
idf.py -p /dev/ttyACM0 build flash
```

### 3. Verify device behaviour

- Plug in the dongle normally
- Confirm the LCD shows "SECRETS inside..."
- Confirm the LED is purple
- Press BOOT and confirm taunt messages cycle

### 4. Verify firmware can be dumped

```bash
esptool.py --port /dev/ttyACM0 read_flash 0 0x400000 /tmp/test_dump.bin
```

Confirm the dump completes and the file is ~4MB.

### 5. Verify the flag is findable

Search the dump for the key bytes:

```bash
xxd /tmp/test_dump.bin | grep "766f 6f64 6f6f"
```

Or open in Ghidra and locate the `xor_key` and `enc_flag` arrays near each other in the `.rodata` section.

## Timing Guide

| Phase | Target Time | Facilitator Notes |
|------|------:|---|
| Intro and setup | 1-2 min | Explain the scenario, point at esptool docs |
| Firmware dump | 2-3 min | Help with port detection if needed |
| Analysis in RE tool | 5-10 min | This is where most time is spent |
| Decryption | 2-3 min | Quick once they have key + ciphertext |
| Total | ~15-20 min | Adjust based on RE experience |

## Intended Solve Path

1. Participant plugs in the device and sees the taunts
2. They use `esptool.py read_flash` to dump the firmware
3. They open the dump in Ghidra, Binary Ninja, a hex editor, or `strings`/`xxd`
4. They locate two byte arrays near each other — one is `voodoo`, the other is the ciphertext
5. They recognise the XOR pattern (loop with modular key index)
6. They decrypt using CyberChef, Python, or manual XOR
7. They present the flag: `hacker_escalate`

## Hint Ladder

### Hint 1: Approach

> "The flag is not visible at runtime. You need to get it out of the firmware binary itself. Have you tried `esptool`?"

### Hint 2: What to look for

> "Once you have the binary, look for short byte arrays that sit near each other. One of them decodes to readable ASCII — that is your key."

### Hint 3: The cipher

> "The encryption is XOR with a repeating key. If you can find the key and the ciphertext, CyberChef can do the rest in one step."

### Hint 4: Specific

> "The key is 6 bytes and spells a word. The ciphertext is 15 bytes. They are in the read-only data section of the firmware. Try searching for `0x76 0x6f` in a hex editor."

### Hint 5: Near-spoiler

> "The key is `voodoo`. XOR the 15-byte ciphertext with it, repeating the key."

## Common Failure Points

- **Issue:** `esptool` cannot connect to the device  
  **Fix:** Hold BOOT while plugging in to enter download mode, or try a different USB port. Check `ls /dev/ttyACM*`.

- **Issue:** Participant cannot find the arrays in the binary  
  **Fix:** Suggest searching for `voodoo` as ASCII with `strings firmware.bin | grep voodoo`, or searching hex for `76 6f 6f 64 6f 6f`.

- **Issue:** Participant finds the arrays but does not recognise XOR  
  **Fix:** Point them at the code structure — a loop with `^=` and modular index is the classic XOR pattern. Suggest CyberChef's XOR recipe.

- **Issue:** Dump is very large and hard to navigate  
  **Fix:** Suggest dumping only the application partition instead of full flash: `esptool.py read_flash 0x10000 0x100000 app.bin`

## Volunteer Notes

- Beginners may not have used esptool before — walk them through the dump command
- Experienced RE people will solve this in under 5 minutes; use the stretch goals below
- The key (`voodoo`) is intentionally readable ASCII to make it findable via `strings`
- The flag is never printed or stored at default log level, so serial monitor will not reveal it

## Stretch Goals

- Can you find the flag without a disassembler, using only `strings` and `xxd`?
- Can you identify the XOR loop in the disassembly and explain how the compiler optimised it?
- What would make this harder to reverse? (Discuss: key derivation, whitebox crypto, code obfuscation)

## Reset Between Participants

1. Confirm the device is still flashed (LCD shows taunts)
2. Delete any firmware dumps left on the station machine
3. Close any open RE tools or terminals from the previous participant

## Safety / Guardrails

The firmware is intentionally simple and the XOR is meant to be broken. The device does not perform any network activity or USB HID injection. The only "secret" is the flag string.
