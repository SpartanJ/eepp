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
cp -R "$BUNDLE" "$STAGING/"
ln -s /Applications "$STAGING/Applications"
hdiutil create -ov -format UDZO -volname "$APP" -srcfolder "$STAGING" "$OUTPUT_DIR/$PACKAGE_NAME.dmg"
echo "Generated $OUTPUT_DIR/$PACKAGE_NAME.dmg"
