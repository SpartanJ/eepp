#!/bin/sh
CANONPATH=`readlink -f "$0"`
DIRPATH="`dirname "$CANONPATH"`"
cd $DIRPATH
cd ../../../
premake4 --with-mojoal gmake
cd make/linux
make -j$(nproc) config=release ecode
cd $DIRPATH
rm -rf ./ecode.app
mkdir -p ecode.app/assets
mkdir -p ecode.app/libs
chmod +x AppRun
cp AppRun ecode.app/
cp ecode.desktop ecode.app/
cp ../../../bin/assets/icon/ee.png ecode.app/ecode.png
cp ../../../libs/linux/libeepp.so ecode.app/libs/
cp ../../../bin/ecode ecode.app/
cp -L $(whereis libSDL2-2.0.so.0 | awk '{print $NF}') ecode.app/libs/
strip ecode.app/libs/libSDL2-2.0.so.0
#cp -r ../../../bin/assets ecode.app/assets
mkdir -p ecode.app/assets/colorschemes
mkdir -p ecode.app/assets/fonts
cp -r ../../../bin/assets/colorschemes ecode.app/assets/
#cp -r ../../../bin/assets/fonts ecode.app/assets/
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/assets/
cp -r ../../../bin/assets/icon ecode.app/assets/
mkdir ecode.app/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/assets/ui/
appimagetool ecode.app
