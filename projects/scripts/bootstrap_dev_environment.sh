#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
METADATA_FILE="$ROOT_DIR/.eepp-dev-environment"
DEV_DIR="$ROOT_DIR/.eepp-dev"
ENV_FILE="$DEV_DIR/env.sh"
EXPECTED_COMMIT="${1:-}"

if [ ! -f "$METADATA_FILE" ]; then
	echo "Missing $METADATA_FILE. This script must run from an unpacked eepp development artifact." >&2
	exit 1
fi

commit=""
compiler=""
configuration=""
platform=""
arch=""
sdl3=""
while IFS='=' read -r key value; do
	case "$key" in
		commit) commit="$value" ;;
		compiler) compiler="$value" ;;
		configuration) configuration="$value" ;;
		platform) platform="$value" ;;
		arch) arch="$value" ;;
		sdl3) sdl3="$value" ;;
	esac
done < "$METADATA_FILE"

for key in commit configuration platform arch sdl3; do
	if [ -z "${!key}" ]; then
		echo "Invalid $METADATA_FILE: missing '$key'" >&2
		exit 1
	fi
done

if [ -n "$EXPECTED_COMMIT" ] && [ "$commit" != "$EXPECTED_COMMIT" ]; then
	echo "Development artifact commit mismatch: expected $EXPECTED_COMMIT, got $commit" >&2
	exit 1
fi

if [ "$platform" != "linux" ] || [ "$arch" != "x86_64" ]; then
	echo "Unsupported development artifact: platform=$platform arch=$arch" >&2
	exit 1
fi

config_name="${configuration%%_*}"
obj_dir="$ROOT_DIR/obj/linux/$arch/$config_name"
if [ ! -d "$obj_dir" ]; then
	echo "Missing object directory: $obj_dir" >&2
	exit 1
fi

# The artifact's cached objects include X11 dragging. Require its bundled declarations before
# regenerating or rebuilding so __has_include cannot select a different backend configuration.
for header in Xlib.h Xatom.h keysym.h; do
	if [ ! -f "$DEV_DIR/sysroot/include/X11/$header" ]; then
		echo "Missing bundled X11 header: $header. Download a fresh development artifact." >&2
		exit 1
	fi
done

# Compiler-generated dependency files contain absolute system-header paths from
# the GitHub Actions runner (for example /usr/include/c++/13/...). Those paths
# make GNU Make consider otherwise valid cached objects stale after relocation.
# Remove only /usr dependencies; project dependencies remain relative and keep
# normal incremental header tracking intact.
dep_count=0
while IFS= read -r -d '' dep_file; do
	sed -Ei 's#(^|[[:space:]])/usr/[^[:space:]\\]+##g' "$dep_file"
	dep_count=$((dep_count + 1))
done < <(find "$obj_dir" -type f -name '*.d' -print0)

echo "Made $dep_count dependency files portable."

if [ ! -x "$ROOT_DIR/premake5" ]; then
	echo "Missing bundled premake5 executable: $ROOT_DIR/premake5" >&2
	exit 1
fi

# Generated project files contain absolute paths from the CI workspace in some
# post-build commands. Regenerating them is cheap and keeps the cached objects.
(
	cd "$ROOT_DIR"
	./premake5 --disable-static-build --with-debug-symbols --with-backend=SDL3 gmake
)

sdl_lib_dir="$ROOT_DIR/sdl3_build/SDL3-$sdl3/build"
if [ ! -e "$sdl_lib_dir/libSDL3.so" ]; then
	sdl_link="$(find "$ROOT_DIR/sdl3_build" -type l -name libSDL3.so -print -quit 2>/dev/null || true)"
	if [ -z "$sdl_link" ]; then
		echo "Could not find retained SDL3 $sdl3 build output." >&2
		exit 1
	fi
	sdl_lib_dir="$(dirname "$sdl_link")"
fi

mkdir -p "$DEV_DIR/lib" "$DEV_DIR/include" "$DEV_DIR/sysroot/include"

# The retained SDL build contains the exact headers used in CI. Expose them in
# an SDL3/ include layout so files including <SDL3/SDL.h> compile without a
# system-wide SDL development package.
ln -sfn "$ROOT_DIR/sdl3_build/SDL3-$sdl3/include/SDL3" "$DEV_DIR/include/SDL3"

# Minimal CI/agent images often provide the OpenGL runtime (libGL.so.1) but not
# the development linker symlink (libGL.so). A local shim is enough for -lGL
# without requiring root or package installation.
gl_linker_path="$(cc -print-file-name=libGL.so 2>/dev/null || true)"
if [ -z "$gl_linker_path" ] || [ "$gl_linker_path" = "libGL.so" ] || [ ! -e "$gl_linker_path" ]; then
	gl_runtime_path=""
	if command -v ldconfig >/dev/null 2>&1; then
		gl_runtime_path="$(ldconfig -p 2>/dev/null | awk '$1 == "libGL.so.1" { print $NF; exit }')"
	fi
	if [ -n "$gl_runtime_path" ] && [ -e "$gl_runtime_path" ]; then
		ln -sfn "$gl_runtime_path" "$DEV_DIR/lib/libGL.so"
		echo "Created local libGL.so linker shim -> $gl_runtime_path"
	else
		echo "Warning: libGL.so is unavailable; relinking OpenGL targets may fail." >&2
	fi
fi

{
	printf 'export EEPP_DEV_ROOT=%q\n' "$ROOT_DIR"
	printf 'export EEPP_DEV_SDL_LIB=%q\n' "$sdl_lib_dir"
	printf 'export EEPP_DEV_LIB_DIR=%q\n' "$DEV_DIR/lib"
	printf 'export EEPP_DEV_INCLUDE_DIR=%q\n' "$DEV_DIR/include"
	printf 'export EEPP_DEV_SYSROOT_INCLUDE=%q\n' "$DEV_DIR/sysroot/include"
	cat <<'ENVEOF'
export LD_LIBRARY_PATH="$EEPP_DEV_SDL_LIB:$EEPP_DEV_ROOT/libs/linux/x86_64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBRARY_PATH="$EEPP_DEV_LIB_DIR:$EEPP_DEV_SDL_LIB${LIBRARY_PATH:+:$LIBRARY_PATH}"
export CPATH="$EEPP_DEV_INCLUDE_DIR:$EEPP_DEV_SYSROOT_INCLUDE${CPATH:+:$CPATH}"
export SDL_AUDIODRIVER="${SDL_AUDIODRIVER:-dummy}"
ENVEOF
} > "$ENV_FILE"

echo "Development artifact ready:"
echo "  commit:        $commit"
echo "  compiler:      ${compiler:-unknown}"
echo "  configuration: $configuration"
echo "  SDL3:          $sdl3"
echo ""
echo "Load the build/runtime environment with:"
echo "  source '$ENV_FILE'"
echo ""
echo "Then build incrementally with:"
echo "  make -C '$ROOT_DIR/make/linux' -j\"\$(nproc)\" -e config='$configuration'"
