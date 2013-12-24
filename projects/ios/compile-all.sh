#!/bin/sh
cd $(dirname "$0")

./compile-arm7.sh $1 $2 $3 $4 $5
./compile-x86.sh $1 $2 $3 $4 $5

cd ../../libs/ios/

if [ -f arm7/libeepp-static-debug.a ] && [ -f x86/libeepp-static-debug.a ]; then
lipo -create -arch armv7 arm7/libeepp-static-debug.a -arch i386 x86/libeepp-static-debug.a -output ./libeepp-debug.a
fi

if [ -f arm7/libeepp-static.a ] && [ -f x86/libeepp-static.a ]; then
lipo -create -arch armv7 arm7/libeepp-static.a -arch i386 x86/libeepp-static.a -output ./libeepp.a
fi