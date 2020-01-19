#!/bin/sh
# Currently latest emsdk tested and working version: latest-fastcomp
# remember to first set the environment
# source /path/to/emsdk/emsdk_env.sh
cd $(dirname "$0")
premake4 --file=../../premake4.lua --with-gles1 --with-gles2 --with-static-eepp --platform=emscripten --with-backend=SDL2 gmake
cd ../../make/emscripten/
ln -sf ../../bin/assets/ ./
sed -i 's/-rcs/rcs/g' *.make
emmake make -j`nproc` $@
# Fix a bug in emscripten, see my patch: https://github.com/emscripten-core/emscripten/pull/10208
sed -i 's/function _alGetSource3f(source,/function _alGetSource3f(sourceId,/' ../../bin/*.js
