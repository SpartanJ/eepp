#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --use-frameworks gmake

cd ../../make/macosx/
sed -e "s/-Wl,-x//g" -i .make

make -j`nproc` $@

cd ../../bin/
ln -sf ../libs/macosx/libeepp.dylib .
ln -sf ../libs/macosx/libeepp-debug.dylib .
if [ -f ../libs/macosx/libeepp-maps-debug.dylib ]; then
ln -sf ../libs/macosx/libeepp-maps-debug.dylib .
fi
