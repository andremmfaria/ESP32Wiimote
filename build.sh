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

    # Generate gcovr reports for covered files.
    gcovr \
        --root . \
        --filter src \
        --exclude 'test/.*' \
        --exclude '.*/.pio/.*' \
        --txt \
        --output coverage/gcovr-covered-summary.txt \
        --print-summary

    gcovr \
        --root . \
        --filter src \
        --exclude 'test/.*' \
        --exclude '.*/.pio/.*' \
        --csv \
        --output coverage/gcovr.csv

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

    # Build a full source inventory report so missing instrumentation is explicit.
    find src -type f | sort > coverage/src-all-files.txt
    awk -F, 'NR > 1 {print $1}' coverage/gcovr.csv | sort > coverage/src-covered-files.txt || true

    # Compose a full summary including files without gcov data as 0% coverage.
    awk -F, '
        NR == 1 {next}
        {
            file = $1;
            lineTotal[file] = $2;
            lineExec[file] = $3;
            linePct[file] = $4;
        }
        END {
            for (f in lineTotal) {
                print f "|" lineTotal[f] "|" lineExec[f] "|" linePct[f];
            }
        }
    ' coverage/gcovr.csv | sort > coverage/gcovr-covered-map.txt

    {
        printf '%-46s %8s %8s %8s   %s\n' "File" "Lines" "Exec" "Cover" "Status"
        printf '%-46s %8s %8s %8s   %s\n' "----------------------------------------------" "--------" "--------" "--------" "----------------"

        totalLines=0
        totalExec=0

        while IFS= read -r srcFile; do
            if coveredLine=$(grep -F "${srcFile}|" coverage/gcovr-covered-map.txt); then
                lines=$(echo "$coveredLine" | cut -d'|' -f2)
                execLines=$(echo "$coveredLine" | cut -d'|' -f3)
                coverRaw=$(echo "$coveredLine" | cut -d'|' -f4)
                cover=$(awk -v p="$coverRaw" 'BEGIN { printf "%.1f%%", p * 100.0 }')
                status="COVERAGE_DATA"
            else
                lines=$(wc -l < "$srcFile")
                execLines=0
                cover="0.0%"
                status="NO_COVERAGE_DATA"
            fi

            totalLines=$((totalLines + lines))
            totalExec=$((totalExec + execLines))

            printf '%-46s %8d %8d %8s   %s\n' "$srcFile" "$lines" "$execLines" "$cover" "$status"
        done < coverage/src-all-files.txt

        if [ "$totalLines" -gt 0 ]; then
            totalPct=$(awk -v e="$totalExec" -v t="$totalLines" 'BEGIN { printf "%.1f%%", (e / t) * 100 }')
        else
            totalPct="0.0%"
        fi

        printf '%-46s %8s %8s %8s\n' "----------------------------------------------" "--------" "--------" "--------"
        printf '%-46s %8d %8d %8s\n' "TOTAL" "$totalLines" "$totalExec" "$totalPct"
    } > coverage/gcovr-summary.txt

    {
        echo "# Source Coverage Status"
        echo
        echo "Legend:"
        echo "  COVERAGE_DATA = file has gcov data in this run"
        echo "  NO_COVERAGE_DATA = file has no gcov data in this run"
        echo
        while IFS= read -r srcFile; do
            if grep -qx "$srcFile" coverage/src-covered-files.txt; then
                echo "COVERAGE_DATA $srcFile"
            else
                echo "NO_COVERAGE_DATA $srcFile"
            fi
        done < coverage/src-all-files.txt
    } > coverage/src-coverage-status.txt

    # Generate lcov + genhtml report.
    lcov \
        --capture \
        --directory .pio/build/native-coverage \
        --output-file coverage/lcov.info \
        --rc branch_coverage=1 \
        --rc geninfo_unexecuted_blocks=1 >/dev/null

    lcov \
        --remove coverage/lcov.info '/usr/*' '*/test/*' '*/.pio/*' \
        --output-file coverage/lcov.info \
        --rc branch_coverage=1 >/dev/null

    genhtml \
        coverage/lcov.info \
        --output-directory coverage/html-lcov \
        --branch-coverage >/dev/null

    # Keep repository root clean from gcov side artifacts.
    rm -f ./*.gcov

    echo "Coverage outputs:"
    echo "  - coverage/gcovr-summary.txt"
    echo "  - coverage/gcovr-covered-summary.txt"
    echo "  - coverage/gcovr.csv"
    echo "  - coverage/gcovr.xml"
    echo "  - coverage/html-gcovr/index.html"
    echo "  - coverage/src-coverage-status.txt"
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
  test:native:list     List native test modules available for per-module runs.
  test:native:<module> Run a single native module test (example: test:native:wifi_queue).
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
  ./build.sh test:native:list
  ./build.sh test:native:wifi_router
  ./build.sh test:dev
  ESP32_PORT=/dev/ttyUSB0 ./build.sh test:dev
  ./build.sh test:coverage
  ./build.sh release
EOF
}

list_native_modules() {
        find test/native -maxdepth 1 -mindepth 1 -type d -name 'test_*' \
                -exec basename {} \; \
                | sed 's/^test_//' \
                | sort
}

run_native_module() {
        local module="$1"
    shift
        local normalized="${module//-/_}"
        local filter="native/test_${normalized}"

        if [[ ! -d "test/native/test_${normalized}" ]]; then
                echo "Error: unknown native module '${module}'" >&2
                echo >&2
                echo "Available modules:" >&2
                list_native_modules | sed 's/^/  - /' >&2
                exit 2
        fi

        pio_cmd test -e native --filter "$filter" "$@"
}

case "$TARGET" in
    help|-h|--help)
        usage
        ;;

    test:native)
        pio_cmd test -e native "$@"
        ;;

    test:native:list)
        list_native_modules
        ;;

    test:native:*)
        module="${TARGET#test:native:}"
        run_native_module "$module" "$@"
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
