#!/bin/sh
# Currently latest emsdk tested and working version: latest-fastcomp
# remember to first set the environment
# source /path/to/emsdk/emsdk_env.sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --with-emscripten-pthreads --with-gles2 --with-static-eepp --platform=emscripten --with-backend=SDL2 gmake
cd ../../make/emscripten/
ln -sf ../../bin/assets/ ./
emmake make -j`nproc` $@
