#!/bin/bash

# Packages the eepp SDK for Linux, macOS and MinGW (cross).
#
# Usage: ./package_sdk.sh <platform> <arch> [--strip] [--version <version>]
# Examples:
#   ./package_sdk.sh linux x86_64 --strip
#   ./package_sdk.sh macos arm64
#   ./package_sdk.sh windows x86_64 --strip    (MinGW cross build)
#
# --version accepts "nightly" (default) or a stable release tag of the form
# eepp-%d.%d.%d (the tag prefix is stripped from the package names).
#
# Expects the release build to be present in bin/ and libs/ (see the CI workflows).
# Produces projects/<platform>/sdk/eepp-sdk-<platform>-<arch>-nightly.tar.gz
# (or eepp-sdk-windows-<arch>-mingw-nightly.zip for MinGW builds).
#
# Package layout mirrors the repository root:
#   bin/assets + binaries, bin/ runtime libraries, libs/<platform>/<arch>/,
#   include/ and docs/articles/.

set -euo pipefail

if [ $# -lt 2 ]; then
	echo "Usage: $0 <platform> <arch> [--strip] [--version <version>]"
	exit 1
fi

PLATFORM="$1"
ARCH="$2"
STRIP_BINARIES=false
SDK_VERSION="nightly"
while [ $# -gt 2 ]; do
	case "$3" in
		--strip)
			STRIP_BINARIES=true
			;;
		--version)
			SDK_VERSION="${4:-}"
			if [ -z "$SDK_VERSION" ]; then
				echo "Error: --version requires a value" >&2
				exit 1
			fi
			shift
			;;
		-*)
			echo "Unknown option: $3"
			exit 1
			;;
	esac
	shift
done

CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH/../.." || exit 1
ROOT="$PWD"

SDK_DIRNAME="eepp-sdk"
STAGING="projects/$PLATFORM/sdk/$SDK_DIRNAME"

# Repository layout name used for libs/<platform>/<arch>/ (premake's os.target()).
LIBS_PLATFORM="$PLATFORM"
if [ "$PLATFORM" == "macos" ]; then
	LIBS_PLATFORM="macosx"
fi

