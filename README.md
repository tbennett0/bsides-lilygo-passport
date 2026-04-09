# Lab Machine Setup

Scripts for provisioning Lubuntu lab machines before the event.

## Structure

The repository keeps one ESP-IDF project per lab under `labs/<lab-id>/` at the repo root. Each lab directory now contains its own `setup.sh` so firmware and host provisioning steps stay together.

```
bsides-lilygo-passport/          # clone of this repo (LAB_DIR)
├── setup-base.sh                # One-time machine tooling (ESP-IDF, USB access)
├── setup-labs.sh                # Discovers and runs all lab setup scripts
├── provision.sh                 # Runs setup-base.sh + setup-labs.sh
├── labs/
│   └── 01-trust-keyboard/
│       ├── setup.sh             # “Trust Me, I'm a Keyboard” setup entrypoint
│       ├── CMakeLists.txt
│       └── main/
└── setup/                       # setup docs
```

## Quickstart

Run these from the repository root:

```bash
# 1. Install base tooling (once per machine)
./setup-base.sh

# 2. Log out and back in (required if user groups changed)

# 3. Run all lab setups
./setup-labs.sh

# 4. Or run a specific lab by name
./setup-labs.sh trust-keyboard
```

Or run everything in one command:

```bash
./provision.sh
```

---

## setup-base.sh

Run once per machine before the event. Lab-agnostic.

What it does:
- Installs system packages required by ESP-IDF (`git`, `cmake`, `ninja-build`, etc.)
- Clones and installs ESP-IDF `v5.3.2` (ESP32-S3 target only) to `~/esp/esp-idf`
- Adds `source ~/esp/esp-idf/export.sh` to `~/.bashrc` so `idf.py` is always in PATH
- Adds a udev rule for VID `303a` (Espressif) so USB devices are accessible without `sudo`
- Adds the current user to the `dialout` and `plugdev` groups for `/dev/ttyACM*` serial access

Note: if group membership changes, the user must log out and back in before flashing works without `sudo`.

---

## setup-labs.sh

Discovers and runs every `labs/*/setup.sh` in alphabetical order.

```bash
./setup-labs.sh                 # run all labs
./setup-labs.sh trust-keyboard  # run only labs whose directory name contains that substring
```

Each lab script runs in its own bash process, so a failure in one lab is reported but does not abort the others.

Environment variables passed to each lab's `setup.sh`:

| Variable | Value |
|----------|-------|
| `LABS_BASE_DIR` | Absolute path to the `labs/` directory |
| `ESP_IDF_DIR` | Path to ESP-IDF (`~/esp/esp-idf`) |
| `LAB_NAME` | The directory name of the lab being set up |

---

## Adding a new lab

1. At the repository root, add the ESP-IDF project:
   ```
   labs/02-my-new-lab/     # firmware sources, CMakeLists.txt, main/, …
   ```

2. In the same lab directory, add a setup script:
   ```
   labs/02-my-new-lab/setup.sh
   ```
   The script should `cd "$LAB_DIR/labs/$LAB_NAME"` (see `labs/01-trust-keyboard/setup.sh`) so the build path matches the repo. It should be self-contained:
   - Clone or update the repo (if needed)
   - Build firmware and create any host files (flags, configs, etc.)
   - Print a short summary when done

3. Check that `idf.py` is available at the start if your lab uses ESP-IDF:
   ```bash
   if ! command -v idf.py > /dev/null 2>&1; then
       echo "ERROR: idf.py not found. Run setup-base.sh first." >&2
       exit 1
   fi
   ```

4. `setup-labs.sh` will pick it up automatically; no registration needed.

Minimal `setup.sh` template:
```bash
#!/usr/bin/env bash
set -euo pipefail

GREEN='\033[0;32m'; NC='\033[0m'
info() { echo -e "${GREEN}[+]${NC} $*"; }

# your setup steps here

info "My lab is ready."
```

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `idf.py: command not found` | Run `source ~/.bashrc` or open a new terminal |
| `Permission denied` on `/dev/ttyACM0` | Log out and back in after running `setup-base.sh` |
| `git clone` fails | Check network access and confirm `LAB_REPO` is set in the lab's `setup.sh` |
| Build fails with missing components | Delete the `build/` directory in the lab project and run `idf.py build` again |
| udev rule not taking effect | Run `sudo udevadm control --reload-rules && sudo udevadm trigger`, then unplug and replug the device |
