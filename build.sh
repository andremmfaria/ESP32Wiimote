#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT_DIR"

TARGET="${1:-help}"
shift || true

require_cmd() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Error: required command '$1' is not installed or not in PATH." >&2
        exit 1
    fi
}

pio_cmd() {
    require_cmd pio
    pio "$@"
}

run_coverage() {
    require_cmd gcovr
    require_cmd lcov
    require_cmd genhtml

    # Execute coverage-instrumented tests.
    pio_cmd test -e native-coverage "$@"

    # Prepare output directory.
    mkdir -p coverage
    rm -f ./*.gcov
    rm -f coverage/*.txt coverage/*.xml coverage/*.info
    rm -rf coverage/html-gcovr coverage/html-lcov
    mkdir -p coverage/html-gcovr coverage/html-lcov

    # Generate gcovr summary + XML + HTML details.
    gcovr \
        --root . \
        --filter src \
        --exclude 'test/.*' \
        --exclude '.*/.pio/.*' \
        --txt \
        --output coverage/gcovr-summary.txt \
        --print-summary

    gcovr \
        --root . \
        --filter src \
        --exclude 'test/.*' \
        --exclude '.*/.pio/.*' \
        --xml-pretty \
        --output coverage/gcovr.xml

    gcovr \
        --root . \
        --filter src \
        --exclude 'test/.*' \
        --exclude '.*/.pio/.*' \
        --html-details \
        --output coverage/html-gcovr/index.html

    # Generate lcov + genhtml report.
    lcov \
        --capture \
        --directory .pio/build/native-coverage \
        --output-file coverage/lcov.info \
        --rc lcov_branch_coverage=1 >/dev/null

    lcov \
        --remove coverage/lcov.info '/usr/*' '*/test/*' '*/.pio/*' \
        --output-file coverage/lcov.info \
        --rc lcov_branch_coverage=1 >/dev/null

    genhtml \
        coverage/lcov.info \
        --output-directory coverage/html-lcov \
        --branch-coverage >/dev/null

    # Keep repository root clean from gcov side artifacts.
    rm -f ./*.gcov

    echo "Coverage outputs:"
    echo "  - coverage/gcovr-summary.txt"
    echo "  - coverage/gcovr.xml"
    echo "  - coverage/html-gcovr/index.html"
    echo "  - coverage/lcov.info"
    echo "  - coverage/html-lcov/index.html"
}

with_port_args() {
    # Optional hardware port override for ESP32 targets.
    if [[ -n "${ESP32_PORT:-}" ]]; then
        echo "--upload-port" "$ESP32_PORT"
    fi
}

usage() {
    cat <<'EOF'
Proto Build System

Usage:
  ./build.sh <target> [extra args]

Targets:
  help                 Show this help.
  test:native          Run native unit tests.
  test:coverage        Run native coverage and write reports to coverage/.
  test:dev             Run embedded tests on esp32dev (hardware).
  test:dev:build       Build embedded tests without upload/execution.
  build:dev            Build esp32dev firmware.
  release              Build release firmware (esp32dev-release).
  upload:dev           Build and upload esp32dev firmware.
  upload:release       Build and upload esp32dev-release firmware.
  monitor:dev          Open serial monitor for esp32dev.
  clean                Clean all PlatformIO build artifacts.
  clean:coverage       Remove generated coverage artifacts.

Environment Variables:
  ESP32_PORT           Serial port for hardware actions (example: /dev/ttyUSB0)

Examples:
  ./build.sh test:native
  ./build.sh test:dev
  ESP32_PORT=/dev/ttyUSB0 ./build.sh test:dev
  ./build.sh test:coverage
  ./build.sh release
EOF
}

case "$TARGET" in
    help|-h|--help)
        usage
        ;;

    test:native)
        pio_cmd test -e native "$@"
        ;;

    test:coverage)
        run_coverage "$@"
        ;;

    test:dev)
        pio_cmd test -e esp32dev $(with_port_args) "$@"
        ;;

    test:dev:build)
        pio_cmd test -e esp32dev --without-uploading --without-testing "$@"
        ;;

    build:dev)
        pio_cmd run -e esp32dev "$@"
        ;;

    release)
        pio_cmd run -e esp32dev-release "$@"
        ;;

    upload:dev)
        pio_cmd run -e esp32dev -t upload $(with_port_args) "$@"
        ;;

    upload:release)
        pio_cmd run -e esp32dev-release -t upload $(with_port_args) "$@"
        ;;

    monitor:dev)
        pio_cmd device monitor -b 115200 $(with_port_args) "$@"
        ;;

    clean)
        pio_cmd run -t clean "$@"
        ;;

    clean:coverage)
        rm -rf coverage
        rm -f ./*.gcov
        find . -name '*.gcda' -o -name '*.gcno' | xargs -r rm -f
        ;;

    *)
        echo "Error: unknown target '$TARGET'" >&2
        echo
        usage
        exit 2
        ;;
esac
