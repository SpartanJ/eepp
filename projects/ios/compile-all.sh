#!/bin/sh
cd $(dirname "$0") || exit

./compile-arm64.sh $@
./build-sdl2.sh
