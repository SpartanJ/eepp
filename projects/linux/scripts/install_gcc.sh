#!/bin/bash

# Exit script on error
set -e

# Specify the GCC version you want to install (e.g., 9, 10, 11)
GCC_VERSION=11

# Add the PPA for newer GCC versions
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update

# Install the specified GCC version and its g++ counterpart
sudo apt-get install -y gcc-$GCC_VERSION g++-$GCC_VERSION

# Update the alternatives to set the specified GCC version as the default
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-$GCC_VERSION 100
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-$GCC_VERSION 100
sudo update-alternatives --install /usr/bin/cc cc /usr/bin/gcc 100
sudo update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-$GCC_VERSION 100

# Confirm installation
gcc --version
g++ --version

echo "GCC $GCC_VERSION installation is complete and set as the default."
