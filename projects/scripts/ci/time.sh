#!/bin/bash
# Measure a command without hiding a failed compiler or packager exit status.
set -u
label=$1
shift
metrics_dir="$(cd "$(dirname "$0")/../../.." && pwd)/.ci-metrics"
mkdir -p "$metrics_dir"
started=$(date +%s)
"$@"
result=$?
elapsed=$(( $(date +%s) - started ))
printf '%s\t%s\t%s\n' "$label" "$elapsed" "$result" >> "$metrics_dir/timings.tsv"
echo "$label: ${elapsed}s (exit $result)"
exit "$result"
