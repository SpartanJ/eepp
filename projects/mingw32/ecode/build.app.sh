#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit

ARCH=x86_64
ARCHI=$ARCH
BUILDTYPE=release
for i in "$@"; do
  case $i in
    arch=*)
      ARCH_CONFIG="${i#*=}"
      shift
      ;;
    buildtype=*)
      BUILDTYPE_CONFIG="${i#*=}"
      shift
      ;;
    *)
      ;;
  esac
done

if [[ "$BUILDTYPE_CONFIG" == "debug" ]]; then
  BUILDTYPE=debug
fi

if [[ "$ARCH_CONFIG" == "x86" ]]; then
  ARCH=x86
  ARCHI=i686
fi

../make.sh -e config="$BUILDTYPE"_"$ARCH" -j"$(nproc)" ecode

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
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Bold.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Oblique.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-BoldOblique.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/codicon.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DroidSansFallbackFull.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/plugins ecode/assets/
mkdir ecode/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode/assets/ui/
mkdir -p ecode/assets/icon
cp ../../../bin/assets/icon/ecode.png ecode/assets/icon/
cp ../../../bin/assets/ca-bundle.pem ecode/assets/ca-bundle.pem

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_ZIP_NAME=ecode-windows-$ECODE_MAJOR_VERSION.$ECODE_MINOR_VERSION.$ECODE_PATCH_LEVEL-$ARCH.zip

zip -r "$ECODE_ZIP_NAME" ecode/
