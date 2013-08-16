#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --os=windows --platform=mingw32 --with-static-freetype gmake
cd ../../make/mingw32/
time make $@ -e verbose=yes
cd ../../libs/mingw32/
mv eepp.dll ../../
mv eepp-debug.dll ../../
