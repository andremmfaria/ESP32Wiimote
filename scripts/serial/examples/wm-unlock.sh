#!/usr/bin/env bash
set -euo pipefail
TOKEN="${1:-${SERIAL_TOKEN:-esp32wiimote_serial_token_v1}}"
SECONDS_WINDOW="${2:-60}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm unlock "$TOKEN" "$SECONDS_WINDOW"
