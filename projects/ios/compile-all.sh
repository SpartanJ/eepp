#!/bin/sh
cd $(dirname "$0")

./build-sdl2.sh
./compile-arm64.sh $@
