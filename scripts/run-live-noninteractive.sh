#!/usr/bin/env bash
set -euo pipefail

PORT="${ESP32_PORT:-/dev/ttyUSB0}"
SUITE="${1:-embedded/test_wifi_noninteractive}"
LOG_DIR="${2:-artifacts/live-tests}"
TS="$(date +%Y%m%d-%H%M%S)"
LOG_FILE="${LOG_DIR}/$(echo "${SUITE}" | tr '/' '_')-${TS}.log"

mkdir -p "${LOG_DIR}"

echo "Running non-interactive embedded test suite"
echo "  port: ${PORT}"
echo "  suite: ${SUITE}"
echo "  log:  ${LOG_FILE}"

pio test -e esp32dev --upload-port "${PORT}" -f "${SUITE}" -v | tee "${LOG_FILE}"

if grep -q "\[PASSED\]" "${LOG_FILE}"; then
  echo "Non-interactive live test completed; see ${LOG_FILE}"
else
  echo "Live test may have failed; inspect ${LOG_FILE}" >&2
  exit 1
fi
