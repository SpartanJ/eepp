#!/bin/bash
set -euo pipefail

# Use the same SDL3 release as the Windows SDK build.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../.." && pwd)"
SDL_VERSION="$(awk -F '"' '/^remote_sdl3_version_number =/ { print $2; exit }' "$ROOT_DIR/premake5.lua")"
KEEP_BUILD=false
for arg in "$@"; do
	case "$arg" in
		--keep-build) KEEP_BUILD=true ;;
		*) echo "Unknown option: $arg" >&2; exit 1 ;;
	esac
done

sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config unzip wget wayland-protocols \
	libasound2-dev libdbus-1-dev libdecor-0-dev libdrm-dev libegl1-mesa-dev \
	libgbm-dev libgl1-mesa-dev libgles2-mesa-dev libibus-1.0-dev libpipewire-0.3-dev \
	libpulse-dev libsndio-dev libudev-dev libwayland-dev libx11-dev libxcursor-dev \
	libxext-dev libxfixes-dev libxi-dev libxkbcommon-dev libxrandr-dev libxss-dev libxtst-dev

BUILD_DIR="$ROOT_DIR/sdl3_build"
SOURCE_DIR="$BUILD_DIR/SDL3-$SDL_VERSION"
mkdir -p "$BUILD_DIR"
wget "https://libsdl.org/release/SDL3-$SDL_VERSION.zip" -O "$BUILD_DIR/SDL3.zip"
unzip -q -o "$BUILD_DIR/SDL3.zip" -d "$BUILD_DIR"

cmake -S "$SOURCE_DIR" -B "$SOURCE_DIR/build" \
	-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local \
	-DSDL_SHARED=ON -DSDL_STATIC=OFF -DSDL_TEST_LIBRARY=OFF -DSDL_TESTS=OFF \
	-DSDL_X11=ON -DSDL_WAYLAND=ON -DSDL_WAYLAND_SHARED=ON \
	-DSDL_WAYLAND_LIBDECOR=ON -DSDL_WAYLAND_LIBDECOR_SHARED=ON
cmake --build "$SOURCE_DIR/build" --config Release -j"$(nproc)"
sudo cmake --install "$SOURCE_DIR/build" --config Release
sudo ldconfig

rm -f "$BUILD_DIR/SDL3.zip"
if [ "$KEEP_BUILD" = false ]; then
	rm -rf "$BUILD_DIR"
else
	echo "Keeping SDL3 build tree at: $BUILD_DIR"
fi
echo "SDL3 version $SDL_VERSION installed successfully."
