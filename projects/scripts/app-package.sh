#!/bin/bash
# Shared metadata, CLI and asset handling. Sourced by the platform packagers.
set -euo pipefail
PACKAGE_SCRIPTS=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
PACKAGE_ROOT=$(cd "$PACKAGE_SCRIPTS/../.." && pwd)

package_error() {
	echo "Error: $*" >&2
	exit 1
}

package_init() {
	if [ $# -eq 0 ] || [ "$1" = --help ]; then
		echo "Usage: build.app.sh <app> [--version <label>] [--arch <arch>] [--backend SDL2|SDL3]"
		echo "  [--skip-build] [--source-dir <bin>] [--output-dir <dir>] [--dmg] (macOS)"
		echo "  [--use-frameworks] [--with-debug-symbols] [--with-static-cpp] [--buildtype=release|debug]"
		exit 0
	fi
	APP=$1
	shift
	[[ "$APP" =~ ^[a-z][a-z0-9_-]*$ ]] || package_error "Invalid application name: $APP"
	APP_DIR="$PACKAGE_ROOT/src/tools/$APP"
	[ -f "$APP_DIR/assets.list" ] || package_error "Missing $APP_DIR/assets.list"
	ARCH=$(uname -m)
	VERSION=
	BACKEND=SDL2
	SKIP_BUILD=false
	DMG=false
	USE_FRAMEWORKS=false
	DEBUG_SYMBOLS=
	STATIC_CPP=
	BUILDTYPE=release
	SOURCE_DIR="$PACKAGE_ROOT/bin"
	OUTPUT_DIR="$PACKAGE_ROOT/projects/$PLATFORM/$APP"
	while [ $# -gt 0 ]; do
		case "$1" in
			--version|--arch|--backend|--source-dir|--output-dir)
				[ $# -ge 2 ] && [ -n "$2" ] && [[ "$2" != --* ]] || package_error "Missing value for $1"
				case "$1" in
					--version) VERSION=$2 ;;
					--arch) ARCH=$2 ;;
					--backend) BACKEND=$2 ;;
					--source-dir) SOURCE_DIR=$2 ;;
					--output-dir) OUTPUT_DIR=$2 ;;
				esac
				shift 2 ;;
			--arch=*) ARCH=${1#*=}; shift ;;
			--backend=*) BACKEND=${1#*=}; shift ;;
			--buildtype=*) BUILDTYPE=${1#*=}; shift ;;
			--skip-build) SKIP_BUILD=true; shift ;;
			--dmg) DMG=true; shift ;;
			--use-frameworks) USE_FRAMEWORKS=true; shift ;;
			--with-debug-symbols) DEBUG_SYMBOLS=--with-debug-symbols; shift ;;
			--with-static-cpp) STATIC_CPP=--with-static-cpp; shift ;;
			*) package_error "Unknown argument: $1" ;;
		esac
	done
	case "$ARCH" in aarch64) ARCH=arm64 ;; amd64) ARCH=x86_64 ;; i686) ARCH=x86 ;; esac
	case "$ARCH" in arm64|x86_64|x86) ;; *) package_error "Unsupported architecture: $ARCH" ;; esac
	case "$BACKEND" in SDL2|sdl2) BACKEND=SDL2 ;; SDL3|sdl3) BACKEND=SDL3 ;; *) package_error "Unsupported backend: $BACKEND" ;; esac
	if [ "$PLATFORM" = macos ] && [ "$ARCH" = x86 ]; then
		package_error "macOS packages require arm64 or x86_64"
	fi
	if [ "$PLATFORM" = haiku ] && [ "$BACKEND" != SDL2 ]; then
		package_error "Haiku packages currently require SDL2"
	fi
	case "$BUILDTYPE" in release|debug) ;; *) package_error "Unsupported build type: $BUILDTYPE" ;; esac
	if [ "$PLATFORM" != mingw32 ] && [ "$BUILDTYPE" != release ]; then
		package_error "Debug packages are only supported by the MinGW packager"
	fi
	if [ "$PLATFORM" != macos ] && { $DMG || $USE_FRAMEWORKS; }; then
		package_error "--dmg and --use-frameworks require macOS"
	fi
	PREFIX=$(printf '%s' "$APP" | tr '[:lower:]-' '[:upper:]_')
	MAJOR=$(package_version_part MAJOR_VERSION)
	MINOR=$(package_version_part MINOR_VERSION)
	PATCH=$(package_version_part PATCH_LEVEL)
	APP_VERSION="$MAJOR.$MINOR.$PATCH"
	VERSION=${VERSION:-$APP_VERSION}
	[[ "$VERSION" =~ ^[a-zA-Z0-9][a-zA-Z0-9._+-]*$ ]] || package_error "Invalid version label: $VERSION"
	SOURCE_DIR=$(cd "$SOURCE_DIR" && pwd)
	mkdir -p "$OUTPUT_DIR"
	OUTPUT_DIR=$(cd "$OUTPUT_DIR" && pwd)
	LIB_PLATFORM=$PLATFORM
	case "$PLATFORM" in macos) LIB_PLATFORM=macosx ;; freebsd) LIB_PLATFORM=bsd ;; mingw32) LIB_PLATFORM=windows ;; esac
	LIB_DIR="$PACKAGE_ROOT/libs/$LIB_PLATFORM/$ARCH"
	[ -d "$LIB_DIR" ] || LIB_DIR="$PACKAGE_ROOT/libs/$LIB_PLATFORM"
	PACKAGE_NAME="$APP-${PLATFORM/mingw32/windows}-$VERSION-$ARCH"
}

