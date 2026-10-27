#!/bin/sh
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
exec bash "$SCRIPT_DIR/../build.app.sh" ecode --arch x86_64 --use-frameworks "$@"
