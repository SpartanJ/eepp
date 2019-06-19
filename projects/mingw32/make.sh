#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --os=windows --platform=mingw32 gmake
cd ../../make/mingw32/
make $@

case "$OSTYPE" in
    darwin*|linux*|freebsd*)
		cd ../../bin/
		ln -sf ../libs/mingw32/eepp.dll eepp.dll
		ln -sf ../libs/mingw32/eepp-debug.dll eepp-debug.dll
		;;
	cygwin*|win*)
		cd ../../libs/mingw32/

		if [ -f eepp.dll ]; then
			cp -f eepp.dll ../../bin/eepp.dll
		fi

		if [ -f eepp-debug.dll ]; then
			cp -f eepp-debug.dll ../../bin/eepp-debug.dll
		fi
		;;
esac
