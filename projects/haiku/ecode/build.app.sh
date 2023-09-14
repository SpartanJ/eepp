#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit
cd ../../../ || exit
premake5 gmake2
cd make/haiku || exit
make -j"$(nproc)" config=release_x86_64 ecode
cd "$DIRPATH" || exit
rm -rf ./ecode.app
mkdir -p ecode.app/assets
mkdir -p ecode.app/lib
cp ../../../bin/assets/icon/ecode.png ecode.app/ecode.png
cp ../../../libs/haiku/x86_64/libeepp.so ecode.app/lib/
cp ../../../bin/ecode ecode.app/
cp -L /boot/system/lib/libSDL2-2.0.so.0 ecode.app/lib/
strip ecode.app/lib/libSDL2-2.0.so.0
mkdir -p ecode.app/assets/colorschemes
mkdir -p ecode.app/assets/fonts
cp -r ../../../bin/assets/colorschemes ecode.app/assets/
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Bold.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Oblique.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-BoldOblique.ttf ecode/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/codicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DroidSansFallbackFull.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/assets/
mkdir -p ecode.app/assets/icon/
cp -r ../../../bin/assets/icon/ecode.png ecode.app/assets/icon/
mkdir ecode.app/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/assets/ui/

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_NAME=ecode-haiku-"$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"-"$(uname -m)"

mv ecode.app ecode
tar -czf "$ECODE_NAME".tar.gz ecode
mv ecode ecode.app
