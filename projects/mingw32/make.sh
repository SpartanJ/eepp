#!/bin/bash
cd "$(dirname "$0")" || exit

ARCH=32
ARCHI=x86
for i in "$@"; do
  case $i in
    config=*)
      CONFIG="${i#*=}"
      shift
      ;;
    *)
      ;;
  esac
done

if [[ "$CONFIG" == *"x86_64"* ]]; then
  ARCH=64
  ARCHI=x86_64
fi

premake5 --file=../../premake5.lua --os=windows --cc=mingw --with-mojoal --windows-mingw-build gmake2
cd ../../make/windows/ || exit

mingw"$ARCH"-make "$@"

case "$OSTYPE" in
	darwin*|linux*|freebsd*)
		cd ../../bin/ || exit
		ln -sf ../libs/windows/$ARCHI/eepp.dll eepp.dll
		ln -sf ../libs/windows/$ARCHI/eepp-debug.dll eepp-debug.dll
		;;
	cygwin*|win*)
		cd ../../libs/windows/$ARCHI || exit

		if [ -f eepp.dll ]; then
			cp -f eepp.dll ../../bin/eepp.dll
		fi

		if [ -f eepp-debug.dll ]; then
			cp -f eepp-debug.dll ../../bin/eepp-debug.dll
		fi
		;;
esac
