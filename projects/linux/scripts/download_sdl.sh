#!/bin/bash
set -euo pipefail

# Use the release asset, which has the same contents as the libsdl.org archive.
SDL_MAJOR="$1"
SDL_VERSION="$2"
DESTINATION="$3"
ARCHIVE="SDL${SDL_MAJOR}-${SDL_VERSION}.zip"
URLS=(
	"https://libsdl.org/release/$ARCHIVE"
	"https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/$ARCHIVE"
)

for url in "${URLS[@]}"; do
	echo "Downloading: $url"
	if wget --timeout=60 --tries=1 "$url" -O "$DESTINATION"; then
		exit 0
	fi
	rm -f "$DESTINATION"
	echo "Failed to download: $url" >&2
done

echo "Failed to download $ARCHIVE from all available sources" >&2
exit 1
