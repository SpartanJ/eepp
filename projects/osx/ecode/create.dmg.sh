#!/bin/sh
# brew install create-dmg
rm -f ecode.dmg
create-dmg --icon ecode 0 16 --window-size 256 256 --app-drop-link 0 80 --icon-size 32 --text-size 12 --volicon ../../../bin/assets/icon/ee.icns ecode.dmg ecode.app
