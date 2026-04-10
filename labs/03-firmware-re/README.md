# 03 — Firmware Reverse Engineering

A firmware extraction and reverse engineering challenge for the LILYGO T-Dongle-S3.

## Concept

The device is flashed with firmware containing an XOR-encrypted flag. The LCD displays taunting messages but never reveals the secret. Participants must:

1. Dump the firmware using `esptool`
2. Find the XOR key and ciphertext in a reverse engineering tool
3. Decrypt the flag (e.g. with CyberChef)

## Hardware

- LILYGO T-Dongle-S3 (ESP32-S3, ST7735 LCD, APA102 LED)

## Build

Requires [ESP-IDF v5.3+](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/get-started/).

```bash
cd labs/03-firmware-re
idf.py set-target esp32s3
idf.py build flash
```

## Project Structure

```
03-firmware-re/
├── CMakeLists.txt          # ESP-IDF project definition
├── sdkconfig.defaults      # Default config (ESP32-S3, 16MB flash)
├── PARTICIPANT.md          # Attendee handout
├── FACILITATOR.md          # Volunteer guide (contains answer key)
├── setup.sh                # Lab provisioning script
└── main/
    ├── CMakeLists.txt      # Component source list
    ├── idf_component.yml   # IDF dependencies
    ├── Kconfig.projbuild   # Menuconfig options (LCD enable)
    ├── main.c              # Entry point — XOR flag + taunt display
    ├── board.h             # T-Dongle-S3 pin definitions
    ├── display.h / .c      # ST7735 LCD driver
    ├── font8x8.h           # 8x8 bitmap font
    ├── led.h / .c          # APA102 RGB LED driver
    └── button.h / .c       # BOOT button debounce
```

## Device Behaviour

- On boot: LCD shows "SECRETS inside...", LED is purple
- Pressing BOOT cycles through taunt messages
- The flag is never displayed or printed at the default log level

## Documentation

- **PARTICIPANT.md** — Handout for attendees
- **FACILITATOR.md** — Volunteer guide with answer key, hint ladder, and troubleshooting
