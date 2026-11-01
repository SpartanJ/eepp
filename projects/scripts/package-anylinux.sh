#!/bin/bash
# Invoked by the Linux packager after the conventional AppImage has been generated.
set -euo pipefail
APP=$1
STAGING=$2
OUTPUT_DIR=$3
VERSION=$4
ARCH=$5
APPIMAGE_ARCH=$ARCH
[ "$APPIMAGE_ARCH" != arm64 ] || APPIMAGE_ARCH=aarch64

# Sharun passes --argv0 to the bundled loader; glibc added it in 2.33.
# This is a packaging-host requirement, not a requirement on the user's libc.
LIBC_VERSION=$(getconf GNU_LIBC_VERSION 2>/dev/null || true)
if [[ "$LIBC_VERSION" =~ ^glibc\ ([0-9]+)\.([0-9]+) ]]; then
	if (( BASH_REMATCH[1] < 2 || ( BASH_REMATCH[1] == 2 && BASH_REMATCH[2] < 33 ) )); then
		echo "AnyLinux packaging requires glibc >= 2.33 for the loader's --argv0 option (found $LIBC_VERSION)" >&2
		exit 1
	fi
fi

# Isolate downloads and deployment: quick-sharun patches its copies of binaries and
# libraries. Never let those changes reach the conventional AppImage or tarball.
WORK=$(mktemp -d "${TMPDIR:-/tmp}/eepp-anylinux.XXXXXX")
trap 'rm -rf "$WORK"' EXIT
if [ -n "${QUICK_SHARUN:-}" ]; then
	[ -f "$QUICK_SHARUN" ] || { echo "Missing QUICK_SHARUN script: $QUICK_SHARUN" >&2; exit 1; }
else
	QUICK_SHARUN="$WORK/quick-sharun.sh"
	REVISION=bec22efd769b122f1fa9c89606a2d969498a5930
	curl -fL --retry 3 -o "$QUICK_SHARUN" "https://raw.githubusercontent.com/pkgforge-dev/Anylinux-AppImages/$REVISION/useful-tools/quick-sharun.sh"
	echo "fa9dbe2532d568e1c1b8eeaba94d1c57109b776b5541d21b74d3f44b74faf4d3  $QUICK_SHARUN" | sha256sum -c -
fi

export APPDIR="$WORK/AppDir" OUTPATH="$WORK/output"
export OUTNAME="$APP-anylinux-$VERSION-$ARCH.AppImage" VERSION ARCH APPIMAGE_ARCH
export MAIN_BIN="$APP.bin" DESKTOP="$STAGING/$APP.desktop" ICON="$STAGING/$APP.png"
export TMPDIR="$WORK" APPIMAGETOOL="$WORK/appimagetool"
# SDL loads OpenGL dynamically. Collect Mesa explicitly, and trace other dlopen
# dependencies under Xvfb. Library search overrides are confined to deployment;
# the generated launcher uses sharun and does not export LD_LIBRARY_PATH.
export DEPLOY_OPENGL=1 ANYLINUX_LIB=1
# quick-sharun's SDL deployment adds every system libSDL*.so, even when the
# caller supplies its own SDL. Those copies land earlier in sharun's search path
# and can silently downgrade the runtime. Supply SDL ourselves and retain the
# companion libraries that quick-sharun normally adds for SDL's dlopen calls.
export DEPLOY_SDL=0 DEPLOY_PULSE=1 DEPLOY_COMMON_LIBS=1
DEPLOY_INPUTS=("$STAGING/$APP.bin" "$STAGING/libs/"*.so*)
for library in libudev.so.1 libusb-1.0.so.0 libdecor-0.so.0; do
	resolved=$(ldconfig -p 2>/dev/null | awk -v name="$library" '$1 == name && !found {print $NF; found=1}' || true)
	[ ! -f "$resolved" ] || DEPLOY_INPUTS+=("$resolved")
done
mkdir -p "$APPDIR" "$OUTPATH"
cp -R "$STAGING/assets" "$APPDIR/assets"
LD_LIBRARY_PATH="$STAGING/libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
	sh "$QUICK_SHARUN" "${DEPLOY_INPUTS[@]}"

# Resource lookup also works when extracted or when uruntime's mount name differs
# from the traditional .mount_* convention. Both executable layouts see assets.
ln -s ../assets "$APPDIR/bin/assets"
ln -s ../../assets "$APPDIR/shared/bin/assets"
sh "$QUICK_SHARUN" --make-appimage
[ -f "$OUTPATH/$OUTNAME" ] || { echo "AnyLinux image was not produced: $OUTNAME" >&2; exit 1; }
mv "$OUTPATH/$OUTNAME" "$OUTPUT_DIR/$OUTNAME"
echo "Generated $OUTPUT_DIR/$OUTNAME"
