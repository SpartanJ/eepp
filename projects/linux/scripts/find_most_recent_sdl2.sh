#!/bin/bash

VERBOSE=
DESIRED_ARCH=

for i in "$@"; do
    case $i in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --arch=*)
            DESIRED_ARCH="${i#*=}"
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

# Determine current architecture if no --arch specified
if [[ -z "$DESIRED_ARCH" ]]; then
    CURRENT_MACHINE=$(uname -m)
    case "$CURRENT_MACHINE" in
        x86_64|amd64)
            DESIRED_ARCH="x86-64"
            ;;
        i386|i686)
            DESIRED_ARCH="80386"
            ;;
        aarch64|arm64)
            DESIRED_ARCH="aarch64"
            ;;
        arm*)
            DESIRED_ARCH="ARM"
            ;;
        *)
            echo "Unsupported current architecture: $CURRENT_MACHINE"
            exit 1
            ;;
    esac
    if [[ -n "$VERBOSE" ]]; then
        echo "No architecture specified; using current: $DESIRED_ARCH"
    fi
fi

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
    # Default missing parts to 0
    major=${major:-0}
    minor=${minor:-0}
    patch=${patch:-0}
    echo $((major * 10000 + minor * 100 + patch))
}

latest_version_int=0
latest_version="0.0.0"
latest_lib=""

for lib in $lib_paths; do
    if [[ -f "$lib" ]]; then
        # Check architecture using 'file' command
        file_output=$(file -L "$lib" 2>/dev/null)
        if echo "$file_output" | grep -q "ELF .* shared object, .* $DESIRED_ARCH"; then
            version=$(get_sdl_version "$lib")
            if [[ -n "$version" ]]; then
                if [ -n "$VERBOSE" ]; then
                    echo "Found matching version $version in $lib"
                fi
                version_int=$(version_to_int "$version")
                if (( version_int > latest_version_int )); then
                    latest_version_int=$version_int
                    latest_version="$version"
                    latest_lib="$lib"
                fi
            else
                if [ -n "$VERBOSE" ]; then
                    echo "Could not determine version for $lib (matching arch)"
                fi
            fi
        else
            if [ -n "$VERBOSE" ]; then
                echo "Skipping $lib (architecture mismatch)"
            fi
        fi
    fi
done

if [[ -n "$latest_lib" ]]; then
    if [ -n "$VERBOSE" ]; then
        echo "The newest matching version is $latest_version in $latest_lib"
    fi
    echo "$latest_lib"
else
    if [ -n "$VERBOSE" ]; then
        echo "No valid SDL2 library found for architecture $DESIRED_ARCH."
    fi
    # Fallback: original behavior, print the last path from whereis (or nothing if none)
    whereis libSDL2-2.0.so.0 | awk '{print $NF}'
fi
