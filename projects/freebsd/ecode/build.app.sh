#!/bin/sh
# Compatibility entry point; all application packaging lives in the platform script.
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec bash "$SCRIPT_DIR/../build.app.sh" ecode "$@"
