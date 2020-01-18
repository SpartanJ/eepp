#!/bin/sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --use-frameworks gmake

cd ../../make/macosx/
sed -e "s/-Wl,-x//g" -i .make

make $@

cd ../../bin/
ln -sf ../libs/macosx/libeepp.dylib .
ln -sf ../libs/macosx/libeepp-debug.dylib .
