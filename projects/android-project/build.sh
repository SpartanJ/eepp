#!/bin/sh
adb devices || exit

cd "$(dirname "$0")" || exit

./assets.sh || exit

./gradlew build || exit

adb install -r app/build/outputs/apk/debug/app-debug.apk

#ndk-gdb --force --start --verbose
