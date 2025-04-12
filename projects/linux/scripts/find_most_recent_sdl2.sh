#!/bin/bash

VERBOSE=
for i in "$@"; do
	case $i in
		--verbose)
			VERBOSE=true
			shift
			;;
		-*)
			echo "Unknown option $i"
			exit 1
			;;
		*)
			;;
	esac
done

lib_paths=$(whereis libSDL2-2.0.so.0 | cut -d ':' -f 2)

get_sdl_version() {
    local lib_path="$1"
    local lib_dir
    lib_dir=$(dirname "$lib_path")

    if [[ -f "$lib_dir/../bin/sdl2-config" ]]; then
        "$lib_dir/../bin/sdl2-config" --version 2>/dev/null
    else
        strings "$lib_path" | grep -Eo "SDL2-[0-9]+\.[0-9]+\.[0-9]+" | head -n 1 | cut -d '-' -f 2
    fi
}

version_to_int() {
    local version="$1"
    local major minor patch
    IFS='.' read -r major minor patch <<< "$version"
    echo $((major * 10000 + minor * 100 + patch))
}

latest_version_int=0
latest_version="0.0.0"
latest_lib=""

for lib in $lib_paths; do
    if [[ -f "$lib" ]]; then
        version=$(get_sdl_version "$lib")
        if [ -n "$VERBOSE" ]; then
            echo "Found version $version in $lib"
        fi
        version_int=$(version_to_int "$version")
        if (( version_int > latest_version_int )); then
            latest_version_int=$version_int
            latest_version="$version"
            latest_lib="$lib"
        fi
    fi
done

if [[ -n "$latest_lib" ]]; then
    if [ -n "$VERBOSE" ]; then
        echo "The newest version is $latest_version in $latest_lib"
    fi
    echo "$latest_lib"
else
    if [ -n "$VERBOSE" ]; then
        echo "No valid SDL2 library found."
    fi
    whereis libSDL2-2.0.so.0 | awk '{print $NF}'
fi
