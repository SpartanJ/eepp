#!/bin/bash
# Compatibility entry point. Relative sources historically resolve from this directory.
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
[ $# -eq 2 ] || { echo "Usage: $0 <source_dir> <dest_dir>" >&2; exit 1; }
SOURCE_DIR=$1
[[ "$SOURCE_DIR" = /* ]] || SOURCE_DIR="$SCRIPT_DIR/$SOURCE_DIR"
exec bash "$SCRIPT_DIR/copy_app_assets.sh" ecode "$SOURCE_DIR" "$2"
