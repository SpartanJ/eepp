#!/bin/sh
cd $(dirname "$0")

premake4 --file=../../premake4.lua --platform=ios-cross-x86 --with-static-freetype --with-static-eepp --with-gles1 --with-gles2 --with-static-backend gmake 

cd ../../make/ios-cross-x86/
make $@
