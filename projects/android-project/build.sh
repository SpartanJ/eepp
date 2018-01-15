#!/bin/sh
adb devices

cd $(dirname "$0")

./assets.sh

./gradlew build

#adb install -r app/build/outputs/apk/app-debug.apk

#ndk-gdb --force --start --verbose
