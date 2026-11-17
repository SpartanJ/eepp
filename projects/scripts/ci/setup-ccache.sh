#!/bin/bash
# Source this after installing the native compiler and ccache, including inside VMs.

# ccache 3 uses zlib (maximum 9); ccache 4 uses Zstandard (maximum 22).
# Force compression even when a restored configuration disables it.
unset CCACHE_NOCOMPRESS CCACHE_HARDLINK CCACHE_FILECLONE
export CCACHE_COMPRESS=true CCACHE_NOHARDLINK=true CCACHE_NOFILECLONE=true
case "$(ccache --version)" in
	'ccache version 3.'*) export CCACHE_COMPRESSLEVEL=9 ;;
	*) export CCACHE_COMPRESSLEVEL=22 ;;
esac
echo "ccache compression enabled: level $CCACHE_COMPRESSLEVEL; cache limit ${CCACHE_MAXSIZE:-default}"

# Keep executable symlinks on the runner/VM's native filesystem: an SSHFS-backed
# workspace can resolve their absolute targets on the host instead of in the VM.
EEPP_CCACHE_WRAPPERS=$(mktemp -d "${TMPDIR:-/tmp}/eepp-compiler-wrappers.XXXXXX")
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
