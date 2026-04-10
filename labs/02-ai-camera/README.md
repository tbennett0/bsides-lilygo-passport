# CyberPassport Participant Handout Template

## Find AI Camera

**Category:** [Blue Team Boulevard / Intelligence Collection & Comms]  
**Difficulty:** [Beginner]  
**Target Time:** [10-20 minutes]

## Scenario

Your nemesis has set up AI-powered security cameras around the campus. You have been assigned to locate each camera and determine the direction it is facing.

Use the provided web flasher tool to program your LilyGo T3. Once flashing is complete, take the LilyGo device and walk around the building to identify the Wi-Fi and Bluetooth networks the cameras are broadcasting.

**Do not touch or harm the cameras.** Perform reconnaissance only.

## Your Goal

Complete the lab by identifying the `FLAG{}` string captured by the LilyGo scanner.

## What You Can Use

- Any Chrome-based laptop to flash the LilyGo T3

## Rules

- Do not touch the cameras

## Starting Point

1. Open a Chrome-based browser and navigate to [https://flock-finder.org/](https://flock-finder.org/)
2. Flash the firmware

## Checkpoints

You are probably on the right track if:
- The LilyGo screen is on and displays the message “scanning”

## Completion

To finish the station, show a volunteer the **unredacted flag**:

- `FLAG{REDACTED}`

**Need a hint? Ask a volunteer.**


## Addional information

After the event, you can flash the firmware from https://flock-finder.org/
Or from this repo: https://github.com/spuder/flock-you

Alternativly flash from the .bin file in this repo

```bash
esptool \
  --chip esp32s3 \
  --port /dev/cu.usbmodem11301 \
  --baud 921600 \
  --before default_reset \
  --after hard_reset \
  write_flash \
  -z \
  --flash_mode dio \
  --flash_size detect \
  0x0 ./t_dongle_s3-factory.bin
```
