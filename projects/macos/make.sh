#!/bin/sh
cd "$(dirname "$0")" || exit 1

CRYPTO_OPTION=
DEBUG_SYMBOLS=
ASAN_OPTION=
BUILD_CONFIG=debug
BUILD_ARCH=
FUZZ_OPTION=
for arg do
    shift
    case "$arg" in
        --arch=arm64|--arch=x86_64) BUILD_ARCH=${arg#*=} ;;
        --arch=*) echo "Unsupported macOS architecture: ${arg#*=}" >&2; exit 1 ;;
        --with-debug-symbols) DEBUG_SYMBOLS=--with-debug-symbols ;;
        --address-sanitizer) ASAN_OPTION=--address-sanitizer ;;
        config=*) BUILD_CONFIG=${arg#*=}; set -- "$@" "$arg" ;;
        --with-openssl) CRYPTO_OPTION=--with-openssl ;;
        --with-security-fuzz) FUZZ_OPTION=--with-security-fuzz ;;
        *) set -- "$@" "$arg" ;;
    esac
done

case "$BUILD_CONFIG" in
    debug*) ./sync_debug_flavour.sh "$ASAN_OPTION" || exit 1 ;;
    *)
        if [ -n "$ASAN_OPTION" ]; then
            echo "--address-sanitizer is only supported in debug builds." >&2
            exit 1
        fi
        ;;
esac

USE_ARCH=
if [ -z "$BUILD_ARCH" ] && command -v premake4 >/dev/null 2>&1
then
    premake4 --file=../../premake4.lua --use-frameworks --disable-static-build ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS} ${ASAN_OPTION:+$ASAN_OPTION} ${CRYPTO_OPTION:+$CRYPTO_OPTION} ${FUZZ_OPTION:+$FUZZ_OPTION} gmake || exit 1
elif command -v premake5 &> /dev/null
then
    premake5 --file=../../premake5.lua --use-frameworks --disable-static-build ${DEBUG_SYMBOLS:+$DEBUG_SYMBOLS} ${ASAN_OPTION:+$ASAN_OPTION} ${CRYPTO_OPTION:+$CRYPTO_OPTION} ${FUZZ_OPTION:+$FUZZ_OPTION} gmake || exit 1
    USE_ARCH=arm64/
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
    set -- "$@" "CC=${CC:-clang}" "CXX=${CXX:-clang++}"
fi
make -j"$JOBS" "$@"
