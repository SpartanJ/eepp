#!/bin/sh
cd $(dirname "$0")
cd ../../make/windows/
make -j4 -e CC=i686-w64-mingw32-gcc CXX=i686-w64-mingw32-g++ AR=i686-w64-mingw32-ar $@ 
