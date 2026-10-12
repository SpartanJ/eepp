#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"

# MinGW-w64 11 ships the UIA headers but no x86/x64 UIAutomationCore import library.
# Generate only the imports eepp uses; the implementation remains Windows' system DLL.
case "$1" in
  64)
    UIA_ARCH=x86_64
    UIA_MACHINE=i386:x86-64
    UIA_DEFINITION=uiautomationcore.def
    UIA_FLAGS=(-m64)
    UIA_AS_FLAGS=--64
    ;;
  32)
    UIA_ARCH=x86
    UIA_MACHINE=i386
    UIA_DEFINITION=uiautomationcore_x86.def
    UIA_FLAGS=(-m32)
    UIA_AS_FLAGS=--32
    ;;
  *) exit 1 ;;
esac

UIA_COMPILER="$2"
UIA_SYSTEM_LIBRARY=$("$UIA_COMPILER" "${UIA_FLAGS[@]}" -print-file-name=libuiautomationcore.a)
if [[ -f "$UIA_SYSTEM_LIBRARY" ]]; then
  exit 0
fi

UIA_OUTPUT="../../libs/windows/$UIA_ARCH/thirdparty/libuiautomationcore.a"
if [[ -f "$UIA_OUTPUT" && "$UIA_OUTPUT" -nt "$UIA_DEFINITION" &&
      "$UIA_OUTPUT" -nt build_uiautomation.sh ]]; then
  exit 0
fi

mkdir -p "../../libs/windows/$UIA_ARCH/thirdparty"
UIA_DLLTOOL=$("$UIA_COMPILER" -print-prog-name=dlltool)
UIA_ASSEMBLER=$("$UIA_COMPILER" -print-prog-name=as)
"$UIA_DLLTOOL" --machine "$UIA_MACHINE" --as "$UIA_ASSEMBLER" --as-flags "$UIA_AS_FLAGS" --kill-at \
  --input-def "$UIA_DEFINITION" --output-lib "$UIA_OUTPUT"
