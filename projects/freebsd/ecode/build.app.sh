#!/bin/sh
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit
cd ../../../ || exit
VERSION=

while [ $# -gt 0 ]; do
    case "$1" in
        --version)
            if [ -n "$2" ]; then
                VERSION="$2"
                shift
            else
                echo "Error: --version requires an argument." >&2
                exit 1
            fi
            ;;
        --)
            shift
            break
            ;;
        -*)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
        *)
            break
            ;;
    esac
    shift
done

CONFIG_NAME=
if command -v premake5 &> /dev/null
then
    premake5 --with-text-shaper gmake || exit
    CONFIG_NAME=release_x86_64
elif command -v premake4 &> /dev/null
then
    premake4 --with-text-shaper gmake || exit
    CONFIG_NAME=release
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd make/bsd || exit

if command -v nproc &> /dev/null # nproc is only available on FreeBSD 13.2 and later
then
  parallel_tasks=$(nproc)
else
  parallel_tasks=$(getconf NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu)
  # the former is the number of online cpus, the later is the total number of cpus
  # note: if you are on illumos, use psrinfo -n and psrinfo -p instead
fi
gmake -j"${parallel_tasks}" config="${CONFIG_NAME}" ecode || exit

cd "$DIRPATH" || exit
rm -rf ./ecode.app
mkdir -p ecode.app/assets
mkdir -p ecode.app/libs
chmod +x AppRun
cp AppRun ecode.app/
cp ecode.desktop ecode.app/
cp ../../../bin/assets/icon/ecode.png ecode.app/ecode.png
cp ../../../libs/bsd/x86_64/libeepp.so ecode.app/libs/
cp ../../../bin/ecode ecode.app/ecode.bin
cp -L /usr/local/lib/libSDL2-2.0.so.0 ecode.app/libs/
strip ecode.app/libs/libSDL2-2.0.so.0
mkdir -p ecode.app/assets/colorschemes
mkdir -p ecode.app/assets/fonts
mkdir -p ecode.app/assets/i18n
cp -r ../../../bin/assets/i18n ecode.app/assets/
cp -r ../../../bin/assets/colorschemes ecode.app/assets/
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Bold.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Oblique.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-BoldOblique.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/codicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Bold.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Italic.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-BoldItalic.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DroidSansFallbackFull.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/assets/
mkdir -p ecode.app/assets/icon/
cp -r ../../../bin/assets/icon/ecode.png ecode.app/assets/icon/
mkdir ecode.app/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/assets/ui/
cp ../../../bin/assets/ca-bundle.pem ecode.app/assets/ca-bundle.pem

if [ -n "$VERSION" ];
then
ECODE_VERSION="$VERSION"
else
VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')
ECODE_VERSION="$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"
fi

ECODE_NAME=ecode-freebsd-"$ECODE_VERSION"-x86_64

mv ecode.app/AppRun ecode.app/ecode
mv ecode.app ecode

echo "Generating $ECODE_NAME.tar.gz"
tar -czf "$ECODE_NAME".tar.gz ecode

mv ecode ecode.app
