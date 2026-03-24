#!/usr/bin/env bash
set -euo pipefail

SERIAL_PORT="${ESP32_PORT:-/dev/ttyUSB0}"
SERIAL_BAUD="${ESP32_BAUD:-115200}"
SERIAL_RESPONSE_TIMEOUT="${ESP32_SERIAL_TIMEOUT:-3}"

serial_setup_port() {
  stty -F "$SERIAL_PORT" "$SERIAL_BAUD" raw -echo -echoe -echok -echoctl -echoke -icrnl -ixon -ixoff
}

serial_open() {
  serial_setup_port
  exec 3<>"$SERIAL_PORT"
}

serial_close() {
  exec 3>&-
}

serial_drain() {
  local line
  while IFS= read -r -t 0.05 line <&3; do :; done
}

serial_send_line() {
  local line="$1"
  printf '%s\n' "$line" >&3
}

serial_read_response() {
  local line
  local had_output=0
  local start_seconds=$SECONDS

  while true; do
    if IFS= read -r -t 0.2 line <&3; then
      had_output=1
      printf '%s\n' "$line"
      if [[ "$line" == @wm:* ]]; then
        return 0
      fi
      continue
    fi

    if [[ "$had_output" -eq 1 ]]; then
      return 0
    fi

    if (( SECONDS - start_seconds >= SERIAL_RESPONSE_TIMEOUT )); then
      return 1
    fi
  done
}

serial_exchange() {
  local cmd="$1"
  serial_send_line "$cmd"
  if ! serial_read_response; then
    printf 'No serial response within %ss for command: %s\n' "$SERIAL_RESPONSE_TIMEOUT" "$cmd" >&2
    return 1
  fi
  return 0
}
