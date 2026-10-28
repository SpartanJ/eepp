#!/bin/bash
set -euo pipefail
SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
PLATFORM=macos
source "$SCRIPT_DIR/../scripts/app-package.sh"
package_init "$@"

if ! $SKIP_BUILD; then
	BUILD_ARCH_ARG=
	if [ "$ARCH" != "$(uname -m)" ]; then
		command -v premake5 >/dev/null 2>&1 || package_error "Cross architecture builds require premake5"
		CONFIG_NAME="release_$ARCH"
		LIB_DIR="$PACKAGE_ROOT/libs/macosx/$ARCH"
		BUILD_ARCH_ARG="--arch=$ARCH"
	else
		package_premake
	fi
	# Keep the array nonempty for macOS's Bash 3.2 with nounset enabled.
	BUILD_ARGS=("config=$CONFIG_NAME" "$APP")
	[ -z "$BUILD_ARCH_ARG" ] || BUILD_ARGS+=("$BUILD_ARCH_ARG")
	cd "$PACKAGE_ROOT"
	if $USE_FRAMEWORKS; then
		[ "$BACKEND" = SDL2 ] || package_error "Framework packaging currently supports SDL2 only"
		bash "$SCRIPT_DIR/make.sh" "${BUILD_ARGS[@]}" ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS}
	elif ! bash "$SCRIPT_DIR/make_no_fw.sh" "${BUILD_ARGS[@]}" "--backend=$BACKEND" ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS}; then
		[ "$BACKEND" = SDL2 ] || package_error "SDL3 build failed"
		bash "$SCRIPT_DIR/make.sh" "${BUILD_ARGS[@]}" ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS}
	fi
fi

