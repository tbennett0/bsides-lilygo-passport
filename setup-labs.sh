#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
# setup-labs.sh — Discover and run all lab setup scripts.
#
# Looks for setup.sh in each subdirectory of labs/ and runs them in
# alphabetical order. Each lab's setup.sh is self-contained and
# responsible for cloning its own repo, building, creating files, etc.
#
# Usage:
#   ./setup-labs.sh                   # run all labs
#   ./setup-labs.sh trust-keyboard    # run only labs matching substring
#
# Convention:
#   labs/
#   ├── 01-trust-keyboard/
#   │   └── setup.sh            # must be executable
#   ├── 02-wifi-sniff/
#   │   └── setup.sh
#   └── ...
#
# Each setup.sh receives these environment variables:
#   LABS_BASE_DIR  — absolute path to the labs/ directory
#   ESP_IDF_DIR    — path to ESP-IDF (if installed)
#   LAB_NAME       — the directory name of this lab
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
LABS_DIR="$SCRIPT_DIR/labs"
FILTER="${1:-}"

# ── Colors ─────────────────────────────────────────────────────────
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${GREEN}[+]${NC} $*"; }
warn()  { echo -e "${YELLOW}[!]${NC} $*"; }
error() { echo -e "${RED}[x]${NC} $*"; }
header(){ echo -e "\n${CYAN}━━━ $* ━━━${NC}"; }

# ── Source ESP-IDF if available ────────────────────────────────────
export ESP_IDF_DIR="${ESP_IDF_DIR:-$HOME/esp/esp-idf}"
if [ -f "$ESP_IDF_DIR/export.sh" ]; then
    . "$ESP_IDF_DIR/export.sh" > /dev/null 2>&1
fi

export LABS_BASE_DIR="$LABS_DIR"

# ── Discover and run lab setup scripts ─────────────────────────────
if [ ! -d "$LABS_DIR" ]; then
    error "No labs/ directory found at $LABS_DIR"
    echo "  Create labs/<lab-name>/setup.sh to get started."
    exit 1
fi

FOUND=0
PASS=0
FAIL=0

for lab_dir in "$LABS_DIR"/*/; do
    [ -d "$lab_dir" ] || continue

    lab_name="$(basename "$lab_dir")"
    setup_script="$lab_dir/setup.sh"

    # Apply filter if provided
    if [ -n "$FILTER" ] && [[ "$lab_name" != *"$FILTER"* ]]; then
        continue
    fi

    if [ ! -f "$setup_script" ]; then
        warn "Skipping $lab_name — no setup.sh found"
        continue
    fi

    FOUND=$((FOUND + 1))
    header "Setting up: $lab_name"

    export LAB_NAME="$lab_name"

    if bash "$setup_script"; then
        info "$lab_name — done!"
        PASS=$((PASS + 1))
    else
        error "$lab_name — FAILED (exit code $?)"
        FAIL=$((FAIL + 1))
    fi
done

# ── Summary ────────────────────────────────────────────────────────
echo ""
echo "════════════════════════════════════════════════════════════════"
if [ "$FOUND" -eq 0 ]; then
    if [ -n "$FILTER" ]; then
        warn "No labs matching '$FILTER' found in $LABS_DIR"
    else
        warn "No labs found in $LABS_DIR"
    fi
    echo "  Create labs/<lab-name>/setup.sh to add a lab."
else
    info "Labs complete: $PASS passed, $FAIL failed (of $FOUND found)"
fi
echo "════════════════════════════════════════════════════════════════"

[ "$FAIL" -eq 0 ] || exit 1
