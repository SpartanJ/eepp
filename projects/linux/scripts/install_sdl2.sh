#!/bin/bash

# Define SDL version and download URL
SDL_VERSION="2.30.8"
SDL_URL="https://libsdl.org/release/SDL2-${SDL_VERSION}.zip"
ARCH="x86_64" # Default architecture for host

# Function to install dependencies
install_dependencies() {
    sudo apt-get update
    sudo apt-get install -y build-essential unzip wget libasound2-dev libpulse-dev \
                            libudev-dev libx11-dev libxext-dev libxrandr-dev \
                            libxcursor-dev libxi-dev libxinerama-dev libxss-dev \
                            libxkbcommon-dev libwayland-dev wayland-protocols \
                            libegl1-mesa-dev libgbm-dev || exit
}

# Function to install cross-compile dependencies for aarch64
install_cross_dependencies() {
    sudo apt-get install -y gcc-aarch64-linux-gnu g++-aarch64-linux-gnu || exit
}

# Function to configure for cross-compilation
configure_for_aarch64() {
    export CC=aarch64-linux-gnu-gcc
    export CXX=aarch64-linux-gnu-g++
    export AR=aarch64-linux-gnu-ar
    export RANLIB=aarch64-linux-gnu-ranlib
    export STRIP=aarch64-linux-gnu-strip
    ./configure --host=aarch64-linux-gnu --enable-video-x11 --enable-video-wayland --enable-wayland-shared || exit
}

# Parse options
if [ "$1" == "--aarch64" ]; then
    ARCH="aarch64"
    install_cross_dependencies
fi

# Create a temporary directory for the SDL2 build
mkdir -p ./sdl2_build
cd ./sdl2_build || exit

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
    ./configure --enable-video-x11 --enable-video-wayland --enable-wayland-shared || exit
fi

# Build and install SDL2
echo "Building SDL2 for ${ARCH}..."
make -j"$(nproc)"

echo "Installing SDL2..."
sudo make install || exit

# Clean up
echo "Cleaning up..."
rm -rf ./sdl2_build

# Update shared library cache
sudo ldconfig

echo "SDL2 version ${SDL_VERSION} installed successfully for ${ARCH}!"
