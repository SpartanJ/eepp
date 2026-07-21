#!/bin/bash
set -euo pipefail
CANONPATH=$(readlink -f "$0")
DIRPATH="$(dirname "$CANONPATH")"

cd ../../bin/unit_tests

echo "=== Running eepp unit tests under GDB (xvfb) ==="

sh "$DIRPATH/xvfb-run-eepp" \
  gdb --batch --quiet --return-child-result \
    -ex "set confirm off" \
    -ex "set print thread-events off" \
    -ex "run" \
    -ex "bt full" \
    -ex "thread apply all bt full" \
    -ex "info registers" \
    -ex "quit" \
    --args ./eepp-unit_tests "$@"

GDB_STATUS=$?

if [ $GDB_STATUS -ne 0 ] && [ $GDB_STATUS -ne 139 ]; then
  echo "❌ GDB exited with unexpected code: $GDB_STATUS"
  exit 1
fi

echo "✅ Unit tests completed (GDB status: $GDB_STATUS)"
