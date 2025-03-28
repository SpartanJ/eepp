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
echo "Building SDL2 for arch $ARCH"
HOST="--host=aarch64-w64-mingw32"

if [ ! -f "/usr/local/cross-tools/aarch64-w64-mingw32/bin/SDL2.dll" ]; then
echo "SDL2 found in cross-tools folder"
# exit 0
fi

else
echo "Building SDL2 for arch $(uname -m)"
HOST=
fi

SDLVER=$(grep "remote_sdl2_version_number =" ../../premake5.lua | awk '{print $3}' | tr -d '"')

echo "SDL2 version $SDLVER"
echo "$PATH"

cd "../../src/thirdparty/SDL2-$SDLVER/" || exit 1

echo "SDL2 running autogen"
./autogen.sh || exit 1

echo "SDL2 running configure"
./configure $HOST || exit 1

echo "SDL2 make..."
sudo PATH="$PATH" make -j"$(nproc)" install || exit 1
