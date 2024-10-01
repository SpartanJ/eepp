#!/bin/bash

# Define SDL version and download URL
SDL_VERSION="2.30.8"
SDL_URL="https://libsdl.org/release/SDL2-${SDL_VERSION}.zip"

# Install necessary dependencies
sudo apt-get update
sudo apt-get install -y build-essential unzip wget libasound2-dev libpulse-dev libudev-dev libx11-dev libxext-dev libxrandr-dev libxcursor-dev libxi-dev libxinerama-dev libxss-dev libxkbcommon-dev

# Create a temporary directory for the SDL2 build
mkdir -p ./sdl2_build
cd ./sdl2_build || exit

# Download SDL2 source code
echo "Downloading SDL2 version ${SDL_VERSION}..."
wget "${SDL_URL}" -O SDL2.zip || exit

# Unzip the SDL2 source code
echo "Extracting SDL2..."
unzip SDL2.zip || exit

# Enter the SDL2 directory
cd "SDL2-${SDL_VERSION}" || exit

# Configure, build, and install SDL2
echo "Configuring SDL2..."
./configure || exit

echo "Building SDL2..."
make -j"$(nproc)" || exit

echo "Installing SDL2..."
sudo make install || exit

# Clean up
echo "Cleaning up..."
rm -rf ./sdl2_build

# Update shared library cache
sudo ldconfig

echo "SDL2 version ${SDL_VERSION} installed successfully!"
