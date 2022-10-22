#!/bin/sh
cd $(dirname "$0")

CLANGPATH=`xcrun -find -sdk iphonesimulator clang`
export TOOLCHAINPATH=`dirname $CLANGPATH`/

if [ -z "$IOSVERSION" ]; then
export IOSVERSION=`xcrun --sdk iphonesimulator --show-sdk-version`
fi

if [ -z "$SYSROOTPATH" ]; then
export SYSROOTPATH=`xcrun --sdk iphonesimulator --show-sdk-platform-path`/Developer/SDKs/iPhoneSimulator$IOSVERSION.sdk/
fi

premake4 --file=../../premake4.lua --platform=ios-x86_64 --with-static-eepp --with-gles1 --with-gles2 --with-static-backend --use-frameworks gmake 

cd ../../make/ios-x86_64/
make -j`nproc` $@ eepp-static
