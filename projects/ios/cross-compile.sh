#!/bin/sh
cd $(dirname "$0")

export TOOLCHAINPATH=/home/apps/ios/bin/
export SYSROOTPATH=/home/apps/ios/Platforms/iPhoneOS5.0.sdk
export IOSVERSION=5.0

premake4 --file=../../premake4.lua --platform=ios-cross-arm7 --with-static-freetype --with-static-eepp --with-gles1 --with-gles2 --with-static-backend gmake 

cd ../../make/ios/
time make $@
