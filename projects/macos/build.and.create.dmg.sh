#!/bin/bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)

if [ $# -eq 0 ] || [ "$1" = --help ]; then
	echo "Usage: $0 <app> [<app> ...] [build.app.sh options]"
	echo "       $0 all [build.app.sh options]"
	echo "all packages ecode, eefm, eeiv and eproc. Options apply to every app."
	[ $# -gt 0 ] && exit 0
	exit 1
fi

APPS=()
while [ $# -gt 0 ] && [[ "$1" != -* ]]; do
	if [ "$1" = all ]; then
		APPS+=(ecode eefm eeiv eproc)
	else
		APPS+=("$1")
	fi
	shift
done
[ ${#APPS[@]} -gt 0 ] || { echo "Error: expected an application or all" >&2; exit 1; }

for app in "${APPS[@]}"; do
	# build.app.sh creates the DMG after successfully building and signing the app.
	bash "$SCRIPT_DIR/build.app.sh" "$app" --dmg "$@"
done
