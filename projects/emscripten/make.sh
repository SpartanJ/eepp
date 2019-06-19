#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --with-gles1 --with-gles2 --with-static-eepp --platform=emscripten --with-backend=SDL2 gmake 
cd ../../make/emscripten/
ln -sf ../../bin/assets/ ./
sed -i 's/-rcs/rcs/g' *.make
emmake make -j`nproc` $@
