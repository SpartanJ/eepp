#!/bin/sh
# AddressSanitizer debug builds keep their own objects in obj/macosx/asan, so changing flavour only
# recompiles what changed. Both flavours share the linked outputs though, and make wouldn't relink
# them: the objects of the other flavour are older. Remove them so they're linked again.
cd "$(dirname "$0")" || exit 1

FLAVOUR=${1:-plain}
MARKER=../../obj/macosx/.debug-flavour
PREVIOUS=$(cat "$MARKER" 2>/dev/null)

if [ "${PREVIOUS:-plain}" != "$FLAVOUR" ]; then
    find ../../bin -maxdepth 2 -type f -perm -100 -name '*-debug' -delete
    find ../../libs/macosx -type f -name '*-debug.*' -delete
fi

mkdir -p "$(dirname "$MARKER")"
echo "$FLAVOUR" > "$MARKER"
