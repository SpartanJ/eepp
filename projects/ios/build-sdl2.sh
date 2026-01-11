#!/bin/sh
cd $(dirname "$0")

SDLVER=$(grep "remote_sdl2_version_number =" ../../premake5.lua | awk '{print $3}' | tr -d '"')

cd ../../src/thirdparty/SDL2-$SDLVER/

if [ ! -f build-ios-device/libSDL2.a ]; then

mkdir -p build-ios-device
cd build-ios-device

cmake -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_SYSROOT=iphoneos \
      -DCMAKE_OSX_ARCHITECTURES=arm64 \
      -DCMAKE_OSX_DEPLOYMENT_TARGET=9.0 \
      -DSDL_STATIC=ON \
      -DSDL_SHARED=OFF \
      ..
cmake --build . --config Release

cd ..

mkdir -p ../../../libs/ios/arm64/

if [ ! -f ../../../libs/ios/libSDL2.a ]; then
cp -f ./build-ios-device/libSDL2.a ../../../libs/ios/libSDL2.a
fi

if [ ! -f ../../../libs/ios/arm64/libSDL2.a ]; then
cp -f ./build-ios-device/libSDL2.a ../../../libs/ios/arm64/libSDL2.a
fi

fi
