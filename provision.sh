#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────
# provision.sh — Full machine provisioning: base tooling + all labs.
#
# Usage:
#   chmod +x provision.sh
#   ./provision.sh
# ─────────────────────────────────────────────────────────────────────
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m'

info()   { echo -e "${GREEN}[+]${NC} $*"; }
header() { echo -e "\n${CYAN}━━━ $* ━━━${NC}\n"; }

header "Step 1 of 2: Base tooling"
bash "$SCRIPT_DIR/setup-base.sh"

header "Step 2 of 2: Labs"
bash "$SCRIPT_DIR/setup-labs.sh"

echo ""
echo "════════════════════════════════════════════════════════════════"
info "Machine provisioning complete."
echo ""
echo "  If user groups changed, LOG OUT and back in before flashing."
echo "════════════════════════════════════════════════════════════════"
