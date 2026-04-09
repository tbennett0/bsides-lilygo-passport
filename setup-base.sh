#!/usr/bin/env bash
# setup-base.sh — Install shared tooling on a Lubuntu lab machine.
set -euo pipefail

# Configuration
ESP_IDF_DIR="$HOME/esp/esp-idf"
ESP_IDF_BRANCH="v5.3.2"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info()  { echo -e "${GREEN}[+]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
error() { echo -e "${RED}[x]${NC} $*"; exit 1; }

# 1) System packages
info "Installing system packages..."
sudo apt-get update -qq
sudo apt-get install -y -qq \
    git wget curl flex bison gperf python3 python3-pip python3-venv \
    cmake ninja-build ccache libffi-dev libssl-dev dfu-util \
    libusb-1.0-0 > /dev/null
info "System packages installed."

# 2) ESP-IDF
if [ -d "$ESP_IDF_DIR" ]; then
    info "ESP-IDF already present at $ESP_IDF_DIR — skipping clone."
else
    info "Cloning ESP-IDF $ESP_IDF_BRANCH (this takes a few minutes)..."
    mkdir -p "$(dirname "$ESP_IDF_DIR")"
    git clone --branch "$ESP_IDF_BRANCH" --recursive --depth 1 \
        https://github.com/espressif/esp-idf.git "$ESP_IDF_DIR"
fi

info "Running ESP-IDF install script (ESP32-S3 only)..."
cd "$ESP_IDF_DIR"
./install.sh esp32s3
info "ESP-IDF installed."

# 3) Shell profile
EXPORT_LINE=". \"$ESP_IDF_DIR/export.sh\" > /dev/null 2>&1"
PROFILE="$HOME/.bashrc"

if ! grep -qF "esp-idf/export.sh" "$PROFILE" 2>/dev/null; then
    info "Adding ESP-IDF to $PROFILE..."
    echo "" >> "$PROFILE"
    echo "# ESP-IDF (added by bsides lab setup)" >> "$PROFILE"
    echo "$EXPORT_LINE" >> "$PROFILE"
else
    info "ESP-IDF already in $PROFILE — skipping."
fi

# 4) USB access (udev + groups)
UDEV_RULE='SUBSYSTEM=="usb", ATTR{idVendor}=="303a", MODE="0666"'
UDEV_FILE="/etc/udev/rules.d/99-esp32.rules"

if [ ! -f "$UDEV_FILE" ] || ! grep -qF "303a" "$UDEV_FILE" 2>/dev/null; then
    info "Adding udev rule for ESP32-S3 USB access..."
    echo "$UDEV_RULE" | sudo tee "$UDEV_FILE" > /dev/null
    sudo udevadm control --reload-rules
    sudo udevadm trigger
else
    info "udev rule already present — skipping."
fi

GROUPS_CHANGED=false
for group in dialout plugdev; do
    if getent group "$group" > /dev/null 2>&1; then
        if ! id -nG "$USER" | grep -qw "$group"; then
            info "Adding $USER to $group group..."
            sudo usermod -aG "$group" "$USER"
            GROUPS_CHANGED=true
        fi
    fi
done

# 5) Summary
echo ""
echo "════════════════════════════════════════════════════════════════"
info "Base setup complete!"
echo ""

. "$ESP_IDF_DIR/export.sh" > /dev/null 2>&1
echo "  ESP-IDF:   $ESP_IDF_DIR"
echo "  Version:   $(idf.py --version 2>/dev/null || echo 'NOT FOUND')"
echo "  Profile:   $PROFILE (ESP-IDF auto-sourced)"
echo "  udev:      $UDEV_FILE"
echo ""
echo "════════════════════════════════════════════════════════════════"

if [ "$GROUPS_CHANGED" = true ]; then
    echo ""
    warn "User groups were changed. LOG OUT and back in before flashing."
fi

echo ""
info "Next: run ./setup-labs.sh to provision individual labs."
