#!/usr/bin/env bash
set -euo pipefail
MASK="${1:-0x01}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm led "$MASK"
