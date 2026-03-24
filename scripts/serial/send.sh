#!/usr/bin/env bash
set -euo pipefail

print_help() {
  cat <<'EOF'
Send one ESP32Wiimote serial CLI command and print the immediate response.

Expected CLI format:
  wm <command> [args...]

Common examples:
  wm status
  wm unlock <token>
  wm wifi-status
  wm wifi-control on
  wm wifi-set-network <ssid> <password>

Environment:
  ESP32_PORT            Serial device path (default: /dev/ttyUSB0)
  ESP32_BAUD            Serial baud rate (default: 115200)
  ESP32_SERIAL_TIMEOUT  Response timeout in seconds (default: 3)

Usage:
  ESP32_PORT=/dev/ttyUSB0 scripts/serial/send.sh "wm status"
  scripts/serial/send.sh "wm wifi-control on"
EOF
}

if [[ ${1:-} == "-h" || ${1:-} == "--help" ]]; then
  print_help
  exit 0
fi

if [[ $# -lt 1 ]]; then
  print_help
  exit 2
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=scripts/serial/common.sh
source "${SCRIPT_DIR}/common.sh"

serial_open
trap serial_close EXIT
serial_drain
serial_exchange "$*"
