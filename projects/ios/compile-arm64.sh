#!/bin/sh
cd $(dirname "$0")

CLANGPATH=`xcrun -find -sdk iphoneos clang`
export TOOLCHAINPATH=`dirname $CLANGPATH`/

if [ -z "$IOSVERSION" ]; then
export IOSVERSION=`xcrun --sdk iphoneos --show-sdk-version`
fi

if [ -z "$SYSROOTPATH" ]; then
export SYSROOTPATH=`xcrun --sdk iphoneos --show-sdk-platform-path`/Developer/SDKs/iPhoneOS$IOSVERSION.sdk/
fi

premake5 --file=../../premake5.lua --os=ios --with-static-eepp --with-gles1 --with-gles2 --with-static-backend gmake

cd ../../make/ios/
make -j$(sysctl -n hw.ncpu) $@ eepp-static
