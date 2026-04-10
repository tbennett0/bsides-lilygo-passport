# BSides LilyGO Passport

A collection of conference labs built around the LILYGO T-Dongle-S3 and related station materials for BSides hands-on learning. The repository holds machine setup, some lab firmware projects themselves, host provisioning scripts, and the participant/facilitator documentation used to run the stations.

## What This Repository Contains

At a high level, this repo is organized around individual labs under `labs/`, plus shared setup and documentation assets at the repo root.

Current contents include:

- ESP-IDF firmware projects for LILYGO-based labs
- Per-lab host setup scripts and pre-event provisioning hooks
- Facilitator guides and participant handouts
- Reusable template files for future station writeups
- Shared machine setup scripts for Lubuntu-based event systems

## Current Labs

Current labs in the repo include:

- `labs/01-trust-keyboard`
- `labs/02-ai-camera`

`labs/01-trust-keyboard` demonstrates USB HID trust using a LILYGO T-Dongle-S3 that enumerates as a keyboard, waits for a physical button press, then injects a benign command on a Lubuntu host.

`labs/02-ai-camera` uses a browser-based web flasher to prepare a LilyGO device for locating nearby AI camera beacons, and its lab setup installs Google Chrome for that workflow.

See:

- `labs/01-trust-keyboard/README.md` for the technical lab reference
- `labs/01-trust-keyboard/PARTICIPANT.md` for the attendee-facing handout
- `labs/01-trust-keyboard/FACILITATOR.md` for the volunteer/facilitator guide
- `labs/02-ai-camera/README.md` for the AI camera participant handout

## Repository Layout

```text
bsides-lilygo-passport/
├── README.md                            # Project overview and onboarding
├── setup-base.sh                        # One-time machine tooling
├── setup-labs.sh                        # Runs per-lab setup scripts
├── provision.sh                         # Base setup + all lab setup
├── participant_handout_template.md      # Reusable participant template
├── builder_volunteer_guide_template.md  # Reusable builder/volunteer template
└── labs/
    ├── 01-trust-keyboard/
    │   ├── README.md
    │   ├── PARTICIPANT.md
    │   ├── FACILITATOR.md
    │   ├── setup.sh
    │   └── main/
    └── 02-ai-camera/
        ├── README.md
        └── setup.sh
```

## Getting Started

If you are here to provision a machine for the event, setup is still the quickest entry point.

Run these from the repository root:

```bash
# 1. Install shared tooling (once per machine)
./setup-base.sh

# 2. Log out and back in if user groups changed

# 3. Provision all labs
./setup-labs.sh

# 4. Or provision a specific lab
./setup-labs.sh trust-keyboard
./setup-labs.sh ai-camera
```

Or run the full process in one command:

```bash
./provision.sh
```

## Setup Scripts

### `setup-base.sh`

Run once per machine before the event. This is lab-agnostic setup.

What it does:

- Installs shared system packages needed by ESP-IDF
- Clones and installs ESP-IDF `v5.3.2` for `esp32s3`
- Adds ESP-IDF export sourcing to `~/.bashrc`
- Installs a udev rule for Espressif USB devices
- Adds the current user to `dialout` and `plugdev`

If group membership changes, the user must log out and back in before flashing works without `sudo`.

### `setup-labs.sh`

Discovers and runs each `labs/*/setup.sh` script in alphabetical order.

```bash
./setup-labs.sh
./setup-labs.sh trust-keyboard
```

Each lab setup runs in its own shell process, so one failing lab does not abort the rest. Lab-specific dependencies should live there; for example, `labs/02-ai-camera/setup.sh` installs Google Chrome for the browser flasher workflow.

Environment variables passed to each lab `setup.sh`:

| Variable | Value |
|----------|-------|
| `LABS_BASE_DIR` | Absolute path to the `labs/` directory |
| `ESP_IDF_DIR` | Path to ESP-IDF (`~/esp/esp-idf`) |
| `LAB_NAME` | The directory name of the lab being set up |

### `provision.sh`

Convenience wrapper that runs:

1. `setup-base.sh`
2. `setup-labs.sh`

Use it when preparing a new event machine from scratch.

## Adding a New Lab

To add another station:

1. Add the lab project under `labs/<lab-id>/`
2. Add a `setup.sh` in that same lab directory
3. Keep the lab self-contained: firmware, docs, and host setup steps should live together
4. If it uses ESP-IDF, verify `idf.py` is available to the setup script
5. Add lab-specific `README.md`, participant, and facilitator docs as needed

Minimal `setup.sh` example:

```bash
#!/usr/bin/env bash
set -euo pipefail

GREEN='\033[0;32m'; NC='\033[0m'
info() { echo -e "${GREEN}[+]${NC} $*"; }

# your setup steps here

info "My lab is ready."
```

## Templates

Use these when creating new lab docs:

- `participant_handout_template.md`
- `builder_volunteer_guide_template.md`

They match the structure used for conference-facing participant and volunteer materials.

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `idf.py: command not found` | Run `source ~/.bashrc` or open a new terminal |
| `Permission denied` on `/dev/ttyACM0` | Log out and back in after running `setup-base.sh` |
| `git clone` fails | Check network access and confirm `LAB_REPO` is set in the lab's `setup.sh` |
| Build fails with missing components | Delete the `build/` directory in the lab project and run `idf.py build` again |
| udev rule not taking effect | Run `sudo udevadm control --reload-rules && sudo udevadm trigger`, then unplug and replug the device |
