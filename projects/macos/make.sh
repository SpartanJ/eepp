#!/bin/sh
cd $(dirname "$0")

USE_ARCH=
if command -v premake4 &> /dev/null
then
    premake4 --file=../../premake4.lua --use-frameworks --disable-static-build --with-text-shaper gmake
elif command -v premake5 &> /dev/null
then
    premake5 --file=../../premake5.lua --use-frameworks --disable-static-build --with-text-shaper gmake
    USE_ARCH=arm64/
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd ../../make/macosx/

make -j$(sysctl -n hw.ncpu) $@
