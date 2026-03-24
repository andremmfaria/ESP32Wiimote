#!/usr/bin/env bash
set -euo pipefail
REASON="${1:-0x16}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm disconnect "$REASON"
