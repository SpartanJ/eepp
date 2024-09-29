#!/bin/bash
cd "$(dirname "$0")" || exit

ARCH=32
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
fi

if command -v premake5 &> /dev/null
then
    premake5 --file=../../premake5.lua --os=windows --cc=mingw --windows-mingw-build gmake2
elif [ -f ../../premake5 ]; then
    ../../premake5 --file=../../premake5.lua --os=windows --cc=mingw --windows-mingw-build gmake2
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd ../../make/windows/ || exit

if command -v mingw"$ARCH"-make &> /dev/null
then
  mingw"$ARCH"-make "$@"
else
  export CC=x86_64-w64-mingw32-gcc-posix
  export CXX=x86_64-w64-mingw32-g++-posix
  make "$@"
fi