# Stable release tags use the eepp-%d.%d.%d format; strip the tag prefix for
# the package names. Anything else must be "nightly".
case "$SDK_VERSION" in
	nightly)
		ARCHIVE_SUFFIX="nightly"
		;;
	eepp-*.*)
		if ! [[ "$SDK_VERSION" =~ ^eepp-[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
			echo "Error: invalid version '$SDK_VERSION' (expected nightly or eepp-%d.%d.%d)" >&2
			exit 1
		fi
		ARCHIVE_SUFFIX="${SDK_VERSION#eepp-}"
		;;
	*)
		echo "Error: invalid version '$SDK_VERSION' (expected nightly or eepp-%d.%d.%d)" >&2
		exit 1
		;;
esac

# Strip tool: the MinGW cross build needs the binutils of the target
# toolchain; a native strip cannot handle PE binaries.
if [ "$PLATFORM" == "windows" ]; then
	STRIP_CMD="x86_64-w64-mingw32-strip"
	if ! command -v "$STRIP_CMD" > /dev/null 2>&1; then
		echo "Warning: $STRIP_CMD not found, binaries will not be stripped" >&2
		STRIP_CMD=""
	fi
else
	STRIP_CMD="strip"
fi

rm -rf "projects/$PLATFORM/sdk"
mkdir -p "$STAGING/bin" "$STAGING/libs/$LIBS_PLATFORM/$ARCH" "$STAGING/docs"

# ---------------------------------------------------------------------------
# Assets: everything except the unit-test / benchmark-only asset directories.
# ---------------------------------------------------------------------------

mkdir -p "$STAGING/bin/assets"

# Directories only consumed by unit tests or benchmarks.
TEST_ONLY_ASSET_DIRS=(
	html
	logo
	textfiles
	textformat
)

# Individual files only consumed by unit tests or benchmarks.
TEST_ONLY_ASSET_FILES=(
	test.zip
)

is_test_only() {
	local rel="$1"
	local entry
	for entry in "${TEST_ONLY_ASSET_DIRS[@]}"; do
		if [ "$rel" == "$entry" ]; then
			return 0
		fi
	done
	for entry in "${TEST_ONLY_ASSET_FILES[@]}"; do
		if [ "$rel" == "$entry" ]; then
			return 0
		fi
	done
	return 1
}

for entry in bin/assets/*; do
	rel="${entry#bin/assets/}"
	if is_test_only "$rel"; then
		continue
	fi
	cp -r "$entry" "$STAGING/bin/assets/"
done

# ---------------------------------------------------------------------------
# Binaries: every release executable in bin/ is packaged (any new "eepp-*"
# binary is picked up automatically) plus the fixed tool set: ecode, eterm and
# eeiv. Unit tests and benchmarks live outside bin/ and are never matched.
# ---------------------------------------------------------------------------

# Only accept the executable format of the target platform: prevents stale
# cross-compile artifacts (for example MinGW .exe files lying around in bin/
# of a Linux checkout) from leaking into the wrong package.
is_executable() {
	case "$PLATFORM" in
		macos)
			case "$(file -b "$1")" in
				Mach-O*) return 0 ;;
			esac
			;;
		windows)
			case "$(file -b "$1")" in
				*PE32*) return 0 ;;
			esac
			;;
		*)
			case "$(file -b "$1")" in
				ELF*) return 0 ;;
			esac
			;;
	esac
	return 1
}

copy_binary() {
	local src="$1"

	if [ ! -f "$src" ]; then
		echo "Warning: binary not found, skipping: $src" >&2
		return 0
	fi

	if ! is_executable "$src"; then
		return 0
	fi

	cp "$src" "$STAGING/bin/"
	if [ "$STRIP_BINARIES" = true ] && [ -n "$STRIP_CMD" ]; then
		"$STRIP_CMD" "$STAGING/bin/$(basename "$src")"
	fi
}

for entry in bin/eepp-*; do
	[ -e "$entry" ] || continue
	case "$entry" in
		*-debug | *.dll | *.so | *.dylib | *.a | *.data | *.html | *.js | *.wasm)
			continue
			;;
	esac
	copy_binary "$entry"
done

if [ "$PLATFORM" == "windows" ]; then
	copy_binary bin/ecode.exe
	copy_binary bin/eterm.exe
	copy_binary bin/eeiv.exe
else
	copy_binary bin/ecode
	copy_binary bin/eterm
	copy_binary bin/eeiv
fi

# ---------------------------------------------------------------------------
# Libraries: shared libraries required at runtime plus the static libraries
# useful for linking. Release configurations only ("*-debug*" is skipped).
# ---------------------------------------------------------------------------

LIBS_SRC="libs/$LIBS_PLATFORM/$ARCH"
if [ ! -d "$LIBS_SRC" ]; then
	LIBS_SRC="libs/$LIBS_PLATFORM"
fi

if [ "$PLATFORM" == "macos" ]; then
	SHARED_LIBS=(libeepp.dylib libeepp-maps.dylib libeepp-physics.dylib)
	STATIC_LIBS=(
		libeepp-maps-static.a
		libeepp-physics-static.a
		libeterm.a
		liblanguages-syntax-highlighting.a
	)
elif [ "$PLATFORM" == "windows" ]; then
	SHARED_LIBS=(eepp.dll eepp-maps.dll eepp-physics.dll)
	# MinGW import libraries (libeepp*.a, generated along with the DLLs) plus
	# the statically linked modules and tools.
	STATIC_LIBS=(
		libeepp.a
		libeepp-maps.a
		libeepp-physics.a
		libeepp-maps-static.a
		libeepp-physics-static.a
		libeterm.a
		liblanguages-syntax-highlighting.a
	)
else
	SHARED_LIBS=(libeepp.so libeepp-maps.so libeepp-physics.so)
	STATIC_LIBS=(
		libeepp-maps-static.a
		libeepp-physics-static.a
		libeterm.a
		liblanguages-syntax-highlighting.a
	)
fi

copy_lib() {
	local src="$1"
	local dest_dir="$2"
	local name
	name="$(basename "$src")"

	if [ ! -f "$src" ]; then
		echo "Warning: library not found, skipping: $src" >&2
		return 0
	fi

	cp -L "$src" "$dest_dir/"
	if [ "$STRIP_BINARIES" = true ]; then
		case "$name" in
			*.a)
				# Static archives only tolerate debug-symbol stripping: a full
				# strip corrupts the archive's object members.
				case "$PLATFORM" in
					macos)
						strip -S "$dest_dir/$name" 2>/dev/null || true
						;;
					windows)
						[ -n "$STRIP_CMD" ] && "$STRIP_CMD" -g "$dest_dir/$name" 2>/dev/null || true
						;;
					*)
						"$STRIP_CMD" --strip-debug "$dest_dir/$name" 2>/dev/null || true
						;;
				esac
				;;
			*)
				case "$PLATFORM" in
					macos)
						# cctools strip has no --strip-unneeded; -x keeps local
						# symbols (needed for linking) and drops debug info.
						strip -x "$dest_dir/$name" 2>/dev/null || true
						;;
					*)
						# Shared libraries stay runnable and linkable after
						# removing the unneeded symbols.
						"$STRIP_CMD" --strip-unneeded "$dest_dir/$name" 2>/dev/null || true
						;;
				esac
				;;
		esac
	fi
}

for target in "$STAGING/bin" "$STAGING/libs/$LIBS_PLATFORM/$ARCH"; do
	for lib in "${SHARED_LIBS[@]}"; do
		copy_lib "$LIBS_SRC/$lib" "$target"
	done
done

for lib in "${STATIC_LIBS[@]}"; do
	copy_lib "$LIBS_SRC/$lib" "$STAGING/libs/$LIBS_PLATFORM/$ARCH"
done

# ---------------------------------------------------------------------------
# Headers, documentation and package README.
# ---------------------------------------------------------------------------

cp -r include "$STAGING/include"
cp -r docs/articles "$STAGING/docs/articles"
sed "s/@SDK_VERSION@/$SDK_VERSION/g" projects/scripts/eepp-sdk-README.md > "$STAGING/README.md"

# ---------------------------------------------------------------------------
# macOS: make the package relocatable. The build links against homebrew's
# SDL2 and against libs/<platform>/<arch>/ through absolute or homebrew paths;
# rewrite every dependency to @loader_path and re-sign ad-hoc (mirroring what
# projects/macos/ecode/build.app.sh does for ecode.app).
# ---------------------------------------------------------------------------

fix_macos_ids() {
	local bin_dir="$STAGING/bin"

	if [ "$PLATFORM" != "macos" ]; then
		return 0
	fi

	# Point the shared libraries at their bundled SDL2 dependency.
	for lib in "${SHARED_LIBS[@]}"; do
		[ -f "$bin_dir/$lib" ] || continue
		install_name_tool -change "$(otool -L "$bin_dir/$lib" | awk '/SDL2/ {print $1; exit}')" \
			"@loader_path/libSDL2-2.0.0.dylib" "$bin_dir/$lib" 2>/dev/null || true
	done

	# The executables record the absolute path of the just-built libeepp.dylib;
	# rewrite it (and any other eepp dependency) to its bundled location.
	for exe in "$bin_dir"/*; do
		[ -f "$exe" ] && is_executable "$exe" || continue
		while read -r dep; do
			case "$dep" in
				*@loader_path* | */eepp-sdk/*)
					continue
					;;
			esac
			case "$(basename "$dep")" in
				libeepp.dylib | libeepp-maps.dylib | libeepp-physics.dylib)
					install_name_tool -change "$dep" "@loader_path/$(basename "$dep")" "$exe"
					;;
				libSDL2*.dylib)
					if [ -f "$bin_dir/libSDL2-2.0.0.dylib" ]; then
						install_name_tool -change "$dep" "@loader_path/libSDL2-2.0.0.dylib" "$exe"
					fi
					;;
			esac
		done < <(otool -L "$exe" | tail -n +2 | awk '{print $1}')
		codesign --force --sign - "$exe" 2>/dev/null || true
	done

	for lib in "${SHARED_LIBS[@]}"; do
		[ -f "$bin_dir/$lib" ] || continue
		install_name_tool -id "@loader_path/$lib" "$bin_dir/$lib"
		codesign --force --sign - "$bin_dir/$lib" 2>/dev/null || true
	done

	codesign --force --sign - "$STAGING/libs/$LIBS_PLATFORM/$ARCH"/*.dylib 2>/dev/null || true
}

# ---------------------------------------------------------------------------
# Bundle SDL2 next to the binaries when available. Optional: the package
# README documents the fallback to the system SDL2.
# ---------------------------------------------------------------------------

bundle_sdl2() {
	if [ "$PLATFORM" == "windows" ]; then
		# Premake already drops the SDL2 DLL into bin/ when configured with
		# --windows-mingw-build.
		if [ -f bin/SDL2.dll ]; then
			cp -L bin/SDL2.dll "$STAGING/bin/"
		else
			echo "Warning: bin/SDL2.dll not found, the tools will require it at runtime" >&2
		fi
	elif [ "$PLATFORM" == "macos" ]; then
		SDL2_PATH="$(brew --prefix)/opt/sdl2/lib/libSDL2-2.0.0.dylib"
		if [ -f "$SDL2_PATH" ]; then
			cp -L "$SDL2_PATH" "$STAGING/bin/"
			chmod +w "$STAGING/bin/libSDL2-2.0.0.dylib"
			install_name_tool -id "@loader_path/libSDL2-2.0.0.dylib" "$STAGING/bin/libSDL2-2.0.0.dylib"
			codesign --force --sign - "$STAGING/bin/libSDL2-2.0.0.dylib" 2>/dev/null || true
		fi
	else
		SDL2_PATH="$(ldd "$STAGING/bin/eepp-ui-hello-world" 2>/dev/null | awk '/libSDL[23]/ {print $3; exit}')"
		if [ -n "$SDL2_PATH" ] && [ -f "$SDL2_PATH" ]; then
			cp -L "$SDL2_PATH" "$STAGING/bin/"
		fi
	fi
}

bundle_sdl2
fix_macos_ids

# ---------------------------------------------------------------------------
# Package archive: zip for Windows (keeps the +x bits intact in tar.gz for
# Unix platforms).
# ---------------------------------------------------------------------------

ARCHIVE_BASE="eepp-sdk-$PLATFORM-$ARCH"
if [ "$PLATFORM" == "windows" ]; then
	ARCHIVE_NAME="$ARCHIVE_BASE-mingw-$ARCHIVE_SUFFIX.zip"
	cd "projects/$PLATFORM/sdk"
	zip -qr "$ARCHIVE_NAME" "$SDK_DIRNAME"
	cd "$ROOT"
else
	ARCHIVE_NAME="$ARCHIVE_BASE-$ARCHIVE_SUFFIX.tar.gz"
	cd "projects/$PLATFORM/sdk"
	tar -czf "$ARCHIVE_NAME" "$SDK_DIRNAME"
	cd "$ROOT"
fi

echo "Generated projects/$PLATFORM/sdk/$ARCHIVE_NAME"