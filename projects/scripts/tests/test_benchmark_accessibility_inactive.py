"""Deterministic runner tests; no graphics, D-Bus or benchmark timings are required."""

import contextlib
import importlib.util
import io
import json
from pathlib import Path
import subprocess
import unittest
from unittest import mock


SCRIPT = Path(__file__).resolve().parents[1] / "benchmark_accessibility_inactive.py"
SPEC = importlib.util.spec_from_file_location("benchmark_accessibility_inactive", SCRIPT)
benchmark = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(benchmark)


def sample(notifications_ms=1000):
    return {
        "initialization_ms": 0.02,
        "readiness_ms": 1,
        "warmup_ms": 100,
        "notifications_ms": notifications_ms,
        "wall_ms": notifications_ms + 100,
        "iterations": 1000000,
        "warmup_iterations": 100000,
    }


def executable_report():
    return {
        "initialization_us": 20,
        "readiness_us": 1000,
        "warmup_us": 100000,
        "notifications_us": 1000000,
        "iterations": 1000000,
        "warmup_iterations": 100000,
    }


class InactiveBenchmarkTests(unittest.TestCase):
    def test_defaults_preserve_strict_guard_with_long_fixed_workload(self):
        args = benchmark.create_parser().parse_args([])
        self.assertEqual(args.iterations, 1000000)
        self.assertEqual(args.warmup_iterations, 100000)
        self.assertEqual(args.samples, 10)
        self.assertEqual(args.max_notification_ratio, 1.01)
        self.assertFalse(hasattr(args, "confirmation_samples"))

    def test_pairs_reverse_order_without_losing_pair_alignment(self):
        args = benchmark.create_parser().parse_args(["--samples", "4"])
        enabled, disabled, calls = [], [], []

        def run(args, is_disabled):
            calls.append(is_disabled)
            return {"pair": (len(calls) - 1) // 2}

        with mock.patch.object(benchmark, "run_sample", side_effect=run):
            benchmark.collect_samples(args, enabled, disabled)
        self.assertEqual(calls, [True, False, False, True, True, False, False, True])
        self.assertEqual(enabled, [{"pair": n} for n in range(4)])
        self.assertEqual(disabled, enabled)

    def test_guard_still_uses_ratio_of_medians(self):
        enabled = [sample(v) for v in (102, 99, 100)]
        disabled = [sample(v) for v in (100, 101, 140)]
        output = benchmark.summarize(enabled, disabled)
        self.assertEqual(output["notification_ratio"], 100 / 101)

    def test_fixed_count_on_both_pass_and_failure_and_failure_json_survives(self):
        for ratio in (1.0, 1.02):
            with self.subTest(ratio=ratio):
                output = io.StringIO()

                def run(args, disabled):
                    return sample(1000 if disabled else 1000 * ratio)

                with mock.patch.object(benchmark, "run_sample", side_effect=run) as runner:
                    with contextlib.redirect_stdout(output):
                        if ratio > 1.01:
                            with self.assertRaisesRegex(RuntimeError, "notification overhead"):
                                benchmark.main(["--json"])
                        else:
                            benchmark.main(["--json"])
                self.assertEqual(runner.call_count, 20)
                report = json.loads(output.getvalue())
                self.assertEqual(report["sample_pairs"], 10)
                self.assertEqual(len(report["raw_enabled"]), 10)
                self.assertEqual(len(report["raw_disabled"]), 10)
                self.assertEqual(bool(report["failures"]), ratio > 1.01)
                self.assertEqual(report["notification_ratio"], ratio)

    def test_exact_threshold_is_allowed(self):
        def run(args, disabled):
            return sample(1000 if disabled else 1010)

        with mock.patch.object(benchmark, "run_sample", side_effect=run):
            with contextlib.redirect_stdout(io.StringIO()):
                benchmark.main(["--json", "--samples", "2"])

    def test_all_guards_remain_failures(self):
        def run(args, disabled):
            result = sample(1000 if disabled else 1500)
            if not disabled:
                result["initialization_ms"] = 251
            return result

        output = io.StringIO()
        with mock.patch.object(benchmark, "run_sample", side_effect=run):
            with contextlib.redirect_stdout(output):
                with self.assertRaises(RuntimeError):
                    benchmark.main(["--json", "--samples", "2"])
        self.assertEqual(len(json.loads(output.getvalue())["failures"]), 3)

    def test_warmup_argument_and_disabled_environment_are_forwarded(self):
        args = benchmark.create_parser().parse_args([])
        completed = subprocess.CompletedProcess([], 0, json.dumps(executable_report()), "")
        for disabled in (False, True):
            with self.subTest(disabled=disabled):
                with mock.patch.dict(benchmark.os.environ, {"EEPP_DISABLE_ACCESSIBILITY": "1"}):
                    with mock.patch.object(benchmark.subprocess, "run", return_value=completed) as run:
                        with mock.patch.object(benchmark.time, "perf_counter", side_effect=(10, 11.25)):
                            result = benchmark.run_sample(args, disabled)
                command = run.call_args.args[0]
                self.assertEqual(command[-2:], ["--benchmark-warmup-iterations", "100000"])
                self.assertEqual("EEPP_DISABLE_ACCESSIBILITY" in run.call_args.kwargs["env"], disabled)
                self.assertTrue(run.call_args.kwargs["check"])
                self.assertEqual(result["warmup_ms"], 100)
                self.assertEqual(result["wall_ms"], 1250)

    def test_stale_binary_or_invalid_duration_is_rejected(self):
        args = benchmark.create_parser().parse_args([])
        cases = [
            ("warmup_iterations", None),
            ("iterations", 100000),
            ("warmup_us", None),
            ("notifications_us", 0),
            ("notifications_us", float("nan")),
            ("initialization_us", float("inf")),
            ("readiness_us", -1),
        ]
        for key, value in cases:
            with self.subTest(key=key, value=value):
                report = executable_report()
                report[key] = value
                completed = subprocess.CompletedProcess([], 0, json.dumps(report), "")
                with mock.patch.object(benchmark.subprocess, "run", return_value=completed):
                    with self.assertRaises(RuntimeError):
                        benchmark.run_sample(args, False)

    def test_invalid_arguments_do_not_launch_a_process(self):
        for option, value in (
            ("--iterations", "0"), ("--samples", "0"),
            ("--warmup-iterations", "-1"), ("--timeout", "0"),
            ("--max-notification-ratio", "nan"), ("--max-wall-ratio", "inf"),
        ):
            with self.subTest(option=option):
                with mock.patch.object(benchmark, "run_sample") as run:
                    with self.assertRaises(RuntimeError):
                        benchmark.main([option, value])
                run.assert_not_called()

    def test_failed_executable_is_not_turned_into_a_timing_result(self):
        args = benchmark.create_parser().parse_args([])
        with mock.patch.object(benchmark.subprocess, "run",
                               side_effect=subprocess.CalledProcessError(1, "benchmark")):
            with self.assertRaises(subprocess.CalledProcessError):
                benchmark.run_sample(args, False)


if __name__ == "__main__":
    unittest.main()
