#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-0x31}"
CONTINUOUS="${2:-on}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm mode "$MODE" "$CONTINUOUS"
