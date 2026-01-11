#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit

ARCH=x86_64
ARCHI=$ARCH
BUILDTYPE=release
VERSION=
for i in "$@"; do
  case $i in
    --version)
      if [[ -n $2 ]]; then VERSION="$2"; fi
      shift
      shift
      ;;
    --arch=*)
      ARCH_CONFIG="${i#*=}"
      shift
      ;;
    --buildtype=*)
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
elif [[ "$ARCH_CONFIG" == "arm64" ]]; then
  ARCH=arm64
  ARCHI=arm64
fi

../make.sh -e config="$BUILDTYPE"_"$ARCH" -j"$(nproc)" ecode || exit

SDLVER=$(grep "remote_sdl2_version_number =" ../../../premake5.lua | awk '{print $3}' | tr -d '"')
bash ../../scripts/copy_ecode_assets.sh ../../bin ecode || exit
cp ../../../bin/ecode.exe ecode/
cp ../../../bin/eepp.dll ecode/

if [[ "$ARCH_CONFIG" == "arm64" ]]; then
cp "/usr/local/cross-tools/aarch64-w64-mingw32/bin/SDL2.dll" ecode/
else
cp ../../../src/thirdparty/"SDL2-$SDLVER"/$ARCHI-w64-mingw32/bin/SDL2.dll ecode/
fi

if [ -n "$VERSION" ];
then
ECODE_VERSION="$VERSION"
else
VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_VERSION="$ECODE_MAJOR_VERSION.$ECODE_MINOR_VERSION.$ECODE_PATCH_LEVEL"
fi

ECODE_ZIP_NAME=ecode-windows-"$ECODE_VERSION"-$ARCH.zip

echo "Generating $ECODE_ZIP_NAME"
zip -r "$ECODE_ZIP_NAME" ecode/
