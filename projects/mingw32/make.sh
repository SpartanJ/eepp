#!/bin/bash
cd "$(dirname "$0")" || exit

ARCH=32
for i in "$@"; do
  case $i in
    config=*)
      CONFIG="${i#*=}"
      shift
      ;;
    *)
      ;;
  esac
done

if [[ "$CONFIG" == *"x86_64"* ]]; then
  ARCH=64
fi

premake5 --file=../../premake5.lua --os=windows --cc=mingw --windows-mingw-build gmake2
cd ../../make/windows/ || exit

mingw"$ARCH"-make "$@"
