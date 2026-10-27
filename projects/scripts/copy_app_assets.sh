#!/bin/bash
set -euo pipefail
source "$(cd "$(dirname "$0")" && pwd)/app-package.sh"
[ $# -ge 3 ] && [ $# -le 4 ] || package_error "Usage: $0 <app> <source-bin> <destination> [platform]"
APP=$1
[[ "$APP" =~ ^[a-z][a-z0-9_-]*$ ]] || package_error "Invalid application: $APP"
APP_DIR="$PACKAGE_ROOT/src/tools/$APP"
SOURCE_DIR=$(cd "$2" && pwd)
PLATFORM=${4:-common}
LIB_PLATFORM=$PLATFORM
package_copy_assets "$3"
