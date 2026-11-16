#!/bin/bash
# Source this after installing the native compiler and ccache, including inside VMs.

EEPP_CCACHE_WRAPPERS="$(dirname "$CCACHE_DIR")/compiler-wrappers"
mkdir -p "$EEPP_CCACHE_WRAPPERS"
eepp_ccache_binary=$(command -v ccache)
for eepp_compiler in cc c++ gcc g++ clang clang++ \
	aarch64-linux-gnu-gcc aarch64-linux-gnu-g++; do
	if command -v "$eepp_compiler" >/dev/null 2>&1; then
		ln -sf "$eepp_ccache_binary" "$EEPP_CCACHE_WRAPPERS/$eepp_compiler"
	fi
done
export PATH="$EEPP_CCACHE_WRAPPERS:$PATH"
export EEPP_CCACHE_WRAPPERS
ccache --zero-stats
unset eepp_ccache_binary eepp_compiler
