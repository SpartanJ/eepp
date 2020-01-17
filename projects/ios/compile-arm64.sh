#!/bin/sh
cd $(dirname "$0")

CLANGPATH=`xcrun -find -sdk iphoneos clang`
export TOOLCHAINPATH=`dirname $CLANGPATH`/

if [ -z "$IOSVERSION" ]; then
export IOSVERSION=12.1
fi

if [ -z "$SYSROOTPATH" ]; then
export SYSROOTPATH=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS$IOSVERSION.sdk/
fi

premake4 --file=../../premake4.lua --platform=ios-arm64 --with-static-eepp --with-gles1 --with-gles2 --with-static-backend --use-frameworks gmake 

cd ../../make/ios-arm64/
make -j`nproc` $@ eepp-static
