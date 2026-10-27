#!/bin/sh
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec bash "$SCRIPT_DIR/../create.dmg.sh" ecode "$@"
