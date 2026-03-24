#!/usr/bin/env bash
set -euo pipefail
ACTION="${1:-start}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm discover "$ACTION"
