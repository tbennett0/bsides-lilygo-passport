#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
# Lab: Trust Me, I'm a Keyboard — USB HID trust demo
#
# Called by setup-labs.sh. Expects ESP-IDF to already be installed
# (run setup-base.sh first).
#
# What it does:
#   1. Clones the lab project (or pulls latest)
#   2. Sets ESP32-S3 target and pre-warms the build cache
#   3. Creates ~/flag.txt
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

# ── Configuration ──────────────────────────────────────────────────
LAB_REPO="https://github.com/tbennett0/bsides-lilygo-passport.git"
LAB_DIR="$HOME/bsides-lilygo-passport"
FLAG_TEXT="FLAG{HID_TRUST_IS_BLIND}"

# ── Colors ─────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[+]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }

# ── Preflight ──────────────────────────────────────────────────────
if ! command -v idf.py > /dev/null 2>&1; then
    echo "ERROR: idf.py not found. Run setup-base.sh first." >&2
    exit 1
fi

# ── Clone / update ─────────────────────────────────────────────────
if [ -d "$LAB_DIR" ]; then
    info "Lab project already at $LAB_DIR — pulling latest..."
    cd "$LAB_DIR"
    git pull --ff-only || warn "Could not pull — using existing checkout."
else
    info "Cloning lab project..."
    git clone "$LAB_REPO" "$LAB_DIR"
fi

# Firmware path matches repo layout: labs/<same name as this script's parent>
LAB_ID="${LAB_NAME:-01-trust-keyboard}"
cd "$LAB_DIR/labs/$LAB_ID"

# ── Build ──────────────────────────────────────────────────────────
info "Setting target to ESP32-S3..."
idf.py set-target esp32s3

info "Pre-warming build cache (first build takes 2-3 minutes)..."
idf.py build

# ── Flag file ──────────────────────────────────────────────────────
info "Creating ~/flag.txt..."
echo "$FLAG_TEXT" > "$HOME/flag.txt"

# ── Done ───────────────────────────────────────────────────────────
echo ""
info "\"Trust Me, I'm a Keyboard\" lab ready!"
echo "  Firmware:   $LAB_DIR/labs/$LAB_ID"
echo "  Flag:       ~/flag.txt -> $FLAG_TEXT"
echo "  Workflow:   cd labs/$LAB_ID && edit main/hid_keyboard.c -> idf.py build flash"
