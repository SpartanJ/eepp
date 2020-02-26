#!/bin/bash
premake5 --file=../../premake5.lua --os=ios --with-static-eepp --with-gles1 --with-gles2 --with-static-backend --use-frameworks xcode4 

cp Info.plist ../../make/ios

if [ ! -f ../../libs/ios/libSDL2.a ]; then

cd ../../src/thirdparty/SDL2-2.0.10/build-scripts
./iosbuild.sh
cp lib/libSDL2.a ../../../../libs/ios/

fi
