#!/bin/sh
cd $(dirname "$0")

USE_ARCH=
if command -v premake4 &> /dev/null
then
    premake4 --file=../../premake4.lua --disable-static-build gmake
elif command -v premake5 &> /dev/null
then
    premake5 --file=../../premake5.lua --disable-static-build gmake
    USE_ARCH=arm64
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd ../../make/macosx/

JOBS=${EEPP_BUILD_JOBS:-$(sysctl -n hw.ncpu 2>/dev/null)}
if ! [ "$JOBS" -gt 0 ] 2>/dev/null; then
    JOBS=$(getconf NPROCESSORS_ONLN 2>/dev/null)
fi
if ! [ "$JOBS" -gt 0 ] 2>/dev/null; then
    echo "Could not determine a safe build job count." >&2
    exit 1
fi

make -j"$JOBS" "$@"
