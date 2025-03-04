#!/bin/sh
cd $(dirname "$0")

USE_ARCH=
if command -v premake4 &> /dev/null
then
    premake4 --file=../../premake4.lua --use-frameworks --disable-static-build gmake
elif command -v premake5 &> /dev/null
then
    premake5 --file=../../premake5.lua --use-frameworks --disable-static-build gmake2
    USE_ARCH=arm64/
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd ../../make/macosx/

make -j$(sysctl -n hw.ncpu) $@

cd ../../bin/
ln -sf ../libs/macosx/"$USE_ARCH"libeepp.dylib .
ln -sf ../libs/macosx/"$USE_ARCH"libeepp-debug.dylib .
if [ -f ../libs/macosx/"$USE_ARCH"libeepp-maps-debug.dylib ]; then
ln -sf ../libs/macosx/"$USE_ARCH"libeepp-maps-debug.dylib .
fi
