#!/bin/bash
set -euo pipefail
# Called by the platform entry points with the platform as the first argument.
PLATFORM=$1
shift
source "$(cd "$(dirname "$0")" && pwd)/app-package.sh"
package_init "$@"
cd "$PACKAGE_ROOT"
if command -v nproc >/dev/null 2>&1; then
	JOBS=$(nproc)
else
	JOBS=$(getconf NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu)
fi
if ! $SKIP_BUILD; then
	package_premake
	"$PREMAKE" --disable-static-build "--with-backend=$BACKEND" ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS} ${STATIC_CPP:+$STATIC_CPP} gmake
	MAKE=make
	[ "$PLATFORM" != freebsd ] || MAKE=gmake
	"$MAKE" -C "make/$LIB_PLATFORM" -j"$JOBS" "config=$CONFIG_NAME" "$APP"
fi

STAGING="$OUTPUT_DIR/$APP.app"
ARCHIVE_STAGE=
EXTRACTED_TOOL=
trap 'rm -rf "$ARCHIVE_STAGE" "$EXTRACTED_TOOL"' EXIT
rm -rf "$STAGING"
mkdir -p "$STAGING"
package_copy_assets "$STAGING"
LIB_DEST="$STAGING/libs"
[ "$PLATFORM" != haiku ] || LIB_DEST="$STAGING/lib"
mkdir -p "$LIB_DEST"
LIB_SOURCE="$SOURCE_DIR/libeepp.so"
[ -f "$LIB_SOURCE" ] || LIB_SOURCE="$LIB_DIR/libeepp.so"
cp -L "$LIB_SOURCE" "$LIB_DEST/"
if [ "$PLATFORM" = haiku ]; then
	cp "$SOURCE_DIR/$APP" "$STAGING/$APP"
	cp -L /boot/system/lib/libSDL2-2.0.so.0 "$LIB_DEST/"
else
	cp "$SOURCE_DIR/$APP" "$STAGING/$APP.bin"
	cat > "$STAGING/AppRun" <<RUNNER
#!/bin/sh
DIRPATH=\$(CDPATH= cd -- "\$(dirname -- "\$0")" && pwd)
export LD_LIBRARY_PATH="\$DIRPATH/libs\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}"
exec "\$DIRPATH/$APP.bin" "\$@"
RUNNER
	chmod +x "$STAGING/AppRun"
	if [ -f "$PACKAGE_ROOT/projects/$PLATFORM/$APP/$APP.desktop" ]; then
		cp "$PACKAGE_ROOT/projects/$PLATFORM/$APP/$APP.desktop" "$STAGING/"
	else
		cat > "$STAGING/$APP.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=$APP
Exec=$APP
Icon=$APP
Categories=Utility;
DESKTOP
	fi
	cp "$SOURCE_DIR/assets/icon/$APP.png" "$STAGING/$APP.png"
	# Native builds resolve SDL from eepp. Cross builds must use the target toolchain:
	# the host's ldd cannot inspect a foreign executable and may try to run its loader.
	SDL_NAME=libSDL2-2.0.so.0
	[ "$BACKEND" != SDL3 ] || SDL_NAME=libSDL3.so.0
	HOST_ARCH=$(uname -m)
	case "$HOST_ARCH" in aarch64) HOST_ARCH=arm64 ;; amd64) HOST_ARCH=x86_64 ;; i686) HOST_ARCH=x86 ;; esac
	if [ "$PLATFORM" = linux ] && [ "$ARCH" != "$HOST_ARCH" ]; then
		[ -n "${CXX:-${CC:-}}" ] || package_error "Cross packaging requires CXX or CC for target $ARCH"
		SDL_PATH=$("${CXX:-$CC}" "-print-file-name=$SDL_NAME")
	else
		# Drain ldd completely: an early awk exit can cause SIGPIPE on FreeBSD,
		# which pipefail would treat as a dependency lookup failure.
		SDL_PATH=$(ldd "$LIB_SOURCE" | awk -v name="$SDL_NAME" '$1 == name && $2 == "=>" && !found {print $3; found=1}')
	fi
	[ -f "$SDL_PATH" ] || package_error "Cannot resolve $SDL_NAME from libeepp.so"
	cp -L "$SDL_PATH" "$LIB_DEST/$SDL_NAME"
	"${STRIP:-strip}" "$LIB_DEST/$SDL_NAME"
fi

if [ "$PLATFORM" = linux ]; then
	APPIMAGETOOL=${APPIMAGETOOL:-appimagetool}
	if ! command -v "$APPIMAGETOOL" >/dev/null 2>&1; then
		# Preserve the existing self-contained AppImage build workflow.
		cd "$OUTPUT_DIR"
		TOOL="appimagetool-$(uname -m).AppImage"
		[ -f "$TOOL" ] || curl -fL -o "$TOOL" "https://github.com/AppImage/appimagetool/releases/download/continuous/$TOOL"
		chmod +x "$TOOL"
		EXTRACTED_TOOL="$OUTPUT_DIR/squashfs-root"
		"./$TOOL" --appimage-extract
		APPIMAGETOOL="$EXTRACTED_TOOL/AppRun"
	fi
	if [ -n "$DEBUG_SYMBOLS" ]; then
		cp -R "$STAGING" "$OUTPUT_DIR/$APP"
		mv "$OUTPUT_DIR/$APP/AppRun" "$OUTPUT_DIR/$APP/$APP"
		(cd "$OUTPUT_DIR" && 7za a -t7z "$PACKAGE_NAME-with-debug-symbols.7z" "$APP" -mx9 -mmt"$JOBS")
		rm -rf "$OUTPUT_DIR/$APP"
		objcopy -S "$STAGING/$APP.bin" "$STAGING/$APP.bin"
		objcopy -S "$LIB_DEST/libeepp.so" "$LIB_DEST/libeepp.so"
	fi
	# The AppImage architecture environment is consumed by appimagetool.
	ARCH="$ARCH" "$APPIMAGETOOL" "$STAGING" "$OUTPUT_DIR/$PACKAGE_NAME.AppImage"
fi

# Archive with an application-named launcher and top-level directory.
ARCHIVE_STAGE=$(mktemp -d "${TMPDIR:-/tmp}/eepp-archive.XXXXXX")
cp -R "$STAGING" "$ARCHIVE_STAGE/$APP"
if [ "$PLATFORM" != haiku ]; then
	rm -f "$ARCHIVE_STAGE/$APP/.DirIcon"
	mv "$ARCHIVE_STAGE/$APP/AppRun" "$ARCHIVE_STAGE/$APP/$APP"
fi
tar -czf "$OUTPUT_DIR/$PACKAGE_NAME.tar.gz" -C "$ARCHIVE_STAGE" "$APP"
echo "Generated $OUTPUT_DIR/$PACKAGE_NAME.tar.gz"
