#!/bin/sh

cd $(dirname "$0")/../

rm libeepp-armv7.a

rm libeepp-i386.a

rm libeepp.a

make -j2 -e IOS=yes NO_SNDFILE=yes STATIC_FT2=yes SIMULATOR=yes DEBUGBUILD=no GLES1=yes

make -j2 -e IOS=yes NO_SNDFILE=yes STATIC_FT2=yes SIMULATOR=no DEBUGBUILD=no GLES1=yes

lipo -create -arch armv7 libeepp-armv7.a -arch i386 libeepp-i386.a -output libeepp.a