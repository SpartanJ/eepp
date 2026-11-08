#!/bin/bash
# Build the tested LLVM-free x86_64 runtime without changing host packages.
set -euo pipefail
if [ "$#" -ne 5 ]; then
	echo "Usage: $0 APP STAGING OUTPUT_DIR VERSION x86_64" >&2
	exit 1
fi
APP=$1
STAGING=$(cd -- "$2" && pwd)
mkdir -p -- "$3"
OUTPUT_DIR=$(cd -- "$3" && pwd)
VERSION=$4
ARCH=$5
if [ "$ARCH" != x86_64 ] || [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
	echo 'The LLVM-free AnyLinux container requires native Linux x86_64 packaging' >&2
	exit 1
fi
SCRIPTS=$(cd -- "$(dirname -- "$0")" && pwd)
# Content-address the local builder cache so changing either pin rebuilds it.
BUILDER_REVISION=$(cat "$SCRIPTS/anylinux/Dockerfile" "$SCRIPTS/anylinux/mesa-packages.sha256" | sha256sum)
BUILDER="eepp-anylinux-mesa-mini:${BUILDER_REVISION:0:16}"
docker build --tag "$BUILDER" "$SCRIPTS/anylinux"
# Source and conventional staging are read-only: dependency tracing and ELF
# patching must only affect the packager's temporary copies. Match the caller's
# identity so generated images remain writable on local and CI hosts.
docker run --rm --init --user "$(id -u):$(id -g)" \
	-e "UPINFO=${UPINFO:-}" -e I_WANT_FAT_APPIMAGE=0 \
	-v "$SCRIPTS:/scripts:ro" -v "$STAGING:/staging:ro" -v "$OUTPUT_DIR:/output" \
	"$BUILDER" bash /scripts/package-anylinux.sh "$APP" /staging /output "$VERSION" "$ARCH"
