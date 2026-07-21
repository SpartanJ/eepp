#!/bin/bash
cd "$(dirname "$0")" || exit

ARCH=x86_64
for i in "$@"; do
  case $i in
    --arch=*)
      ARCH="${i#*=}"
      shift
      ;;
    *)
      ;;
  esac
done

uname -a

if [[ "$ARCH" == "arm64" ]]; then
echo "Building SDL3 for arch $ARCH"
CROSS_PREFIX="/usr/local/cross-tools/aarch64-w64-mingw32"

if [ -f "$CROSS_PREFIX/bin/SDL3.dll" ]; then
echo "SDL3 found in cross-tools folder"
exit 0
fi

else
echo "Building SDL3 for arch $(uname -m)"
CROSS_PREFIX=
fi

SDLVER=$(grep "remote_sdl3_version_number =" ../../premake5.lua | awk '{print $3}' | tr -d '"')

echo "SDL3 version $SDLVER"
echo "$PATH"

cd "../../src/thirdparty/SDL3-$SDLVER/" || exit 1

if [[ "$ARCH" == "arm64" ]]; then
echo "SDL3 running cmake for arm64 cross-compile"
cmake -S . -B build \
	-DCMAKE_SYSTEM_NAME=Windows \
	-DCMAKE_SYSTEM_PROCESSOR=ARM64 \
	-DCMAKE_C_COMPILER=aarch64-w64-mingw32-gcc \
	-DCMAKE_CXX_COMPILER=aarch64-w64-mingw32-g++ \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$CROSS_PREFIX" \
	-DSDL_DISABLE_UNIX=ON || exit 1

echo "SDL3 make..."
cmake --build build --config Release -j"$(nproc)" || exit 1

echo "SDL3 install..."
sudo cmake --install build || exit 1
else
echo "SDL3 running cmake"
cmake -S . -B build \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=/usr/local || exit 1

echo "SDL3 make..."
cmake --build build --config Release -j"$(nproc)" || exit 1

echo "SDL3 install..."
sudo cmake --install build || exit 1
fi
