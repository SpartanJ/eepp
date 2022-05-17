#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua gmake

cd ../../make/macosx/
sed -e "s/-Wl,-x//g" -i .make

make -j`nproc` $@

cd ../../bin/
ln -sf ../libs/macosx/libeepp.dylib .
ln -sf ../libs/macosx/libeepp-debug.dylib .
