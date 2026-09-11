#!/usr/bin/env python3

import argparse
import json
import os
import statistics
import subprocess
import time


def check(condition, message):
	if not condition:
		raise RuntimeError(message)


def run_sample(args, disabled):
	environment = os.environ.copy()
	if disabled:
		environment["EEPP_DISABLE_ACCESSIBILITY"] = "1"
	else:
		environment.pop("EEPP_DISABLE_ACCESSIBILITY", None)
	start = time.perf_counter()
	completed = subprocess.run(
		[
			args.executable,
			"--benchmark-inactive",
			"--benchmark-iterations",
			str(args.iterations),
		],
		check=True,
		capture_output=True,
		text=True,
		timeout=args.timeout,
		env=environment,
	)
	wall_ms = (time.perf_counter() - start) * 1000
	result = json.loads(completed.stdout)
	result["wall_ms"] = wall_ms
	result["initialization_ms"] = result.pop("initialization_us") / 1000
	result["notifications_ms"] = result.pop("notifications_us") / 1000
	return result


def median_sample(samples):
	return {
		key: statistics.median(sample[key] for sample in samples)
		for key in samples[0]
	}


def main():
	parser = argparse.ArgumentParser(description="Benchmark inactive accessibility overhead")
	parser.add_argument("--executable", default="bin/eepp-ui-accessibility")
	parser.add_argument("--iterations", type=int, default=100000)
	parser.add_argument("--samples", type=int, default=5)
	parser.add_argument("--timeout", type=float, default=10)
	parser.add_argument("--max-initialization-ms", type=float, default=250)
	parser.add_argument("--max-notification-ratio", type=float, default=1.1)
	parser.add_argument("--max-wall-ratio", type=float, default=1.25)
	parser.add_argument("--json", action="store_true")
	args = parser.parse_args()
	check(args.iterations > 0 and args.samples > 0, "iterations and samples must be positive")

	# Alternate modes so thermal and scheduler drift affect both populations similarly.
	enabled_samples = []
	disabled_samples = []
	for index in range(args.samples * 2):
		disabled = index % 2 == 0
		result = run_sample(args, disabled)
		(disabled_samples if disabled else enabled_samples).append(result)

	enabled = median_sample(enabled_samples)
	disabled = median_sample(disabled_samples)
	notification_ratio = enabled["notifications_ms"] / disabled["notifications_ms"]
	wall_ratio = enabled["wall_ms"] / disabled["wall_ms"]
	check(
		enabled["initialization_ms"] <= args.max_initialization_ms,
		"inactive native backend initialization exceeded threshold",
	)
	check(
		notification_ratio <= args.max_notification_ratio,
		"inactive accessibility notification overhead exceeded threshold",
	)
	check(wall_ratio <= args.max_wall_ratio, "inactive accessibility wall-time overhead exceeded threshold")
	output = {
		"disabled": disabled,
		"enabled": enabled,
		"notification_ratio": notification_ratio,
		"wall_ratio": wall_ratio,
	}
	if args.json:
		print(json.dumps(output, sort_keys=True))
	else:
		print(
			"Inactive accessibility benchmark passed: "
			f"initialization {enabled['initialization_ms']:.3f}ms, "
			f"notifications {enabled['notifications_ms']:.3f}ms "
			f"({notification_ratio:.3f}x disabled), wall {wall_ratio:.3f}x disabled"
		)


if __name__ == "__main__":
	main()
