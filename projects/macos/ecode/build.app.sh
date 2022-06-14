#!/bin/sh
../make_no_fw.sh config=release ecode
rm -rf ./ecode.app
mkdir -p ecode.app/Contents/MacOS/
mkdir -p ecode.app/Contents/Resources/
cp ../../../bin/assets/icon/ee.icns ecode.app/Contents/Resources/ecode.icns
cp Info.plist ecode.app/Contents/
chmod +x run.sh
cp run.sh ecode.app/Contents/MacOS
cp ../../../libs/macosx/libeepp.dylib ecode.app/Contents/MacOS
cp ../../../bin/ecode ecode.app/Contents/MacOS
cp /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib ecode.app/Contents/MacOS
install_name_tool -change /usr/local/opt/sdl2/lib/libSDL2-2.0.0.dylib @executable_path/libSDL2-2.0.0.dylib ecode.app/Contents/MacOS/libeepp.dylib
install_name_tool -change libeepp.dylib @executable_path/libeepp.dylib ecode.app/Contents/MacOS/ecode
#cp -r ../../../bin/assets ecode.app/Contents/MacOS/assets
mkdir -p ecode.app/Contents/MacOS/assets/colorschemes
cp -r ../../../bin/assets/colorschemes/ ecode.app/Contents/MacOS/assets/colorschemes/
#cp -r ../../../bin/assets/fonts ecode.app/Contents/MacOS/assets/
mkdir -p ecode.app/Contents/MacOS/assets/fonts
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/Contents/MacOS/assets/
cp -r ../../../bin/assets/icon ecode.app/Contents/MacOS/assets/
mkdir ecode.app/Contents/MacOS/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/Contents/MacOS/assets/ui/
