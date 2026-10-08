#!/bin/bash
set -euo pipefail

# Define SDL version and download URL
SDL_VERSION="2.32.10"
SDL_URL="https://libsdl.org/release/SDL2-${SDL_VERSION}.zip"
ARCH="x86_64" # Default architecture for host
KEEP_BUILD=false

for arg in "$@"; do
	case "$arg" in
		--aarch64)
			ARCH="aarch64"
			;;
		--keep-build)
			KEEP_BUILD=true
			;;
		*)
			echo "Unknown option: $arg" >&2
			exit 1
			;;
	esac
done

# Function to install dependencies
install_dependencies() {
    sudo apt-get update
    sudo apt-get install -y build-essential unzip wget wayland-protocols \
                            libasound2-dev \
                            libdbus-1-dev \
                            libegl1-mesa-dev \
                            libgl1-mesa-dev \
                            libgles2-mesa-dev \
                            libglu1-mesa-dev \
                            libibus-1.0-dev \
                            libpulse-dev \
                            libsndio-dev \
                            libudev-dev \
                            libwayland-dev \
                            libx11-dev \
                            libxcursor-dev \
                            libxext-dev \
                            libxi-dev \
                            libxinerama-dev \
                            libxkbcommon-dev \
                            libxrandr-dev \
                            libxss-dev \
                            libxt-dev \
                            libxv-dev \
                            libxxf86vm-dev || exit
}

# Function to install cross-compile dependencies for aarch64
install_cross_dependencies() {
    sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu unzip wget wayland-protocols || exit
    sudo apt-get install -y libasound2-dev:arm64 \
                            libdbus-1-dev:arm64 \
                            libegl1-mesa-dev:arm64 \
                            libgl1-mesa-dev:arm64 \
                            libgles2-mesa-dev:arm64 \
                            libglu1-mesa-dev:arm64 \
                            libibus-1.0-dev:arm64 \
                            libpulse-dev:arm64 \
                            libsndio-dev:arm64 \
                            libudev-dev:arm64 \
                            libwayland-dev:arm64 \
                            libx11-dev:arm64 \
                            libxcursor-dev:arm64 \
                            libxext-dev:arm64 \
                            libxi-dev:arm64 \
                            libxinerama-dev:arm64 \
                            libxkbcommon-dev:arm64 \
                            libxrandr-dev:arm64 \
                            libxss-dev:arm64 \
                            libxt-dev:arm64 \
                            libxv-dev:arm64 \
                            libxxf86vm-dev:arm64 \
                            libdecor-0-dev:arm64 || exit
}

# Function to configure for cross-compilation
configure_for_aarch64() {
    export CC=aarch64-linux-gnu-gcc
    export CXX=aarch64-linux-gnu-g++
    export AR=aarch64-linux-gnu-ar
    export RANLIB=aarch64-linux-gnu-ranlib
    export STRIP=aarch64-linux-gnu-strip
    ./configure --host=aarch64-linux-gnu --prefix=/usr --enable-video-x11 --enable-video-wayland --enable-wayland-shared --enable-libdecor --enable-libdecor-shared || exit
}

# Parse options
if [ "$ARCH" == "aarch64" ]; then
    install_cross_dependencies
fi

# Create a temporary directory for the SDL2 build
BUILD_DIR="$(pwd)/sdl2_build"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit

# Install dependencies
install_dependencies

# Download SDL2 source code
echo "Downloading SDL2 version ${SDL_VERSION}..."
wget "${SDL_URL}" -O SDL2.zip || exit

# Unzip the SDL2 source code
echo "Extracting SDL2..."
unzip SDL2.zip || exit

# Enter the SDL2 directory
cd "SDL2-${SDL_VERSION}" || exit

# Configure the build (with optional cross-compilation)
if [ "$ARCH" == "aarch64" ]; then
    echo "Configuring SDL2 for aarch64 cross-compilation..."
    configure_for_aarch64
else
    echo "Configuring SDL2 for host system..."
    ./configure --enable-video-x11 --enable-video-wayland --enable-wayland-shared --enable-libdecor --enable-libdecor-shared || exit
fi

# Build and install SDL2
echo "Building SDL2 for ${ARCH}..."
make -j"$(nproc)"

echo "Installing SDL2..."
sudo make install || exit

# Clean up
# Update shared library cache
sudo ldconfig

# The zip is redundant once the source has been extracted.
rm -f "$BUILD_DIR/SDL2.zip"

if [ "$KEEP_BUILD" = false ]; then
    echo "Cleaning up..."
    cd "$BUILD_DIR/.." || exit
    rm -rf "$BUILD_DIR"
else
    echo "Keeping SDL2 build tree at: $BUILD_DIR"
fi

echo "SDL2 version ${SDL_VERSION} installed successfully for ${ARCH}!"
