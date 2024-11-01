#!/bin/bash
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"
cd "$DIRPATH" || exit

VERSION=
for i in "$@"; do
	case $i in
		--version)
			if [[ -n $2 ]]; then VERSION="$2"; fi
			shift
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

premake5 --file=../../../premake5.lua --use-frameworks gmake2 || exit

make -C ../../../make/macosx/ -j$(sysctl -n hw.ncpu) -e verbose=true -e config=release_x86_64 ecode || exit
rm -rf ./ecode.app
mkdir -p ecode.app/Contents/MacOS/
mkdir -p ecode.app/Contents/Resources/
cp ../../../bin/assets/icon/ecode.icns ecode.app/Contents/Resources/ecode.icns

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')

if [ -n "$VERSION" ];
then
ECODE_VERSION="$VERSION"
else
ECODE_VERSION="$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"
fi

cat Info.plist.tpl | sed "s/ECODE_VERSION_STRING/${ECODE_VERSION}/g" | sed "s/ECODE_MAJOR_VERSION/${ECODE_MAJOR_VERSION}/g"  | sed "s/ECODE_MINOR_VERSION/${ECODE_MINOR_VERSION}/g" > Info.plist

cp Info.plist ecode.app/Contents/
rm Info.plist
cp ../../../libs/macosx/x86_64/libeepp.dylib ecode.app/Contents/MacOS
cp ../../../bin/ecode ecode.app/Contents/MacOS
SDL2_LIB_PATH="/Library/Frameworks/SDL2.framework/Versions/A/"
cp "$SDL2_LIB_PATH/SDL2" ecode.app/Contents/MacOS/SDL2
install_name_tool -change @rpath/SDL2.framework/Versions/A/SDL2 @executable_path/SDL2 ecode.app/Contents/MacOS/libeepp.dylib
codesign --force -s - ecode.app/Contents/MacOS/SDL2
install_name_tool -change @rpath/libeepp.dylib @executable_path/libeepp.dylib ecode.app/Contents/MacOS/ecode

#cp -r ../../../bin/assets ecode.app/Contents/MacOS/assets
mkdir -p ecode.app/Contents/MacOS/assets/colorschemes
cp -r ../../../bin/assets/colorschemes/ ecode.app/Contents/MacOS/assets/colorschemes/
#cp -r ../../../bin/assets/fonts ecode.app/Contents/MacOS/assets/
mkdir -p ecode.app/Contents/MacOS/assets/fonts
cp -r ../../../bin/assets/fonts/DejaVuSansMono.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Bold.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-Oblique.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMono-BoldOblique.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/DejaVuSansMonoNerdFontComplete.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/nonicons.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/codicon.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Regular.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/remixicon.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoEmoji-Regular.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Bold.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-Italic.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoSans-BoldItalic.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/NotoColorEmoji.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/fonts/DroidSansFallbackFull.ttf ecode.app/Contents/MacOS/assets/fonts/
cp -r ../../../bin/assets/plugins ecode.app/Contents/MacOS/assets/
# cp -r ../../../bin/assets/icon ecode.app/Contents/MacOS/assets/
mkdir -p ecode.app/Contents/MacOS/assets/icon
cp ../../../bin/assets/icon/ecode.png ecode.app/Contents/MacOS/assets/icon
cp ../../../bin/assets/ca-bundle.pem ecode.app/Contents/MacOS/assets/ca-bundle.pem
mkdir ecode.app/Contents/MacOS/assets/ui
cp ../../../bin/assets/ui/breeze.css ecode.app/Contents/MacOS/assets/ui/

# Clear permissions (basically for libSDL2)
chmod -R u+rwX,go+rX,go-w ecode.app
xattr -cr ecode.app

