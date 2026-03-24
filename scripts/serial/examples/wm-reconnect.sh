#!/usr/bin/env bash
set -euo pipefail
ACTION="${1:-on}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm reconnect "$ACTION"
