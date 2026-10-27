#!/bin/bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
exec bash "$SCRIPT_DIR/../scripts/build-unix-app.sh" haiku "$@"
