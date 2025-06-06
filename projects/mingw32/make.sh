#!/bin/bash
cd "$(dirname "$0")" || exit

ARCH=32
for i in "$@"; do
  case $i in
    config=*)
      CONFIG="${i#*=}"
      ;;
    *)
      ;;
  esac
done

PREMAKE5_ARCH=
if [[ "$CONFIG" == *"x86_64"* ]]; then
  ARCH=64
elif [[ "$CONFIG" == *"arm64"* && "$(uname -m)" == "x86_64" ]]; then
  LLVM_MINGW_V="20241015"
  URL="https://github.com/mstorsjo/llvm-mingw/releases/download/$LLVM_MINGW_V/llvm-mingw-$LLVM_MINGW_V-ucrt-ubuntu-20.04-x86_64.tar.xz"
  FILE_NAME="llvm-mingw-$LLVM_MINGW_V-ucrt-ubuntu-20.04-x86_64"
  TAR_FILE_NAME="$FILE_NAME.tar.xz"
  RENAMED_FOLDER="llvm-mingw"
  BIN_PATH="$(pwd)/$RENAMED_FOLDER/bin"

  if [[ ! -f "$TAR_FILE_NAME" ]]; then
    echo "Downloading $TAR_FILE_NAME..."
    curl -LO "$URL" || { echo "Download failed!"; exit 1; }
  else
    echo "$TAR_FILE_NAME already exists. Skipping download."
  fi

  if [[ ! -d "$RENAMED_FOLDER" ]]; then
    echo "Extracting $TAR_FILE_NAME..."
    tar -xf "$TAR_FILE_NAME" || { echo "Extraction failed!"; exit 1; }
    mv "$FILE_NAME" "$RENAMED_FOLDER"
  else
    echo "$RENAMED_FOLDER directory already exists. Skipping extraction."
  fi


  export PATH="$PATH:$BIN_PATH"
  export CC="aarch64-w64-mingw32-gcc"
  export CXX="aarch64-w64-mingw32-g++"
  export AR="aarch64-w64-mingw32-ar"
  echo "Added $BIN_PATH to PATH."

  PREMAKE5_ARCH="--deps-arch=arm64"
fi

PREMAKE5_ARGS="--file=../../premake5.lua --os=windows --cc=mingw --windows-mingw-build --with-text-shaper $PREMAKE5_ARCH gmake"

if command -v premake5 &> /dev/null
then
    premake5 $PREMAKE5_ARGS
elif [ -f ../../premake5 ]; then
    ../../premake5 $PREMAKE5_ARGS
else
    echo "premake5 is not available. Please install one."
    exit 1
fi

if [[ "$CONFIG" == *"arm64"* ]]; then
bash ./build_sdl2.sh --deps-arch=arm64 || exit 1
else
export CC=x86_64-w64-mingw32-gcc-posix
export CXX=x86_64-w64-mingw32-g++-posix
fi

cd ../../make/windows/ || exit

if command -v mingw"$ARCH"-make &> /dev/null
then
  mingw"$ARCH"-make "$@"
else
  # Use x86_64 or arm64 toolchain based on CONFIG
  make "$@"
fi
