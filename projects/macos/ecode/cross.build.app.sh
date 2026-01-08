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

RESOURCES_PATH="ecode.app/Contents/Resources"

premake5 --file=../../../premake5.lua --disable-static-build --use-frameworks gmake || exit

make -C ../../../make/macosx/ -j$(sysctl -n hw.ncpu) -e verbose=true -e config=release_x86_64 ecode || exit

bash ../../scripts/copy_ecode_assets.sh ../../bin $RESOURCES_PATH || exit
mkdir -p ecode.app/Contents/MacOS/
cp ../../../bin/assets/icon/ecode.icns $RESOURCES_PATH/ecode.icns

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

# Clear permissions (basically for libSDL2)
chmod -R u+rwX,go+rX,go-w ecode.app
xattr -cr ecode.app

