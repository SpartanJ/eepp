#!/bin/sh
set -e

# Currently tested emsdk version: 4.0.1
# Remember to first set the environment:
# source /path/to/emsdk/emsdk_env.sh
cd "$(dirname "$0")"
unset CPLUS_INCLUDE_PATH

PREMAKE5=${PREMAKE5:-premake5}
if [ -x ../../premake5 ]; then
	PREMAKE5=../../premake5
fi

"$PREMAKE5" --file=../../premake5.lua --os=emscripten --with-gles2 --with-static-eepp --with-backend=SDL2 gmake
cd ../../make/emscripten/
rm -rf ./assets
cp -r ../../bin/assets/ .
rm -f assets/fonts/NotoColorEmoji.ttf assets/fonts/DejaVuSansMonoNerdFontComplete.ttf assets/fonts/DroidSansFallbackFull.ttf
rm -rf ./ecode
mkdir ecode
cp -r ../../bin/assets/ ecode/assets/
rm -f ecode/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode/assets/fonts/DroidSansFallbackFull.ttf ecode/assets/fonts/NotoColorEmoji.ttf ecode/assets/test.zip ecode/assets/ca-bundle.pem ecode/assets/icon/ee.icns ecode/assets/icon/ee.rc ecode/assets/icon/ee.res ecode/assets/icon/ee.ico ecode/assets/fonts/*.png ecode/assets/fonts/*.fnt ecode/assets/fonts/OpenSans-Regular.ttf ecode/assets/icon/ecode.icns ecode/assets/icon/eterm* ecode/assets/icon/*.svg
rm -rf ecode/assets/atlases ecode/assets/screenshots ecode/assets/cursors ecode/assets/layouts ecode/assets/maps ecode/assets/sounds ecode/assets/sprites ecode/assets/tiles ecode/assets/shaders ecode/assets/ui/uitheme*
emmake make -j"$(nproc)" "$@"