package_version_part() {
	local value
	value=$(awk -v name="${PREFIX}_$1" '$1 == "#define" && $2 == name {print $3}' "$APP_DIR/version.hpp")
	[[ "$value" =~ ^[0-9]+$ ]] || package_error "Expected integer ${PREFIX}_$1 in $APP_DIR/version.hpp"
	printf '%s' "$value"
}

# An include is relative to the manifest; file paths are relative to SOURCE_DIR.
# Optional source|destination entries allow platform-specific exported files.
package_copy_manifest() {
	local manifest=$1 destination=$2 line source target
	while IFS= read -r line || [ -n "$line" ]; do
		line=${line%$'\r'}
		case "$line" in ''|'#'*) continue ;; '@include '*)
			package_copy_manifest "$(dirname "$manifest")/${line#@include }" "$destination"
			continue ;;
		esac
		source=${line%%|*}
		target=$source
		[[ "$line" != *'|'* ]] || target=${line#*|}
		case "/$source/|/$target/" in *'/../'*|*'//'* ) package_error "Invalid asset path: $line" ;; esac
		[ -e "$SOURCE_DIR/$source" ] || package_error "Missing asset: $SOURCE_DIR/$source"
		mkdir -p "$destination/$(dirname "$target")"
		# Copy directory contents without nesting on repeated/overlay entries.
		if [ -d "$SOURCE_DIR/$source" ]; then
			mkdir -p "$destination/$target"
			cp -R "$SOURCE_DIR/$source/." "$destination/$target/"
		else
			cp -L "$SOURCE_DIR/$source" "$destination/$target"
		fi
	done < "$manifest"
}

package_copy_assets() {
	local asset_platform=$PLATFORM
	[ "$asset_platform" != mingw32 ] || asset_platform=windows
	package_copy_manifest "$APP_DIR/assets.list" "$1"
	if [ -f "$APP_DIR/assets.$asset_platform.list" ]; then
		package_copy_manifest "$APP_DIR/assets.$asset_platform.list" "$1"
	fi
}

package_premake() {
	if command -v premake4 >/dev/null 2>&1; then
		PREMAKE=premake4
		CONFIG_NAME=$BUILDTYPE
		LIB_DIR="$PACKAGE_ROOT/libs/$LIB_PLATFORM"
	elif command -v premake5 >/dev/null 2>&1; then
		PREMAKE=premake5
		CONFIG_NAME="${BUILDTYPE}_$ARCH"
		LIB_DIR="$PACKAGE_ROOT/libs/$LIB_PLATFORM/$ARCH"
	else
		package_error "Neither premake4 nor premake5 is available"
	fi
}
