#!/bin/sh
cd $(dirname "$0")

CLANGPATH=`xcrun -find -sdk iphoneos clang`
export TOOLCHAINPATH=`dirname $CLANGPATH`/

if [ -z "$IOSVERSION" ]; then
export IOSVERSION=5.0
fi

if [ -z "$SYSROOTPATH" ]; then
export SYSROOTPATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$IOSVERSION.sdk/
fi

premake4 --file=../../premake4.lua --platform=ios-arm7 --with-static-freetype --with-static-eepp --with-gles1 --with-gles2 --with-static-backend gmake 

cd ../../make/ios-arm7/
make $@ eepp-static
