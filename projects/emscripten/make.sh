#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --with-gles1 --with-gles2 --with-static-eepp --with-static-freetype --platform=emscripten --with-backend=SDL gmake 
cd ../../make/emscripten/
ln -sf ../../assets/ ./
sed -i 's/-rcs/rcs/g' *.make
emmake make $@
