#!/usr/bin/env bash
set -euo pipefail
TOKEN="${1:-wifi_api_token_v2}"
"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/send.sh" wm wifi-set-token "$TOKEN"
