#!/bin/sh
cd $(dirname "$0")

BACKEND=SDL2
DEBUG_SYMBOLS=
FUZZ_OPTION=
CRYPTO_OPTION=
for arg do
    shift
    case "$arg" in
        --backend=SDL2|--backend=sdl2) BACKEND=SDL2 ;;
        --backend=SDL3|--backend=sdl3) BACKEND=SDL3 ;;
        --backend=*) echo "Unsupported backend: ${arg#*=}" >&2; exit 1 ;;
        --with-debug-symbols) DEBUG_SYMBOLS=--with-debug-symbols ;;
        --with-openssl) CRYPTO_OPTION=--with-openssl ;;
        --with-security-fuzz) FUZZ_OPTION=--with-security-fuzz ;;
        *) set -- "$@" "$arg" ;;
    esac
done

USE_ARCH=
if command -v premake4 &> /dev/null
then
    premake4 --file=../../premake4.lua --disable-static-build --with-backend="$BACKEND" ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS} ${CRYPTO_OPTION:+$CRYPTO_OPTION} ${FUZZ_OPTION:+$FUZZ_OPTION} gmake || exit 1
elif command -v premake5 &> /dev/null
then
    premake5 --file=../../premake5.lua --disable-static-build --with-backend="$BACKEND" ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS} ${CRYPTO_OPTION:+$CRYPTO_OPTION} ${FUZZ_OPTION:+$FUZZ_OPTION} gmake || exit 1
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

if [ -n "$FUZZ_OPTION" ]; then
    # Apple Clang does not ship libFuzzer; callers may select an LLVM installation.
    set -- "$@" "CC=${CC:-clang}" "CXX=${CXX:-clang++}"
fi
make -j"$JOBS" "$@"
