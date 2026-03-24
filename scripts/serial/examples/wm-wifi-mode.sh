#!/usr/bin/env bash
set -euo pipefail
MODE="${1:-rest}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm wifi-mode "$MODE"
