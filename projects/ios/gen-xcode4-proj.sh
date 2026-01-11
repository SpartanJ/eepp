#!/bin/sh
premake5 --file=../../premake5.lua --os=ios --with-static-eepp --with-gles1 --with-gles2 --with-static-backend xcode4

cp Info.plist ../../make/ios

sh ./build-sdl2.sh
