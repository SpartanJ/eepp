#!/bin/bash
# brew install create-dmg
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

rm -f ./*arm64.dmg
create-dmg --icon ecode 0 16 --window-size 256 256 --app-drop-link 0 80 --icon-size 32 --text-size 12 --volicon ../../../bin/assets/icon/ecode.icns ecode-macos-"$ECODE_VERSION"-"$(uname -m)".dmg ecode.app
