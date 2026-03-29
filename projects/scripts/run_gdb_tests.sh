#!/bin/bash
set -euo pipefail

cd ../../bin/unit_tests

echo "=== Running eepp unit tests under GDB (xvfb) ==="

xvfb-run -s "-screen 0 1280x1024x24" \
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
