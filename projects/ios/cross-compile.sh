#!/bin/sh
cd $(dirname "$0")

premake4 --file=../../premake4.lua --platform=ios-cross-arm7 --with-static-freetype --with-static-eepp --with-gles1 --with-gles2 --with-static-backend gmake 

cd ../../src/eepp/helper/SDL2/include
if [ ! -d "SDL2" ]; then
ln -sf . SDL2
fi

cd ../../../../../make/ios/
make $@
