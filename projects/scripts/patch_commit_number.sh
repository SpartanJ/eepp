#!/bin/bash

cd "$(dirname "$0")" || exit

COMMIT_NUMBER=$(git rev-list "$(git tag --sort=-creatordate | grep ecode | sed -n 1p)"..HEAD --count) || exit

FILE_PATH="../../src/tools/ecode/version.hpp"

if [[ "$OSTYPE" == "darwin"* || "$OSTYPE" == "freebsd"* ]]; then
    sed -i '' "s/#define ECODE_COMMIT_NUMBER [0-9]\+/#define ECODE_COMMIT_NUMBER $COMMIT_NUMBER/" "$FILE_PATH"
else
    sed -i "s/#define ECODE_COMMIT_NUMBER [0-9]\+/#define ECODE_COMMIT_NUMBER $COMMIT_NUMBER/" "$FILE_PATH"
fi
