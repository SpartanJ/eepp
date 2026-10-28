#!/bin/sh
# clang-format.sh: format C/C++ code with a clang-format that matches the project's style.
#
# Major clang-format releases format the same .clang-format differently. Major 23 is the oldest
# whose output matches the maintainer's code: 22 still writes function-pointer types as (*)
# instead of ( * ) and aligns some nested ternaries differently. All 23.x patch releases produce
# identical output for this repository. A future major may change the style again; check it
# against 23 before raising MIN_MAJOR or the wheel version.
#
# Usage:
#   projects/scripts/clang-format.sh             Format only the lines changed since HEAD.
#   projects/scripts/clang-format.sh --diff      Show what would change without modifying files.
#   projects/scripts/clang-format.sh --file F... Format whole files (only for new files).
#   projects/scripts/clang-format.sh --version   Print the clang-format that will be used.
#
# Uses clang-format and git-clang-format from PATH when clang-format is major MIN_MAJOR or newer
# (for example openSUSE Tumbleweed's clang-tools). Otherwise installs the official PyPI wheel once
# into ${XDG_CACHE_HOME:-~/.cache}/eepp/clang-format-<version>.

set -e

MIN_MAJOR=23
WHEEL_VERSION=23.1.3

if command -v clang-format >/dev/null 2>&1 && command -v git-clang-format >/dev/null 2>&1; then
	MAJOR=$(clang-format --version 2>/dev/null |
		sed -nE 's/.*clang-format version ([0-9]+)\..*/\1/p' | head -n 1)
	if [ -n "$MAJOR" ] && [ "$MAJOR" -ge "$MIN_MAJOR" ]; then
		CLANG_FORMAT=$(command -v clang-format)
		GIT_CLANG_FORMAT=$(command -v git-clang-format)
	fi
fi

if [ -z "$CLANG_FORMAT" ]; then
	VENV="${XDG_CACHE_HOME:-$HOME/.cache}/eepp/clang-format-$WHEEL_VERSION"
	if [ ! -x "$VENV/bin/clang-format" ]; then
		echo "clang-format $MIN_MAJOR or newer not found; installing $WHEEL_VERSION into $VENV" >&2
		python3 -m venv "$VENV"
		"$VENV/bin/pip" install --quiet --disable-pip-version-check \
			"clang-format==$WHEEL_VERSION"
	fi
	CLANG_FORMAT="$VENV/bin/clang-format"
	GIT_CLANG_FORMAT="$VENV/bin/git-clang-format"
fi

cd "$(git rev-parse --show-toplevel)"

# Recent git-clang-format releases also format JSON and other languages by default.
EXTENSIONS=c,h,cc,cpp,cxx,hh,hpp,hxx,inl,m,mm
case "$1" in
--version)
	"$CLANG_FORMAT" --version
	;;
--file)
	shift
	"$CLANG_FORMAT" -i "$@"
	;;
--diff)
	"$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --extensions "$EXTENSIONS" --diff HEAD
	;;
"")
	"$GIT_CLANG_FORMAT" --binary "$CLANG_FORMAT" --extensions "$EXTENSIONS" HEAD
	;;
*)
	echo "Unknown option: $1" >&2
	exit 1
	;;
esac
