#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --disable-static-build gmake

cd ../../make/macosx/
sed -e "s/-Wl,-x//g" -i .make

make -j$(sysctl -n hw.ncpu) -e verbose=true $@

cd ../../bin/
ln -sf ../libs/macosx/libeepp.dylib .
ln -sf ../libs/macosx/libeepp-debug.dylib .
if [ -f ../libs/macosx/libeepp-maps-debug.dylib ]; then
ln -sf ../libs/macosx/libeepp-maps-debug.dylib .
fi
