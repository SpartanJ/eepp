#!/bin/sh
cd $(dirname "$0")
export NDK_PROJECT_PATH=$(dirname "$0")
ndk-build -j4
rm -rf ./bin
ant debug
adb install -r bin/EEPPApp-debug.apk
ndk-gdb --force --start
