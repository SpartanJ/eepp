#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit
cd ../../../ || exit
DEBUG_SYMBOLS=
STATIC_CPP=
VERSION=
ARCH=$(arch)
for i in "$@"; do
    case $i in
        --with-debug-symbols)
            DEBUG_SYMBOLS="--with-debug-symbols"
            shift
            ;;
        --with-static-cpp)
            STATIC_CPP="--with-static-cpp"
            shift
            ;;
        --version)
            if [[ -n $2 ]]; then VERSION="$2"; fi
            shift
            shift
            ;;
        --arch)
            if [[ -n $2 ]]; then ARCH="$2"; fi
            shift
            shift
            ;;
        -*)
            echo "Unknown option $i"
            exit 1
            ;;
        *)
            ;;
    esac
done

if [ "$ARCH" = "aarch64" ]; then
    ARCH="arm64"
fi

CONFIG_NAME=
if command -v premake4 &> /dev/null
then
    premake4 $DEBUG_SYMBOLS --with-text-shaper $STATIC_CPP gmake || exit
    CONFIG_NAME=release
elif command -v premake5 &> /dev/null
then
    premake5 $DEBUG_SYMBOLS --with-text-shaper $STATIC_CPP gmake2 || exit
    CONFIG_NAME=release_"$ARCH"
else
    echo "Neither premake5 nor premake4 is available. Please install one."
    exit 1
fi

cd make/linux || exit
make -j"$(nproc)" config="$CONFIG_NAME" ecode || exit
cd "$DIRPATH" || exit
rm -rf ./ecode.app
mkdir -p ecode.app/assets
mkdir -p ecode.app/libs
chmod +x AppRun
cp AppRun ecode.app/
cp ecode.desktop ecode.app/
cp ../../../bin/assets/icon/ecode.png ecode.app/ecode.png
cp ../../../libs/linux/libeepp.so ecode.app/libs/
cp ../../../bin/ecode ecode.app/ecode.bin
cp -L "$(bash ../scripts/find_most_recent_sdl2.sh)" ecode.app/libs/ || exit
${STRIP:-strip} ecode.app/libs/libSDL2-2.0.so.0
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

export APPIMAGETOOL="appimagetool"

if ! command -v appimagetool &> /dev/null
then
    wget -nc "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-$(arch).AppImage"
    chmod +x "./appimagetool-$(arch).AppImage"
    ./appimagetool-"$(arch)".AppImage --appimage-extract
    APPIMAGETOOL="./squashfs-root/AppRun"
fi

ECODE_NAME=ecode-linux-"$ECODE_VERSION"-"$ARCH"

if [ -n "$DEBUG_SYMBOLS" ];
then
    cp -r ecode.app ecode
    rm ecode/.DirIcon
    mv ecode/AppRun ecode/ecode
    7za a -t7z "$ECODE_NAME"-with-debug-symbols.7z ecode -mx9 -mmt"$(nproc)"
    rm -rf ecode
    objcopy -S ecode.app/ecode.bin ecode.app/ecode.bin
    objcopy -S ecode.app/libs/libeepp.so ecode.app/libs/libeepp.so
fi

echo "Generating $ECODE_NAME.AppImage"
$APPIMAGETOOL ecode.app "$ECODE_NAME".AppImage

rm -f ecode.app/.DirIcon
mv ecode.app/AppRun ecode.app/ecode
mv ecode.app ecode
rm -rf ./squashfs-root

echo "Generating $ECODE_NAME.tar.gz"
tar -czf "$ECODE_NAME".tar.gz ecode

mv ecode ecode.app
