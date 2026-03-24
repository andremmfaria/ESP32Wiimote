#!/usr/bin/env bash
set -euo pipefail

TOKEN="${SERIAL_TOKEN:-esp32wiimote_serial_token_v1}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=scripts/serial/common.sh
source "${SCRIPT_DIR}/common.sh"

serial_open
trap serial_close EXIT
serial_drain

echo "== unlock =="
serial_exchange "wm unlock ${TOKEN} 60"
echo

echo "== wifi-status (before) =="
serial_exchange "wm wifi-status"
echo

echo "== wifi-control on =="
serial_exchange "wm wifi-control on"
echo

echo "== wifi-status (after) =="
serial_exchange "wm wifi-status"
echo

echo "== status =="
serial_exchange "wm status"
