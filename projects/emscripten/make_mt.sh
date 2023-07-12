#!/bin/sh
# Currently latest emsdk tested and working version: latest-fastcomp
# remember to first set the environment
# source /path/to/emsdk/emsdk_env.sh
cd $(dirname "$0") || exit
unset CPLUS_INCLUDE_PATH
premake4 --file=../../premake4.lua --with-emscripten-pthreads --with-gles2 --with-static-eepp --platform=emscripten --with-backend=SDL2 gmake
cd ../../make/emscripten/ || exit
rm -rf ./assets
cp -r ../../bin/assets/ .
rm assets/fonts/NotoColorEmoji.ttf assets/fonts/DejaVuSansMonoNerdFontComplete.ttf assets/fonts/DroidSansFallbackFull.ttf
rm -rf ./ecode
mkdir ecode
cp -r ../../bin/assets/ ecode/assets/
rm ecode/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode/assets/fonts/DroidSansFallbackFull.ttf ecode/assets/fonts/NotoColorEmoji.ttf ecode/assets/test.zip ecode/assets/ca-bundle.pem ecode/assets/icon/ee.icns ecode/assets/icon/ee.rc ecode/assets/icon/ee.res ecode/assets/icon/ee.ico ecode/assets/fonts/*.png ecode/assets/fonts/*.fnt ecode/assets/fonts/OpenSans-Regular.ttf ecode/assets/icon/ecode.icns ecode/assets/icon/eterm* ecode/assets/icon/*.svg
rm -r ecode/assets/atlases ecode/assets/screenshots ecode/assets/cursors ecode/assets/layouts ecode/assets/maps ecode/assets/sounds ecode/assets/sprites ecode/assets/tiles ecode/assets/shaders ecode/assets/ui/uitheme*
emmake make -j"$(nproc)" "$@"
