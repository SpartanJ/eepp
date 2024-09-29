#!/bin/sh
cd $(dirname "$0")

if command -v premake4 &> /dev/null
then
    premake4 --file=../../premake4.lua --use-frameworks gmake
elif command -v premake4 &> /dev/null
then
    premake5 --file=../../premake5.lua --use-frameworks gmake2
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd ../../make/macosx/
sed -e "s/-Wl,-x//g" -i .make

make -j$(sysctl -n hw.ncpu) $@

cd ../../bin/
ln -sf ../libs/macosx/libeepp.dylib .
ln -sf ../libs/macosx/libeepp-debug.dylib .
if [ -f ../libs/macosx/libeepp-maps-debug.dylib ]; then
ln -sf ../libs/macosx/libeepp-maps-debug.dylib .
fi
