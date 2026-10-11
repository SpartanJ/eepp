#!/usr/bin/env python3

"""Accessibility allocation benchmarks: primarily dormant costs with no client attached.

Runs the AccessibilityDormant benchmarks from eepp-benchmarks (a Release build) several times and
reports the median of each measurement. Each --compare LABEL=PATH runs the same benchmark source
built against another tree as well (for example an exported develop or merge-base revision,
compiled with -DEE_BENCH_NO_ACCESSIBILITY), and prints the current build's differences from it.
Allocation counts and bytes cover ordinary C++ new/new[] on the measuring thread only (not malloc,
other threads or background work) and are deterministic; times are indicative only. A run passes
only when every sample exits cleanly and reports every expected measurement, guarded scenarios
included. A compared build whose run fails still reports its measurements.
An additional active-client guard checks that backends not requesting exact text edits allocate
nothing to construct unused records; it does not measure document editing itself.
Use --active-text for warmed semantic queries or --cold-text for first-use text and Inspector checks.
"""

import argparse
import json
import re
import statistics
import subprocess
import sys

LINE = re.compile(r"^ACCESSIBILITY_(?:DORMANT|ACTIVE) (\S+) ?(.*)$")
FIELD = re.compile(r"(\w+)=([-\d.]+)")

# Every build must report these; a run missing any of them did not measure what it claims.
COMMON = {
	"typing": ("allocations", "bytes", "ms"),
	"deleting_characters": ("allocations", "bytes", "ms"),
	"large_deletion": ("allocations", "bytes", "ms"),
	"large_deletion_removed_text_bytes": ("value", "allocated_bytes_per_removed_byte"),
	"spinbox_construction": ("allocations", "bytes", "ms"),
	"spinbox_destruction": ("allocations", "bytes", "ms"),
	"deep_hierarchy_lifecycle": ("allocations", "bytes", "ms"),
	"nested_scene_lifecycle": ("allocations", "bytes", "ms"),
	"list_scroll_updates": ("allocations", "bytes", "ms"),
	"list_model_mutations": ("allocations", "bytes", "ms"),
	"widget_notifications": ("allocations", "bytes", "ms"),
}
# Only builds with accessibility support (not EE_BENCH_NO_ACCESSIBILITY) report these, including
# the guarded unused-text-record, after-disconnect and unreferenced id scenarios.
ACCESSIBILITY = {
	"unused_text_edit_records": ("allocations", "bytes", "ms"),
	"first_update_without_thread_pool": ("allocations", "bytes", "ms"),
	"later_update_without_thread_pool": ("allocations", "bytes", "ms"),
	"readiness_ms": ("value", "backend_available"),
	"model_mutations_never_queried": ("allocations", "bytes", "ms"),
	"model_mutations_after_disconnect": ("allocations", "bytes", "ms"),
	"set_id_no_client": ("allocations", "bytes", "ms"),
	"set_id_active_unreferenced": ("allocations", "bytes", "ms"),
	"set_id_active_referenced": ("allocations", "bytes", "ms"),
}

ACTIVE_TEXT = {
	**{
		f"text_{kind}_{lines}": ("allocations", "bytes", "ms")
		for lines in (10000, 100000, 1000000)
		for kind in ("edits", "queries_after_edit", "warm_queries")
	},
	"text_index_updates": ("allocations", "bytes", "ms"),
	"text_index_work": ("line_reads", "rebuilds"),
	"child_count_materialized": ("allocations", "bytes", "ms"),
	"child_count_scalar": ("allocations", "bytes", "ms"),
	**{
		f"model_rows_{kind}_{rows}": ("allocations", "bytes", "ms")
		for rows in (100, 10000, 100000)
		for kind in ("queried", "mutation", "disconnected")
	},
}
# Original accessibility builds can run semantic queries without the new private index helper.
ACTIVE_COMMON = {
	name: keys for name, keys in ACTIVE_TEXT.items()
	if name not in {"text_index_updates", "text_index_work"}
}

COLD_TEXT = {
	**{
		f"text_{temperature}_{kind}_{lines}": ("allocations", "bytes", "ms")
		for lines in (10000, 100000, 1000000)
		for kind in ("begin", "end", "focus")
		for temperature in ("cold", "warm")
	},
	"inspector_first_requested_offsets": ("allocations", "bytes", "ms"),
	"inspector_warm_requested_offsets": ("allocations", "bytes", "ms"),
}