BUNDLE="$OUTPUT_DIR/$APP.app"
MACOS="$BUNDLE/Contents/MacOS"
RESOURCES="$BUNDLE/Contents/Resources"
BINARY="$SOURCE_DIR/$APP"
EXECUTABLE_DIR=$(dirname "$BINARY")
[ -f "$BINARY" ] || package_error "Missing executable: $BINARY"
lipo "$BINARY" -verify_arch "$ARCH"
case "$BINARY" in "$BUNDLE"/*) package_error "Output bundle contains the source executable: $BINARY" ;; esac
rm -rf "$BUNDLE"
mkdir -p "$MACOS" "$RESOURCES"
package_copy_assets "$RESOURCES"
cp "$BINARY" "$MACOS/$APP"
# Applications use both process-relative and working-directory-relative assets.
# SDL sets the bundle working directory to Resources; the symlink handles the former.
ln -s ../Resources/assets "$MACOS/assets"

PLIST="$BUNDLE/Contents/Info.plist"
if [ -f "$SCRIPT_DIR/$APP/Info.plist.tpl" ]; then
	sed -e "s/APP_VERSION_STRING/$APP_VERSION/g" -e "s/APP_MAJOR_VERSION/$MAJOR/g" \
		-e "s/APP_MINOR_VERSION/$MINOR/g" -e "s/APP_PATCH_LEVEL/$PATCH/g" -e "s/APP_NAME/$APP/g" "$SCRIPT_DIR/$APP/Info.plist.tpl" > "$PLIST"
else
	cat > "$PLIST" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleExecutable</key><string>$APP</string>
<key>CFBundleIdentifier</key><string>dev.ensoft.$APP</string>
<key>CFBundleName</key><string>$APP</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>$APP_VERSION</string>
<key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST
fi
/usr/libexec/PlistBuddy -c "Add :CFBundleVersion string $APP_VERSION" "$PLIST"
if [ -f "$RESOURCES/assets/icon/$APP.icns" ]; then
	cp "$RESOURCES/assets/icon/$APP.icns" "$RESOURCES/$APP.icns"
	/usr/libexec/PlistBuddy -c 'Delete :CFBundleIconFile' "$PLIST" >/dev/null 2>&1 || true
	/usr/libexec/PlistBuddy -c "Add :CFBundleIconFile string $APP.icns" "$PLIST"
fi

# Resolve dependencies from their original locations before rewriting load commands.
# This also bundles SDL3 pulled in by Homebrew's SDL2 compatibility library.
resolve_dependency() {
	local dep=$1 owner=$2 candidate rpath paths rpath_owner
	case "$dep" in
		@loader_path/*) candidate="$(dirname "$owner")/${dep#@loader_path/}" ;;
		@executable_path/*) candidate="$EXECUTABLE_DIR/${dep#@executable_path/}" ;;
		@rpath/*)
			for candidate in "$SOURCE_DIR/${dep#@rpath/}" "$LIB_DIR/${dep#@rpath/}" \
				"/Library/Frameworks/${dep#@rpath/}"; do
				if [ -f "$candidate" ]; then printf '%s\n' "$candidate"; return; fi
			done
			for rpath_owner in "$owner" "$BINARY"; do
				paths=$(otool -l "$rpath_owner" | awk '/cmd LC_RPATH/ {r=1; next} r && $1 == "path" {print $2; r=0}')
				while IFS= read -r rpath; do
					case "$rpath" in
						@loader_path/*) rpath="$(dirname "$rpath_owner")/${rpath#@loader_path/}" ;;
						@executable_path/*) rpath="$EXECUTABLE_DIR/${rpath#@executable_path/}" ;;
					esac
					candidate="$rpath/${dep#@rpath/}"
					if [ -f "$candidate" ]; then printf '%s\n' "$candidate"; return; fi
				done <<< "$paths"
			done
			package_error "Cannot resolve $dep (required by $owner)" ;;
		/*) candidate=$dep ;;
		*)
			for candidate in "$SOURCE_DIR/$dep" "$LIB_DIR/$dep"; do
				if [ -f "$candidate" ]; then printf '%s\n' "$candidate"; return; fi
			done
			package_error "Cannot resolve $dep (required by $owner)" ;;
	esac
	[ -f "$candidate" ] || package_error "Cannot resolve $dep (required by $owner)"
	printf '%s\n' "$candidate"
}

bundle_dependencies() {
	local original=$1 bundled=$2 deps dep resolved name
	deps=$(otool -L "$original")
	while IFS= read -r dep; do
		case "$dep" in /System/Library/*|/usr/lib/*|'') continue ;; esac
		name=$(basename "$dep")
		# otool includes a dylib's own install name before its dependencies.
		[ "$name" != "$(basename "$original")" ] || continue
		resolved=$(resolve_dependency "$dep" "$original")
		if [ ! -f "$MACOS/$name" ]; then
			lipo "$resolved" -verify_arch "$ARCH"
			cp -L "$resolved" "$MACOS/$name"
			chmod u+w "$MACOS/$name"
			install_name_tool -id "@loader_path/$name" "$MACOS/$name"
			bundle_dependencies "$resolved" "$MACOS/$name"
		fi
		install_name_tool -change "$dep" "@loader_path/$name" "$bundled"
	done < <(printf '%s\n' "$deps" | tail -n +2 | awk '{print $1}')
}

bundle_dependencies "$BINARY" "$MACOS/$APP"
chmod -R u+rwX,go+rX,go-w "$BUNDLE"
xattr -cr "$BUNDLE"
# codesign recognizes the main executable as its enclosing app. Sign every bundled
# library first, including unsigned Intel cross-build outputs, then sign the app.
for component in "$MACOS"/*; do
	[ -f "$component" ] && [ "$component" != "$MACOS/$APP" ] || continue
	codesign --force --sign - "$component"
done
codesign --force --sign - "$BUNDLE"
codesign --verify --deep --strict "$BUNDLE"
echo "Generated $BUNDLE"
if $DMG; then
	bash "$SCRIPT_DIR/create.dmg.sh" "$APP" --arch "$ARCH" --version "$VERSION" --output-dir "$OUTPUT_DIR"
fi
