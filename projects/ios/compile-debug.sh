#!/bin/sh

cd $(dirname "$0")/../../

rm ./libs/ios/debug/libeepp-armv7.a
rm ./libs/ios/debug/libeepp-i386.a
rm ./libs/ios/debug/libeepp.a

if [ -z $1 ]; then
	if [ "$1" == 'GLES2' ]; then
		BACKEND="GLES2=yes"
	else
		BACKEND="GLES1=yes"
	fi
else
	BACKEND="GLES2=yes"
fi

make -j2 -e IOS=yes NO_SNDFILE=yes STATIC_FT2=yes SIMULATOR=yes DEBUGBUILD=yes $BACKEND

make -j2 -e IOS=yes NO_SNDFILE=yes STATIC_FT2=yes SIMULATOR=no DEBUGBUILD=yes $BACKEND

lipo -create -arch armv7 ./libs/ios/debug/libeepp-armv7.a -arch i386 ./libs/ios/debug/libeepp-i386.a -output ./libs/ios/debug/libeepp.a