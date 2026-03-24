#!/usr/bin/env bash
set -euo pipefail
SSID="${1:-YOUR_WIFI_SSID}"
PASSWORD="${2:-YOUR_WIFI_PASSWORD}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm wifi-set-network "$SSID" "$PASSWORD"
