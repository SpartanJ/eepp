#!/bin/sh
cd $(dirname "$0")
cd ../../make/mingw32/
premake4 --file=../../premake4.lua --os=windows --platform=mingw32 --with-static-eepp --with-static-freetype gmake
time make $@
