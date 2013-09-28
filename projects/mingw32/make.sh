#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --os=windows --platform=mingw32 --with-static-freetype gmake
cd ../../make/mingw32/
make $@
cd ../../libs/mingw32/

if [ -f eepp.dll ];
then
	cp -f eepp.dll ../../eepp.dll
fi

if [ -f eepp-debug.dll ];
then
	cp -f eepp-debug.dll ../../eepp-debug.dll
fi
