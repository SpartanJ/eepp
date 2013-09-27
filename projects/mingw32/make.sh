#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --os=windows --platform=mingw32 --with-static-freetype gmake
cd ../../make/mingw32/
make $@ -e verbose=yes
cd ../../libs/mingw32/

if [ -f eepp.dll ];
then
	mv eepp.dll ../../
fi

if [ -f eepp-debug.dll ];
then
	mv eepp-debug.dll ../../
fi
