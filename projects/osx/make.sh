#!/bin/sh
cd $(dirname "$0")
/usr/local/bin/premake4 --file=../../premake4.lua --with-static-freetype --use-frameworks gmake
cd ../../make/macosx/
make $@
