#!/bin/sh
# brew install create-dmg

VERSIONPATH=../../../src/tools/ecode/version.hpp
ECODE_MAJOR_VERSION=$(grep "define ECODE_MAJOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_MINOR_VERSION=$(grep "define ECODE_MINOR_VERSION" $VERSIONPATH | awk '{print $3}')
ECODE_PATCH_LEVEL=$(grep "define ECODE_PATCH_LEVEL" $VERSIONPATH | awk '{print $3}')

rm -f ./*x86_64.dmg
create-dmg --icon ecode 0 16 --window-size 256 256 --app-drop-link 0 80 --icon-size 32 --text-size 12 --volicon ../../../bin/assets/icon/ecode.icns ecode-macos-"$ECODE_MAJOR_VERSION"."$ECODE_MINOR_VERSION"."$ECODE_PATCH_LEVEL"-x86_64.dmg ecode.app
