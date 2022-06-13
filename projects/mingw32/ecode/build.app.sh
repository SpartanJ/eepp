#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit

ARCH=x86
ARCHI=i686
for i in "$@"; do
  case $i in
    config=*)
      ARCH="${i#*=}"
      shift
      ;;
    *)
      ;;
  esac
done

if [[ "$ARCH" == *"x86_64"* ]]; then
  ARCH=x86_64
  ARCHI=$ARCH
fi

../make.sh -e config=release_"$ARCH" -j"$(nproc)" ecode

SDLVER=$(grep "remote_sdl2_version =" ../../../premake5.lua | awk '{print $3}' | tr -d '"')
rm -rf ./ecode
mkdir -p ecode/assets
cp ../../../bin/ecode.exe ecode/
cp ../../../bin/eepp.dll ecode/
cp ../../../src/thirdparty/"$SDLVER"/$ARCHI-w64-mingw32/bin/SDL2.dll ecode/
mkdir -p ecode/assets/colorschemes
mkdir -p ecode/assets/fonts
cp -r ../../../bin/assets/colorschemes ecode/assets/
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/plugins ecode/assets/
mkdir ecode/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode/assets/ui/

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_ZIP_NAME=ecode-$ECODE_MAJOR_VERSION.$ECODE_MINOR_VERSION.$ECODE_PATCH_LEVEL-$ARCH.zip

zip -r "$ECODE_ZIP_NAME" ecode/