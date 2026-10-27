#!/bin/bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PLATFORM=mingw32
source "$SCRIPT_DIR/../scripts/app-package.sh"
package_init "$@"
if ! $SKIP_BUILD; then
	bash "$SCRIPT_DIR/make.sh" -e "config=${BUILDTYPE}_$ARCH" -j"$(nproc)" "--backend=$BACKEND" "$APP"
fi
STAGING="$OUTPUT_DIR/$APP"
rm -rf "$STAGING"
mkdir -p "$STAGING"
package_copy_assets "$STAGING"
SUFFIX=
[ "$BUILDTYPE" != debug ] || SUFFIX=-debug
cp "$SOURCE_DIR/$APP$SUFFIX.exe" "$STAGING/"
cp "$SOURCE_DIR/eepp$SUFFIX.dll" "$STAGING/"
SDL_DLL="$BACKEND.dll"
if [ -f "$SOURCE_DIR/$SDL_DLL" ]; then
	cp -L "$SOURCE_DIR/$SDL_DLL" "$STAGING/"
else
	SDL_VERSION=$(awk -v key="remote_${BACKEND}_version_number" 'tolower($1) == tolower(key) {gsub(/"/, "", $3); print $3; exit}' "$PACKAGE_ROOT/premake5.lua")
	TRIPLET=x86_64
	[ "$ARCH" != x86 ] || TRIPLET=i686
	if [ "$ARCH" = arm64 ]; then
		SDL_PATH="/usr/local/cross-tools/aarch64-w64-mingw32/bin/$SDL_DLL"
	else
		SDL_PATH="$PACKAGE_ROOT/src/thirdparty/$BACKEND-$SDL_VERSION/$TRIPLET-w64-mingw32/bin/$SDL_DLL"
	fi
	cp -L "$SDL_PATH" "$STAGING/"
fi
# zip updates existing archives; remove the previous one to avoid stale entries.
rm -f "$OUTPUT_DIR/$PACKAGE_NAME.zip"
(cd "$OUTPUT_DIR" && zip -qr "$PACKAGE_NAME.zip" "$APP")
echo "Generated $OUTPUT_DIR/$PACKAGE_NAME.zip"
