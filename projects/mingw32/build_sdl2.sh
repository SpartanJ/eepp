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

SDLVER=$(awk -F '"' '/^remote_sdl2_version_number =/ { print $2; exit }' ../../premake5.lua)
CONFIGURE_ARGS=()
if [[ "$ARCH" == "arm64" ]]; then
echo "Building SDL2 for arch $ARCH"
CROSS_PREFIX="/usr/local/cross-tools/aarch64-w64-mingw32"
CONFIGURE_ARGS=(--host=aarch64-w64-mingw32 "--prefix=$CROSS_PREFIX")

if [ -f "$CROSS_PREFIX/bin/SDL2.dll" ] &&
   [ "$(awk '/^Version:/ { print $2; exit }' "$CROSS_PREFIX/lib/pkgconfig/sdl2.pc" 2>/dev/null)" = "$SDLVER" ]; then
echo "SDL2 $SDLVER found in cross-tools folder"
exit 0
fi

else
echo "Building SDL2 for arch $(uname -m)"
fi

echo "SDL2 version $SDLVER"
echo "$PATH"

cd "../../src/thirdparty/SDL2-$SDLVER/" || exit 1

echo "SDL2 running autogen"
./autogen.sh || exit 1

echo "SDL2 running configure"
./configure "${CONFIGURE_ARGS[@]}" || exit 1

echo "SDL2 make..."
sudo PATH="$PATH" make -j"$(nproc)" install || exit 1
