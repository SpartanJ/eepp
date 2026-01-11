#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit
cd ../../../ || exit
premake5 gmake
cd make/haiku || exit
make -j"$(nproc)" config=release_x86_64 ecode
cd "$DIRPATH" || exit
bash ../../scripts/copy_ecode_assets.sh ../../bin ecode.app || exit
mkdir -p ecode.app/lib
cp ../../../bin/assets/icon/ecode.png ecode.app/ecode.png
cp ../../../libs/haiku/x86_64/libeepp.so ecode.app/lib/
cp ../../../bin/ecode ecode.app/
cp -L /boot/system/lib/libSDL2-2.0.so.0 ecode.app/lib/
strip ecode.app/lib/libSDL2-2.0.so.0

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_NAME=ecode-haiku-"$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"-"$(uname -m)"

mv ecode.app ecode
tar -czf "$ECODE_NAME".tar.gz ecode
mv ecode ecode.app
