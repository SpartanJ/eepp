#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit
cd ../../../ || exit
DEBUG_SYMBOLS=""
for i in "$@"; do
	case $i in
		--with-debug-symbols)
			DEBUG_SYMBOLS="--with-debug-symbols"
			shift
			;;
		-*|--*)
			echo "Unknown option $i"
			exit 1
			;;
		*)
			;;
	esac
done
premake4 "$DEBUG_SYMBOLS" --with-mojoal gmake
cd make/linux || exit
make -j"$(nproc)" config=release ecode
cd "$DIRPATH" || exit
rm -rf ./ecode.app
mkdir -p ecode.app/assets
mkdir -p ecode.app/libs
chmod +x AppRun
cp AppRun ecode.app/
cp ecode.desktop ecode.app/
cp ../../../bin/assets/icon/ee.png ecode.app/ecode.png
cp ../../../libs/linux/libeepp.so ecode.app/libs/
cp ../../../bin/ecode ecode.app/ecode.bin
cp -L "$(whereis libSDL2-2.0.so.0 | awk '{print $NF}')" ecode.app/libs/
strip ecode.app/libs/libSDL2-2.0.so.0
mkdir -p ecode.app/assets/colorschemes
mkdir -p ecode.app/assets/fonts
cp -r ../../../bin/assets/colorschemes ecode.app/assets/
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/codicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/fonts/DroidSansFallbackFull.ttf ecode.app/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/assets/
mkdir -p ecode.app/assets/icon/
cp -r ../../../bin/assets/icon/ee.png ecode.app/assets/icon/
mkdir ecode.app/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/assets/ui/
cp ../../../bin/assets/ca-bundle.pem ecode.app/assets/ca-bundle.pem

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')

export APPIMAGETOOL="appimagetool"

if ! command -v appimagetool &> /dev/null
then
	wget -nc "https://github.com/AppImage/AppImageKit/releases/download/13/appimagetool-$(arch).AppImage"
	APPIMAGETOOL="./appimagetool-$(arch).AppImage"
	chmod +x "$APPIMAGETOOL"
fi

ECODE_NAME=ecode-linux-"$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"-"$(arch)"

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

$APPIMAGETOOL ecode.app "$ECODE_NAME".AppImage

rm ecode.app/.DirIcon
mv ecode.app/AppRun ecode.app/ecode
mv ecode.app ecode

tar -czf "$ECODE_NAME".tar.gz ecode

mv ecode ecode.app

