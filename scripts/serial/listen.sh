#!/usr/bin/env bash
set -euo pipefail

PORT="${ESP32_PORT:-/dev/ttyUSB0}"
BAUD="${ESP32_BAUD:-115200}"
SECONDS_TO_LISTEN="10"
FOREVER=0

usage() {
  cat <<'EOF'
Listen to ESP32 serial output.

Usage:
  ESP32_PORT=/dev/ttyUSB0 scripts/serial/listen.sh [--seconds N] [--forever]

Options:
  -t, --seconds N   Listen for N seconds (default: 10)
  -f, --forever     Listen indefinitely until interrupted (Ctrl+C)
  -h, --help        Show this help

Environment:
  ESP32_PORT         Serial port (default: /dev/ttyUSB0)
  ESP32_BAUD         Baud rate (default: 115200)
EOF
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      -t|--seconds)
        if [[ $# -lt 2 ]]; then
          echo "error: missing value for $1" >&2
          exit 2
        fi
        SECONDS_TO_LISTEN="$2"
        shift 2
        ;;
      -f|--forever)
        FOREVER=1
        shift
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        echo "error: unknown argument: $1" >&2
        usage >&2
        exit 2
        ;;
    esac
  done

  if [[ "$FOREVER" -eq 0 ]]; then
    if ! [[ "$SECONDS_TO_LISTEN" =~ ^[0-9]+$ ]]; then
      echo "error: --seconds must be a non-negative integer" >&2
      exit 2
    fi
  fi
}

setup_port() {
  stty -F "$PORT" "$BAUD" raw -echo -echoe -echok -echoctl -echoke -icrnl -ixon -ixoff
}

listen_forever() {
  echo "Listening on ${PORT} @ ${BAUD} baud (forever). Press Ctrl+C to stop."
  while IFS= read -r line < "$PORT"; do
    printf '%s\n' "$line"
  done
}

listen_for_seconds() {
  local duration="$1"
  local start_time=$SECONDS

  echo "Listening on ${PORT} @ ${BAUD} baud for ${duration}s..."

  exec 3< "$PORT"
  while (( SECONDS - start_time < duration )); do
    if IFS= read -r -t 0.2 line <&3; then
      printf '%s\n' "$line"
    fi
  done
  exec 3<&-
}

main() {
  parse_args "$@"
  setup_port

  if [[ "$FOREVER" -eq 1 ]]; then
    listen_forever
  else
    listen_for_seconds "$SECONDS_TO_LISTEN"
  fi
}

main "$@"
