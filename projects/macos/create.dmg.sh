#!/bin/bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PLATFORM=macos
source "$SCRIPT_DIR/../scripts/app-package.sh"
package_init "$@"
BUNDLE="$OUTPUT_DIR/$APP.app"
[ -d "$BUNDLE" ] || package_error "Missing app bundle: $BUNDLE"
# hdiutil ships with macOS and does not require create-dmg or Finder automation.
STAGING=$(mktemp -d "${TMPDIR:-/tmp}/eepp-dmg.XXXXXX")
trap 'rm -rf "$STAGING"' EXIT
mkdir "$STAGING/contents"
cp -R "$BUNDLE" "$STAGING/contents/"
ln -s /Applications "$STAGING/contents/Applications"
IMAGE="$STAGING/$PACKAGE_NAME.dmg"
for attempt in 1 2 3; do
	# Disk Arbitration or indexing can briefly hold the image device busy on CI.
	# Keep logs and partial images outside srcfolder; publish only a completed image.
	# Preserve create-dmg's HFS+ filesystem instead of inheriting hdiutil's APFS default.
	if LC_ALL=C hdiutil create -ov -nospotlight -fs HFS+ -format UDZO -volname "$APP" \
		-srcfolder "$STAGING/contents" "$IMAGE" > "$STAGING/hdiutil.log" 2>&1; then
		cat "$STAGING/hdiutil.log"
		mv -f "$IMAGE" "$OUTPUT_DIR/$PACKAGE_NAME.dmg"
		echo "Generated $OUTPUT_DIR/$PACKAGE_NAME.dmg"
		exit 0
	else
		status=$?
	fi
	cat "$STAGING/hdiutil.log" >&2
	if [ "$attempt" -eq 3 ] || ! grep -Fq 'hdiutil: create failed - Resource busy' "$STAGING/hdiutil.log"; then
		exit "$status"
	fi
	echo "Retrying DMG creation after Resource busy (attempt $attempt/3)..." >&2
	rm -f "$IMAGE"
	sleep 2
done
