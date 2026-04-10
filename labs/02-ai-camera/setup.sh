#!/usr/bin/env bash
# ---------------------------------------------------------------------
# Lab: Find AI Camera - install Google Chrome for web flashing
# ---------------------------------------------------------------------
set -euo pipefail

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

info()  { echo -e "${GREEN}[+]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
error() { echo -e "${RED}[x]${NC} $*"; exit 1; }

ARCH="$(dpkg --print-architecture)"
LAB_LABEL="${LAB_NAME:-02-ai-camera}"
KEYRING="/usr/share/keyrings/google-chrome.gpg"
SOURCE_LIST="/etc/apt/sources.list.d/google-chrome.list"
SOURCE_LINE="deb [arch=amd64 signed-by=$KEYRING] https://dl.google.com/linux/chrome/deb/ stable main"

if [ "$ARCH" != "amd64" ]; then
    error "Google Chrome is only supported by this setup on amd64 machines (detected: $ARCH)."
fi

info "Installing Google Chrome for $LAB_LABEL..."

if ! dpkg -s ca-certificates > /dev/null 2>&1 || ! dpkg -s gnupg > /dev/null 2>&1; then
    info "Installing package prerequisites..."
    sudo apt-get update -qq
    sudo apt-get install -y -qq ca-certificates gnupg > /dev/null
fi

if [ ! -f "$KEYRING" ]; then
    info "Adding Google Chrome signing key..."
    curl -fsSL https://dl.google.com/linux/linux_signing_key.pub | sudo gpg --dearmor -o "$KEYRING"
else
    info "Google Chrome signing key already present - skipping."
fi

if [ ! -f "$SOURCE_LIST" ] || ! grep -qF "$SOURCE_LINE" "$SOURCE_LIST" 2>/dev/null; then
    info "Adding Google Chrome apt repository..."
    echo "$SOURCE_LINE" | sudo tee "$SOURCE_LIST" > /dev/null
else
    info "Google Chrome apt repository already present - skipping."
fi

if dpkg -s google-chrome-stable > /dev/null 2>&1; then
    info "Google Chrome already installed - skipping package install."
else
    info "Installing Google Chrome..."
    sudo apt-get update -qq
    sudo apt-get install -y -qq google-chrome-stable > /dev/null
fi

CHROME_BIN=""
if command -v google-chrome > /dev/null 2>&1; then
    CHROME_BIN="$(command -v google-chrome)"
fi

if [ -z "$CHROME_BIN" ] && command -v google-chrome-stable > /dev/null 2>&1; then
    CHROME_BIN="$(command -v google-chrome-stable)"
fi

if [ -z "$CHROME_BIN" ]; then
    error "Google Chrome installation completed but no browser binary was found."
fi

echo ""
info "\"Find AI Camera\" lab ready!"
echo "  Browser:    $CHROME_BIN"
echo "  Workflow:   Run Google Chrome and open the lab web flasher."