def parse(stdout):
	sample = {}
	for line in stdout.splitlines():
		match = LINE.match(line)
		if not match:
			continue
		name, rest = match.groups()
		fields = {}
		# Single-value lines ("name=value ...") carry the measurement in the name itself.
		if "=" in name:
			name, value = name.split("=", 1)
			fields["value"] = float(value)
		for key, value in FIELD.findall(rest):
			fields[key] = float(value)
		sample[name] = fields
	return sample


def missing(sample, expected):
	return sorted(
		f"{name}.{key}" if name in sample else name
		for name, keys in expected.items()
		for key in (keys if name in sample else keys[:1])
		if name not in sample or key not in sample[name]
	)


def run(executable, samples, timeout, expected, test_filter):
	"""Returns the median of every measurement and the reasons the run did not pass, if any."""
	results = {}
	failures = []
	for index in range(samples):
		completed = subprocess.run(
			[executable, f"--filter={test_filter}"],
			capture_output=True,
			text=True,
			timeout=timeout,
		)
		if completed.returncode != 0:
			failures.append(f"sample {index + 1}: exit code {completed.returncode}")
		sample = parse(completed.stdout)
		absent = missing(sample, expected)
		if absent:
			failures.append(f"sample {index + 1}: missing {', '.join(absent)}")
		for name, fields in sample.items():
			for key, value in fields.items():
				results.setdefault(name, {}).setdefault(key, []).append(value)
	medians = {
		name: {key: statistics.median(values) for key, values in fields.items()}
		for name, fields in results.items()
	}
	return medians, failures


def main():
	parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
	parser.add_argument("--executable", default="bin/benchmarks/eepp-benchmarks")
	parser.add_argument("--compare", action="append", default=[], metavar="LABEL=PATH",
		help="the same benchmark built against another tree")
	parser.add_argument("--samples", type=int, default=3)
	parser.add_argument("--timeout", type=float, default=300)
	parser.add_argument("--json", action="store_true")
	mode = parser.add_mutually_exclusive_group()
	mode.add_argument("--active-text", action="store_true",
		help="measure active text indexing, child counts and queried model rows instead of dormant costs")
	mode.add_argument("--cold-text", action="store_true",
		help="measure unqueried first edits, first focus and Inspector offset-index laziness")
	args = parser.parse_args()

	if args.samples < 1:
		parser.error("--samples must be at least 1")
	builds = [("current", args.executable)]
	for entry in args.compare:
		label, _, path = entry.partition("=")
		if not label or not path:
			parser.error(f"--compare expects LABEL=PATH, got {entry!r}")
		if any(label == existing for existing, _ in builds):
			# Results are keyed by label: a repeated one would replace another build's results.
			reason = "is reserved for --executable" if label == "current" else "is already used"
			parser.error(f"--compare label {label!r} {reason}")
		builds.append((label, path))
	output = {}
	failures = {}
	for label, path in builds:
		# The current build must measure everything; a compared tree may lack accessibility.
		if args.cold_text:
			expected = COLD_TEXT
			test_filter = "AccessibilityCold.*"
		elif args.active_text:
			expected = ACTIVE_TEXT if label == "current" else ACTIVE_COMMON
			test_filter = "AccessibilityActive.*"
		else:
			expected = {**COMMON, **ACCESSIBILITY} if label == "current" else COMMON
			test_filter = "AccessibilityDormant.*"
		output[label], failures[label] = run(path, args.samples, args.timeout, expected, test_filter)
	passed = {label: not failures[label] for label in failures}
	if args.json:
		print(json.dumps({"results": output, "guards_passed": passed, "failures": failures},
			indent=1, sort_keys=True))
		sys.exit(0 if passed["current"] else 1)

	current = output["current"]
	labels = [label for label, _ in builds]
	for key in ("allocations", "bytes", "ms"):
		print(f"\n{key} (median of {args.samples})")
		print(f"{'measurement':42}" + "".join(f" {label:>16}" for label in labels))
		for name in sorted(current):
			if key not in current[name]:
				continue
			row = f"{name:42}"
			for label in labels:
				value = output[label].get(name, {}).get(key)
				if value is None:
					row += f" {'-':>16}"
				else:
					row += f" {value:>16.3f}" if key == "ms" else f" {value:>16.0f}"
			print(row)
	for label in labels:
		for failure in failures[label]:
			print(f"\n{label}: {failure} (measurements above are still reported)")
	sys.exit(0 if passed["current"] else 1)


if __name__ == "__main__":
	main()
