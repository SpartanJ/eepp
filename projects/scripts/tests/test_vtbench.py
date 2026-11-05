#!/usr/bin/env python3
"""Fast protocol/report tests; no Ghostty, compiler or desktop required."""

import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
SPEC = importlib.util.spec_from_file_location('vtbench', ROOT / 'projects/benchmarks/vtbench/run.py')
bench = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(bench)


class VtBenchTests(unittest.TestCase):
    def test_macos_generator_configuration_names(self):
        with patch.object(bench.shutil, 'which', return_value='premake4'):
            self.assertEqual(bench.macos_release_config(), 'release')
        with patch.object(bench.shutil, 'which', return_value=None), \
                patch.object(bench.platform, 'machine', return_value='arm64'):
            self.assertEqual(bench.macos_release_config(), 'release_arm64')
        with patch.object(bench.shutil, 'which', return_value=None), \
                patch.object(bench.platform, 'machine', return_value='x86_64'):
            self.assertEqual(bench.macos_release_config(), 'release_x86_64')

    @unittest.skipUnless(sys.platform in ('darwin', 'linux'), 'requires a supported POSIX PTY')
    def test_capture_answers_fragmented_status_queries(self):
        code = ("import os,tty,time; tty.setraw(0); "
                "os.write(1,b'\\x1b[5'); time.sleep(.02); os.write(1,b'n'); "
                "assert os.read(0,4)==b'\\x1b[0n'; os.write(1,b'complete')")
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder) / 'capture'
            bench.capture_pty([sys.executable, '-c', code], out, timeout=3)
            self.assertEqual(out.read_bytes(), b'\x1b[5ncomplete')

    @unittest.skipUnless(sys.platform in ('darwin', 'linux'), 'requires a supported POSIX PTY')
    def test_capture_failure_is_not_accepted_as_a_valid_corpus(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(bench.BenchError):
                bench.capture_pty([sys.executable, '-c', 'raise SystemExit(7)'],
                                  Path(folder) / 'capture', timeout=3)

    @unittest.skipUnless(sys.platform in ('darwin', 'linux'), 'requires a supported POSIX PTY')
    def test_capture_timeout_reaps_child(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaisesRegex(bench.BenchError, 'timed out'):
                bench.capture_pty([sys.executable, '-c', 'import time; time.sleep(30)'],
                                  Path(folder) / 'capture', timeout=.2)

    def test_generator_is_bounded_and_can_cross_read_sizes(self):
        code = "import os\nwhile True: os.write(1,b'x'*8192)\n"
        with tempfile.TemporaryDirectory() as folder:
            out = Path(folder) / 'capture'
            bench.generated_bytes([sys.executable, '-c', code], out, 65539)
            self.assertEqual(out.read_bytes(), b'x' * 65539)

    def test_short_generator_output_is_a_failure(self):
        with tempfile.TemporaryDirectory() as folder:
            with self.assertRaises(bench.BenchError):
                bench.generated_bytes([sys.executable, '-c', 'print("short")'],
                                      Path(folder) / 'capture', 65539)

    def test_query_split_has_no_double_count(self):
        tail = b''
        count = 0
        for data in [b'\x1b', b'[5', b'n\x1b[5n', b'end']:
            added, tail = bench.status_query_count(tail, data)
            count += added
        self.assertEqual(count, 2)

    def test_hyperfine_export_versions(self):
        self.assertEqual(bench.median_seconds({'median': .25}), .25)
        self.assertEqual(bench.median_seconds({'summary': {'time_wall_clock': {'median': .5}}}), .5)

    def test_hyperfine_labels_can_be_names_or_full_commands(self):
        commands = {'eterm': ['eterm-vtbench', 'capture.vt'], 'ghostty': ['ghostty-bench', '+terminal-stream']}
        old = [{'command': 'eterm-vtbench capture.vt', 'median': .1},
               {'command': 'ghostty', 'median': .2}]
        modern = [{'name': 'eterm', 'summary': {'time_wall_clock': {'median': .1}}},
                  {'name': 'ghostty', 'summary': {'time_wall_clock': {'median': .2}}}]
        self.assertEqual(bench.timing_seconds(old, commands), {'eterm': .1, 'ghostty': .2})
        self.assertEqual(bench.timing_seconds(modern, commands), {'eterm': .1, 'ghostty': .2})
        with self.assertRaises(bench.BenchError):
            bench.timing_seconds([{'command': 'unknown', 'median': .1}], commands)

    def test_plan_update_preserves_next_steps_and_rejected_experiments(self):
        old = 'intro\n' + bench.BEGIN_MARKER + '\nold table\n' + bench.END_MARKER + '\nnext steps\nrejected\n'
        new = bench.replace_matrix(old, 'new table')
        self.assertNotIn('old table', new)
        self.assertIn('new table', new)
        self.assertTrue(new.endswith('\nnext steps\nrejected\n'))
        with self.assertRaises(bench.BenchError):
            bench.replace_matrix(old + bench.BEGIN_MARKER, 'duplicate')

    def test_freshness_uses_plan_and_detects_edited_numbers_without_local_json(self):
        fingerprint = 'a' * 64
        report = {'metadata': {'date': '2026-11-05', 'machine': 'test',
                              'eterm_commit': 'b' * 40, 'source_sha256': fingerprint,
                              'warmups': 1, 'runs': 5},
                  'workloads': {'ascii': {'eterm_mbps': 100, 'ghostty_mbps': 200, 'ratio': .5}}}
        text = bench.BEGIN_MARKER + '\n\n' + bench.matrix(report) + '\n\n' + bench.END_MARKER
        self.assertTrue(bench.plan_is_current(text, fingerprint))
        self.assertFalse(bench.plan_is_current(text.replace('100.0', '999.0'), fingerprint))
        self.assertFalse(bench.plan_is_current(text, 'c' * 64))
        with tempfile.TemporaryDirectory() as folder:
            plan = Path(folder) / 'plan.md'
            plan.write_text(text)
            with patch.object(bench, 'PLAN', plan), \
                    patch.object(bench, 'RESULTS', Path(folder) / 'absent.json'), \
                    patch.object(bench, 'source_fingerprint', return_value=fingerprint):
                self.assertEqual(bench.main(['--check']), 0)

    def test_source_fingerprint_ignores_application_edits_but_tracks_parser(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            # Fixed inputs need to exist, while empty source directories are valid.
            for name in ['premake4.lua', 'premake5.lua', '.ecode/project_build.json',
                         'projects/macos/make_no_fw.sh', 'projects/macos/make.sh',
                         'projects/benchmarks/vtbench/run.py',
                         'projects/benchmarks/vtbench/ghostty-controls.patch',
                         'src/modules/eterm/parser.cpp', 'src/tools/eefm/app.cpp']:
                p = root / name
                p.parent.mkdir(parents=True, exist_ok=True)
                p.write_text('initial')
            with patch.object(bench, 'ROOT', root):
                original = bench.source_fingerprint()
                (root / 'src/tools/eefm/app.cpp').write_text('application edit')
                self.assertEqual(original, bench.source_fingerprint())
                (root / 'src/modules/eterm/parser.cpp').write_text('parser edit')
                self.assertNotEqual(original, bench.source_fingerprint())

    def test_report_redaction_includes_nested_command_fields(self):
        value = {'results': [{'command': '<private-root>/bin/tool <private-root>/capture'}]}
        clean = bench.redact(value, [('<private-root>', '<repo>')])
        self.assertEqual(clean['results'][0]['command'], '<repo>/bin/tool <repo>/capture')


if __name__ == '__main__':
    unittest.main()
