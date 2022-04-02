#!/bin/sh
cd $(dirname "$0")
premake5 --file=../../premake5.lua --os=windows --cc=mingw --with-mojoal gmake2
cd ../../make/windows/
mingw32-make $@

case "$OSTYPE" in
    darwin*|linux*|freebsd*)
		cd ../../bin/
		ln -sf ../libs/windows/eepp.dll eepp.dll
		ln -sf ../libs/windows/eepp-debug.dll eepp-debug.dll
		;;
	cygwin*|win*)
		cd ../../libs/windows/

		if [ -f eepp.dll ]; then
			cp -f eepp.dll ../../bin/eepp.dll
		fi

		if [ -f eepp-debug.dll ]; then
			cp -f eepp-debug.dll ../../bin/eepp-debug.dll
		fi
		;;
esac
