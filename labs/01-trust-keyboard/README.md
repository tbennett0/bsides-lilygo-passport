# Trust Me, I’m a Keyboard — USB HID trust demo

A hands-on lab demonstrating that operating systems **immediately trust USB HID devices** with no driver prompt or user consent. When plugged into a Lubuntu host and activated with the BOOT button, this LILYGO T-Dongle-S3 opens a terminal and reads a flag file — proving that a device claiming to be a keyboard can inject keystrokes the moment it is plugged in.

> **Educational use only.** This firmware types a benign `cat` command to display a flag file. It does not install software, exfiltrate data, or modify the host system.

## Hardware

| Component | Detail |
|-----------|--------|
| Board | [LILYGO T-Dongle-S3](https://github.com/Xinyuan-LilyGO/T-Dongle-S3) (ESP32-S3, USB-A plug) |
| Display | 0.96" ST7735 IPS LCD, 80x160, SPI |
| LED | APA102 single RGB LED |
| Button | BOOT (GPIO0) — used to trigger the payload |

## What It Does

1. Plug the dongle into a Lubuntu machine's USB-A port.
2. The device enumerates as **"BSides Lab — Trust Demo Keyboard"** (a standard USB HID keyboard). The OS loads a generic HID driver automatically — no prompt, no consent.
3. The LCD shows **READY** with a green LED.
4. Press the **BOOT** button on the dongle.
5. The device sends **Ctrl+Alt+T** to open a terminal, waits 1.5 seconds, then types:
   ```
   cat ~/flag.txt
   ```
   followed by Enter.
6. The flag `FLAG{HID_TRUST_IS_BLIND}` appears in the terminal.

## Prerequisites

- **ESP-IDF v5.3+** installed and sourced (`source $IDF_PATH/export.sh`)
- The `idf.py` tool available on your PATH
- USB cable for flashing (or the dongle's own USB-A plug in a USB-A port with OTG/host support)

## Build and Flash

From the **repository root**, enter this lab’s firmware directory:

```bash
cd labs/01-trust-keyboard

# 1. Set target (one-time)
idf.py set-target esp32s3

# 2. (Optional) Customise the payload, timings, or LCD settings
idf.py menuconfig
#   → "Trust Me, I am a Keyboard — Configuration"

# 3. Build
idf.py build

# 4. Flash
#    Put the dongle in download mode: hold BOOT, plug in (or press RST
#    while holding BOOT if already plugged in), then release BOOT.
idf.py -p /dev/ttyACM0 flash

# 5. Monitor serial output (optional, for debugging)
idf.py -p /dev/ttyACM0 monitor
```

> **Windows users:** Replace `/dev/ttyACM0` with your COM port (e.g. `COM3`).

## Configuration (menuconfig)

| Setting | Default | Description |
|---------|---------|-------------|
| `PAYLOAD_COMMAND` | `cat ~/flag.txt` | Shell command typed on the host |
| `OPEN_TERMINAL` | `y` | Send Ctrl+Alt+T before typing |
| `TERMINAL_OPEN_DELAY_MS` | `1500` | Wait time (ms) for terminal to appear |
| `KEY_PRESS_MS` | `50` | Duration each key is held |
| `KEY_RELEASE_MS` | `50` | Gap between key releases |
| `ENABLE_LCD` | `y` | Drive the ST7735 display |
| `LCD_BACKLIGHT_INVERTED` | `y` | Flip if backlight is active-low on your board |

## Lab Setup (Lubuntu Host)

### 1. Create the flag file

```bash
echo "FLAG{HID_TRUST_IS_BLIND}" > ~/flag.txt
```

### 2. Verify USB enumeration

Open a terminal and run:

```bash
# Watch kernel messages as you plug in the dongle
dmesg -w
```

You should see something like:
```
usb 1-1: new full-speed USB device number 5 using xhci_hcd
input: BSides Lab Trust Demo Keyboard as /devices/.../input/input12
hid-generic 0003:303A:4004.0008: input: USB HID v1.11 Keyboard [BSides Lab Trust Demo Keyboard]
```

Then confirm with:
```bash
lsusb | grep -i bsides
```

### 3. (Optional) Watch raw HID events

```bash
sudo apt install evtest
sudo evtest
# Select the "Trust Demo Keyboard" device, then press BOOT on the dongle
```

### 4. (Facilitator) Pre-provision machine with setup scripts

From the repository root:

```bash
./setup-base.sh
./setup-labs.sh trust-keyboard
# or run all steps:
./provision.sh
```

## Lab Discussion Points

### Phase 2 — Investigation

- **What device class was this?** USB HID (Human Interface Device), subclass keyboard.
- **Why no driver prompt?** HID is a "trusted" class — the OS has built-in drivers and loads them automatically for any device claiming to be a keyboard, mouse, or gamepad.
- **Why did this happen?** The USB protocol delegates trust to the device. The device tells the host what it is, and the host believes it. There is no cryptographic verification.

### Phase 3 — Defence

- **USB device class whitelisting** — tools like USBGuard (Linux) or Group Policy (Windows) can block unknown HID devices.
- **Endpoint Detection and Response (EDR)** — most EDR tools do not inspect HID traffic. They may detect the *effects* (e.g., suspicious shell commands) but not the injection method.
- **Physical port control** — disable unused USB ports in BIOS, use port blockers, or limit USB access via policy.
- **User awareness** — never plug in untrusted USB devices. This is the simplest and most effective defence.

## Project Structure

```
labs/01-trust-keyboard/         (ESP-IDF project root)
├── CMakeLists.txt              Project definition
├── sdkconfig.defaults          ESP32-S3 / USB / flash defaults
├── README.md                   This file
├── setup.sh                    Lab provisioning script (used by setup-labs.sh)
└── main/
    ├── CMakeLists.txt          Component source list
    ├── idf_component.yml       esp_tinyusb dependency
    ├── Kconfig.projbuild       Configurable settings
    ├── board.h                 Pin definitions (T-Dongle-S3)
    ├── main.c                  Entry point + state machine
    ├── app_state.h / .c        State enum + helper
    ├── hid_keyboard.h / .c     TinyUSB HID keyboard driver
    ├── button.h / .c           GPIO0 BOOT button (debounced)
    ├── led.h / .c              APA102 RGB LED driver
    ├── display.h / .c          ST7735 LCD driver
    └── font8x8.h               8x8 bitmap font
```

## Extending This Project

The modular architecture makes it straightforward to:

- **Change the payload** — edit `PAYLOAD_COMMAND` in menuconfig (no code changes)
- **Add new payloads** — extend `execute_payload()` in `main.c`
- **Port to another board** — change pin definitions in `board.h`
- **Add mouse/consumer HID** — extend the report descriptor in `hid_keyboard.c`
- **Disable the LCD** — set `ENABLE_LCD=n` in menuconfig; all display calls become no-ops

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| LCD stays dark | Try setting `LCD_BACKLIGHT_INVERTED=y` in menuconfig |
| Keys appear garbled | Ensure the host uses a **US QWERTY** keyboard layout |
| Terminal doesn't open | Increase `TERMINAL_OPEN_DELAY_MS`, or ensure Ctrl+Alt+T is bound in the desktop environment |
| Device doesn't enumerate | Reflash: hold BOOT → plug in → release BOOT → `idf.py flash` |
| Characters are missed | Increase `KEY_PRESS_MS` and `KEY_RELEASE_MS` to 80–100 |
