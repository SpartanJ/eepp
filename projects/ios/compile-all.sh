#!/bin/sh
cd $(dirname "$0")

./compile-arm64.sh $@
./compile-x86_64.sh $@

cd ../../libs/ios/

if [ -f arm64/libeepp-static-debug.a ] && [ -f x86_64/libeepp-static-debug.a ]; then
lipo -create -arch arm64 arm64/libeepp-static-debug.a -arch x86_64 x86_64/libeepp-static-debug.a -output ./libeepp-debug.a
fi

if [ -f arm7/libeepp-static.a ] && [ -f x86/libeepp-static.a ]; then
lipo -create -arch arm64 arm64/libeepp-static.a -arch x86_64 x86_64/libeepp-static.a -output ./libeepp.a
fi
