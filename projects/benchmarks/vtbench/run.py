#!/usr/bin/env python3
"""Prepare, build and compare eterm and a pinned Ghostty; see README.md."""

import argparse
import errno
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import signal
import struct
import subprocess
import sys
import tarfile
import tempfile
import time
from datetime import datetime, timezone
from urllib.request import urlopen

if sys.platform in ('darwin', 'linux'):
    import fcntl
    import pty
    import select
    import termios


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
PLAN = ROOT / '.agents/plans/eterm_vt_throughput_vs_ghostty_plan.md'
RESULTS = HERE / 'results.json'
GHOSTTY = 'c7f68a7f48c6a5b357689eb8c2328bc2dab5d8ed'
ZIG = '0.17.0'
GHOSTTY_FLAGS = ['-Demit-bench', '-Doptimize=ReleaseFast', '-Demit-exe=false',
                 '-Dapp-runtime=none']
SCHEMA = 1
START = b'\x1bc\x1b[?2027h'
END = b'\x18\x1b\\\x1b[0m\r\n\x1b[?2027h' + b'\x1b[5n' * 6
QUERY = b'\x1b[5n'
BEGIN_MARKER = '<!-- vtbench:begin -->'
END_MARKER = '<!-- vtbench:end -->'
KITTEN = {'ascii': 32, 'unicode': 40, 'unique_unicode': 36, 'csi': 64,
          'images': 12, 'long_escape_codes': 8}
STYLE = ['--style-rate=0.5', '--sgr-attrs=3', '--run-min=4', '--run-max=16']
UNICODE = ['--weight-one=6', '--weight-two=2', '--weight-three=2',
           '--weight-four=0.3', '--grapheme-rate=0.02']
GENERATED = {
    'styled': ['+styled', '--seed=42', *STYLE],
    'styled_unicode': ['+styled', '--seed=42', *STYLE, *UNICODE],
    'styled_links': ['+styled', '--seed=42', *STYLE, *UNICODE, '--osc8-rate=0.1'],
    'erase': ['+erase', '--seed=42', '--rows=80', '--cols=120', '--line-clears=8'],
    'utf8_wide': ['+utf8', '--seed=42', '--ascii-printable-only',
                  '--weight-wide=0.3', '--space-rate=0.1'],
}
LABELS = {'ascii': 'ASCII', 'unicode': 'Unicode', 'unique_unicode': 'Unique Unicode',
          'csi': 'CSI', 'images': 'Images', 'long_escape_codes': 'Long escapes',
          'styled': 'Styled', 'styled_unicode': 'Styled Unicode',
          'styled_links': 'Styled with links', 'erase': 'Erase',
          'utf8_wide': 'Random wide UTF-8', 'replay': 'TUI replay (optional)'}
PUBLIC = [*KITTEN, *GENERATED]


class BenchError(RuntimeError):
    pass


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def native_arch():
    return {'aarch64': 'arm64', 'AMD64': 'x86_64'}.get(platform.machine(), platform.machine())


def macos_release_config():
    return 'release' if shutil.which('premake4') else 'release_' + native_arch()


def atomic_text(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(text, encoding='utf-8', newline='\n')
    temporary.replace(path)


def save_json(path, value):
    atomic_text(path, json.dumps(value, indent=2, ensure_ascii=False) + '\n')


def redact(value, replacements):
    if isinstance(value, str):
        for source, target in replacements:
            value = value.replace(source, target)
        return value
    if isinstance(value, dict):
        return {redact(k, replacements): redact(v, replacements) for k, v in value.items()}
    if isinstance(value, list):
        return [redact(v, replacements) for v in value]
    return value


def source_fingerprint():
    # Application-only edits (e.g. eefm) and documentation do not invalidate parser measurements.
    files = set()
    extensions = {'.h', '.hpp', '.c', '.cpp', '.m', '.mm', '.inc', '.inl'}
    for name in ['include/eepp', 'src/eepp', 'src/modules/eterm',
                 'src/thirdparty', 'src/benchmarks/vtbench']:
        files.update(p for p in (ROOT / name).rglob('*')
                     if p.is_file() and p.suffix in extensions)
    for name in ['premake4.lua', 'premake5.lua', '.ecode/project_build.json',
                 'projects/macos/make_no_fw.sh', 'projects/macos/make.sh',
                 'projects/benchmarks/vtbench/run.py',
                 'projects/benchmarks/vtbench/ghostty-controls.patch']:
        files.add(ROOT / name)
    result = hashlib.sha256()
    for path in sorted(files):
        result.update(path.relative_to(ROOT).as_posix().encode() + b'\0')
        result.update(bytes.fromhex(digest(path)))
    return result.hexdigest()


def status_query_count(tail, data):
    window = tail + data
    return window.count(QUERY), window[-(len(QUERY) - 1):]


def capture_pty(argv, destination, timeout=60):
    """Set the size before exec; answer DSR even when a query straddles two reads."""
    child, master = pty.fork()
    if child == 0:
        try:
            fcntl.ioctl(1, termios.TIOCSWINSZ, struct.pack('HHHH', 80, 120, 0, 0))
            os.execvp(argv[0], argv)
        except BaseException:
            os._exit(127)
    status = None
    try:
        tail = b''
        with destination.open('wb') as out:
            while True:
                ready, _, _ = select.select([master], [], [], timeout)
                if not ready:
                    raise BenchError('kitten capture timed out')
                try:
                    data = os.read(master, 64 * 1024)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    break
                if not data:
                    break
                out.write(data)
                queries, tail = status_query_count(tail, data)
                if queries:
                    os.write(master, b'\x1b[0n' * queries)
        _, status = os.waitpid(child, 0)
        if os.waitstatus_to_exitcode(status) != 0:
            raise BenchError('kitten capture failed')
    finally:
        os.close(master)
        if status is None:
            try:
                os.kill(child, signal.SIGKILL)
            except ProcessLookupError:
                pass
            os.waitpid(child, 0)


def generated_bytes(argv, destination, size):
    """Bound the infinite generator, and reap only the process this invocation started."""
    with destination.open('wb') as out:
        process = subprocess.Popen(argv, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
        try:
            remaining = size
            while remaining:
                data = process.stdout.read(min(remaining, 64 * 1024))
                if not data:
                    raise BenchError('ghostty-gen stopped before the requested byte count')
                out.write(data)
                remaining -= len(data)
        finally:
            process.stdout.close()
            if process.poll() is None:
                process.terminate()
            try:
                process.wait(timeout=10)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


def wrap_capture(body, target):
    temporary = target.with_suffix('.tmp')
    with temporary.open('wb') as out, body.open('rb') as stream:
        out.write(START)
        shutil.copyfileobj(stream, out)
        out.write(END)
    temporary.replace(target)


def median_seconds(result):
    # Hyperfine 1.x and 2.x export different schemas.
    if 'summary' in result:
        return result['summary']['time_wall_clock']['median']
    return result['median']


def timing_seconds(results, commands):
    seconds = {}
    for result in results:
        label = result.get('name') or result.get('command')
        for name, argv in commands.items():
            if label == name or label == shlex.join(argv):
                seconds[name] = median_seconds(result)
                break
    if set(seconds) != set(commands):
        raise BenchError('Unrecognized hyperfine result labels; no matrix was published')
    return seconds


def replace_matrix(text, replacement):
    if text.count(BEGIN_MARKER) != 1 or text.count(END_MARKER) != 1:
        raise BenchError('The plan must contain one benchmark marker pair')
    first = text.index(BEGIN_MARKER) + len(BEGIN_MARKER)
    last = text.index(END_MARKER)
    if last < first:
        raise BenchError('The benchmark markers are out of order')
    return text[:first] + '\n\n' + replacement.strip() + '\n\n' + text[last:]


def matrix(report):
    meta = report['metadata']
    lines = [f"Measured **{meta['date']}** on **{meta['machine']}**.",
             f"eterm `{meta['eterm_commit'][:9]}`; source fingerprint "
             f"`{meta['source_sha256'][:12]}` includes local changes when present. "
             f"Ghostty `{GHOSTTY[:9]}` with the benchmark-only controls patch.", '',
             f"Whole-process median, decimal MB/s; {meta['warmups']} warmup(s), "
             f"{meta['runs']} runs, 120×80, 10,000 history rows, mode 2027 on.", '',
             '| Workload | eterm MB/s | Ghostty MB/s | eterm / Ghostty |',
             '| --- | ---: | ---: | ---: |']
    for name, row in report['workloads'].items():
        lines.append(f"| {LABELS[name]} | {row['eterm_mbps']:,.1f} | "
                     f"{row['ghostty_mbps']:,.1f} | {row['ratio']:.2f}× |")
    if 'replay' in report['workloads']:
        lines += ['', 'The optional TUI replay requires a user-supplied capture; it is not part '
                  'of the public reproducible corpus.']
    body = '\n'.join(lines)
    checksum = hashlib.sha256(body.encode()).hexdigest()
    return body + f"\n\n<!-- vtbench-state: {meta['source_sha256']} {checksum} -->"


def plan_is_current(text, fingerprint):
    if text.count(BEGIN_MARKER) != 1 or text.count(END_MARKER) != 1:
        return False
    section = text.split(BEGIN_MARKER)[1].split(END_MARKER)[0].strip()
    state = re.fullmatch(r'(.*)\n\n<!-- vtbench-state: ([0-9a-f]{64}) ([0-9a-f]{64}) -->',
                         section, flags=re.S)
    return bool(state and state[2] == fingerprint and
                hashlib.sha256(state[1].encode()).hexdigest() == state[3])


class VtBench:
    def __init__(self, args):
        self.args = args
        self.cache = args.cache.resolve()
        self.cache.mkdir(parents=True, exist_ok=True)
        stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S')
        self.output = Path(tempfile.mkdtemp(prefix=stamp + '-', dir=self.cache))
        atomic_text(self.cache / 'latest-attempt.txt', self.output.name + '\n')
        self.replacements = sorted([(str(self.output), '<report>'),
                                    (str(self.cache), '<cache>'), (str(ROOT), '<repo>'),
                                    (str(Path.home()), '<home>')], key=lambda p: -len(p[0]))
        self.env = dict(os.environ)
        if sys.platform == 'darwin':
            self.library = ROOT / 'libs/macosx'
            if macos_release_config() != 'release':
                self.library /= native_arch()
        else:
            self.library = ROOT / 'libs/linux' / native_arch()
        library_dirs = [str(self.library), str(ROOT / 'bin')]
        self.env['DYLD_LIBRARY_PATH'] = ':'.join(library_dirs)
        self.env['LD_LIBRARY_PATH'] = ':'.join(library_dirs)

    def command(self, argv, label, cwd=ROOT, timeout=1800):
        log = self.output / (label + '.log')
        print(label.replace('-', ' ') + '...', flush=True)
        try:
            with log.open('w') as out:
                result = subprocess.run(argv, cwd=cwd, env=self.env, stdout=out,
                                        stderr=subprocess.STDOUT, timeout=timeout)
        finally:
            if log.exists():
                atomic_text(log, redact(log.read_text(errors='replace'), self.replacements))
        if result.returncode:
            raise BenchError(f'{label} failed; inspect {label}.log in the report')

    def prepare_ghostty(self):
        patch = HERE / 'ghostty-controls.patch'
        key = GHOSTTY[:12] + '-' + digest(patch)[:12]
        source = self.cache / ('ghostty-' + key)
        if not (source / 'build.zig').exists():
            print('Downloading pinned Ghostty sources...', flush=True)
            archive = self.cache / ('ghostty-' + GHOSTTY + '.tar.gz')
            if not archive.exists():
                partial = archive.with_suffix('.tmp')
                with urlopen(f'https://codeload.github.com/ghostty-org/ghostty/tar.gz/{GHOSTTY}',
                             timeout=120) as response, partial.open('wb') as out:
                    shutil.copyfileobj(response, out)
                partial.replace(archive)
            with tempfile.TemporaryDirectory(dir=self.cache) as directory:
                with tarfile.open(archive) as tar:
                    tar.extractall(directory, filter='data')
                unpacked = Path(directory) / ('ghostty-' + GHOSTTY)
                self.command(['patch', '-p1', '-i', str(patch)], 'ghostty-controls', cwd=unpacked)
                unpacked.rename(source)
        binaries = [source / 'zig-out/bin' / n for n in ('ghostty-bench', 'ghostty-gen')]
        build_stamp = source / '.vtbench-build.json'
        current = json.loads(build_stamp.read_text()) if build_stamp.exists() else {}
        flags = [*GHOSTTY_FLAGS]
        if sys.platform == 'darwin':
            flags += ['-Demit-macos-app=false']
        if (current.get('flags') != flags or
                not all(p.exists() and current.get(p.name) == digest(p) for p in binaries)):
            self.command(['zig', 'build', *flags], 'build-ghostty', cwd=source)
            save_json(build_stamp, {'flags': flags, **{p.name: digest(p) for p in binaries}})
        return binaries

    def build_eterm(self):
        if sys.platform == 'darwin':
            config = 'config=' + macos_release_config()
            try:
                self.command(['projects/macos/make_no_fw.sh', config, 'eterm-vtbench'],
                             'build-eterm')
            except BenchError:
                self.command(['projects/macos/make.sh', config, 'eterm-vtbench'],
                             'build-eterm-framework-retry')
        else:
            config = json.loads((ROOT / '.ecode/project_build.json').read_text())
            # Use the project's current release Ninja workflow, including regeneration.
            for index, step in enumerate(config['eepp-linux-ninja']['build']):
                values = {'project_root': str(ROOT), 'build_dir': str(ROOT / 'make/linux'),
                          'os': 'linux', 'build_type': 'release'}
                def expand(value):
                    for key, item in values.items():
                        value = value.replace('${' + key + '}', item)
                    return value
                self.command([step['command'], *shlex.split(expand(step['args']))],
                             'build-eterm-' + str(index), cwd=expand(step['working_dir']))
        binary = ROOT / 'bin/benchmarks/eterm-vtbench'
        if not binary.exists():
            raise BenchError('The release build did not produce eterm-vtbench')
        return binary

    def captures(self, generator, names):
        folder = self.cache / ('captures-quick' if self.args.quick else 'captures')
        folder.mkdir(exist_ok=True)
        manifest_path = folder / 'manifest.json'
        manifest = json.loads(manifest_path.read_text()) if manifest_path.exists() else {}
        kitten_version = subprocess.check_output(['kitten', '--version'], text=True).strip()
        size = 1_000_000 if self.args.quick else 64_000_000
        result = {}
        for name in names:
            if name == 'replay':
                target = self.output / 'replay.vt'
                wrap_capture(self.args.replay, target)
            else:
                target = folder / (name + '.vt')
                settings = {'schema': SCHEMA, 'ghostty': GHOSTTY, 'bytes': size,
                            'wrapper_sha256': hashlib.sha256(START + END).hexdigest()}
                if name in KITTEN:
                    repetitions = 1 if self.args.quick else KITTEN[name]
                    argv = ['kitten', '__benchmark__', '--repetitions', str(repetitions), name]
                    settings.update(kitten=kitten_version, argv=argv)
                else:
                    argv = [str(generator), *GENERATED[name]]
                    settings['argv'] = GENERATED[name]
                    settings['generator_sha256'] = digest(generator)
                cached = manifest.get(name, {})
                if (not target.exists() or cached.get('settings') != settings or
                        cached.get('sha256') != digest(target)):
                    print('Generating ' + name + '...', flush=True)
                    body = folder / (name + '.body')
                    try:
                        if name in KITTEN:
                            capture_pty(argv, body)
                        else:
                            generated_bytes(argv, body, size)
                        wrap_capture(body, target)
                    finally:
                        body.unlink(missing_ok=True)
                    manifest[name] = {'settings': settings, 'sha256': digest(target)}
                    save_json(manifest_path, manifest)
            result[name] = target
        return result

    def measure(self, eterm, ghostty, captures):
        rows = {}
        for index, (name, capture) in enumerate(captures.items()):
            history_rows = None
            commands = {'eterm': [str(eterm), str(capture)],
                        'ghostty': [str(ghostty), '+terminal-stream', '--data=' + str(capture),
                                    '--terminal-cols=120', '--terminal-rows=80',
                                    '--grapheme-width=unicode', '--scrollback-lines=10000']}
            for engine, argv in commands.items():
                if engine == 'ghostty':
                    argv = [*argv, '--report-state=true']
                self.command(argv, name + '-' + engine + '-preflight', timeout=120)
                text = (self.output / (name + '-' + engine + '-preflight.log')).read_text()
                if engine == 'eterm' and 'grapheme=1' not in text:
                    raise BenchError('eterm did not finish in grapheme mode')
                if engine == 'ghostty':
                    state = re.search(r'state history_rows=(\d+) grapheme=(true|false)', text)
                    if not state or state[2] != 'true':
                        raise BenchError('Ghostty did not report the expected grapheme mode')
                    history_rows = int(state[1])
            order = ['eterm', 'ghostty'] if index % 2 == 0 else ['ghostty', 'eterm']
            if self.args.reverse:
                order.reverse()
            raw = self.output / (name + '.json')
            argv = ['hyperfine', '-N', '--style', 'none', '--warmup', str(self.args.warmups),
                    '--runs', str(self.args.runs), '--export-json', str(raw)]
            for engine in order:
                argv += ['-n', engine, shlex.join(commands[engine])]
            self.command(argv, name + '-timing')
            timings = json.loads(raw.read_text())
            save_json(raw, redact(timings, self.replacements))
            seconds = timing_seconds(timings['results'], commands)
            rates = {n: capture.stat().st_size / seconds[n] / 1e6 for n in order}
            rows[name] = {'bytes': capture.stat().st_size, 'sha256': digest(capture),
                          'ghostty_history_rows': history_rows,
                          'eterm_mbps': rates['eterm'], 'ghostty_mbps': rates['ghostty'],
                          'ratio': rates['eterm'] / rates['ghostty']}
            print(f"{LABELS[name]:22} {rates['eterm']:9.1f} / {rates['ghostty']:9.1f} MB/s "
                  f" {rows[name]['ratio']:.2f}x", flush=True)
        return rows

    def run(self):
        names = self.args.workloads or list(PUBLIC)
        if self.args.replay:
            names = [*names, 'replay']
        ghostty, generator = self.prepare_ghostty()
        fingerprint = source_fingerprint()
        eterm = self.build_eterm()
        if source_fingerprint() != fingerprint:
            raise BenchError('Parser or benchmark sources changed during the build; rerun')
        binaries = {'eterm': eterm, 'ghostty': ghostty}
        for library in self.library.glob('libeepp.*'):
            if library.is_file():
                binaries[library.name] = library
        binary_hashes = {name: digest(path) for name, path in binaries.items()}
        captures = self.captures(generator, names)
        if self.args.prepare_only:
            print('Environment and captures ready; no timings published.')
            return
        rows = self.measure(eterm, ghostty, captures)
        if source_fingerprint() != fingerprint:
            raise BenchError('Parser or benchmark sources changed during the run; results not published')
        if any(digest(path) != binary_hashes[name] for name, path in binaries.items()):
            raise BenchError('A benchmark binary or shared library changed during timing; results not published')
        machine = platform.system() + ' ' + platform.machine()
        if sys.platform == 'darwin':
            machine += ', ' + subprocess.check_output(
                ['sysctl', '-n', 'machdep.cpu.brand_string'], text=True).strip()
        commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
        meta = {'schema': SCHEMA, 'date': datetime.now(timezone.utc).strftime('%Y-%m-%d'),
                'machine': machine, 'eterm_commit': commit, 'source_sha256': fingerprint,
                'ghostty_commit': GHOSTTY, 'ghostty_patch_sha256': digest(HERE / 'ghostty-controls.patch'),
                'warmups': self.args.warmups, 'runs': self.args.runs,
                'engine_order': 'alternating, reversed' if self.args.reverse else 'alternating',
                'columns': 120, 'rows': 80, 'history_rows': 10000, 'grapheme_width': 'unicode',
                'metric': 'whole-process median decimal MB/s',
                'eterm_build': 'project release configuration',
                'ghostty_build': GHOSTTY_FLAGS,
                'cxx_version': subprocess.check_output(
                    [*shlex.split(os.environ.get('CXX', 'c++')), '--version'],
                    text=True).splitlines()[0],
                'generator_arguments': GENERATED,
                'kitten_repetitions': {n: 1 if self.args.quick else r for n, r in KITTEN.items()},
                'tools': {n: subprocess.check_output([n, '--version' if n != 'zig' else 'version'],
                                                     text=True).splitlines()[0]
                          for n in ['zig', 'kitten', 'hyperfine']},
                'binaries': binary_hashes}
        report = {'metadata': meta, 'workloads': rows}
        save_json(self.output / 'results.json', report)
        atomic_text(self.output / 'matrix.md', matrix(report) + '\n')
        canonical = (all(n in rows for n in PUBLIC) and not self.args.quick and
                     self.args.runs >= 5 and self.args.warmups >= 1)
        if canonical and not self.args.no_update_plan:
            updated = replace_matrix(PLAN.read_text(), matrix(report))
            save_json(RESULTS, report)
            atomic_text(PLAN, updated)
            print('Updated the plan matrix and local results.json (gitignored).')
        else:
            print('Custom run: report saved; the reference matrix was not changed.')
        atomic_text(self.cache / 'latest-report.txt', self.output.name + '\n')
        print('Reports are in the cache; latest-report.txt identifies this run.')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cache', type=Path, default=ROOT / '.cache/vtbench')
    parser.add_argument('--workloads', nargs='+', choices=PUBLIC)
    parser.add_argument('--replay', type=Path, help='optional local raw VT capture (never uploaded)')
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--warmups', type=int, default=1)
    parser.add_argument('--quick', action='store_true', help='small corpora; does not publish the matrix')
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--no-update-plan', action='store_true')
    parser.add_argument('--reverse', action='store_true', help='reverse each workload\'s engine order')
    parser.add_argument('--check', action='store_true', help='check reference matrix source freshness')
    args = parser.parse_args(argv)
    if sys.version_info < (3, 12):
        parser.error('Python 3.12 or later is required')
    if args.check:
        current = False
        try:
            current = plan_is_current(PLAN.read_text(), source_fingerprint())
        except (OSError, KeyError, ValueError, BenchError):
            pass
        if not current:
            print('STALE: run the benchmark to refresh the reference matrix.')
            return 1
        print('Current: the reference matrix matches the parser and benchmark sources.')
        return 0
    if sys.platform not in ('darwin', 'linux'):
        parser.error('The comparison runner supports macOS and Linux (including WSL)')
    if args.runs < 1 or args.warmups < 0:
        parser.error('runs must be positive and warmups nonnegative')
    if args.replay and not args.replay.is_file():
        parser.error('the replay capture must be a readable file')
    for name in ['git', 'zig', 'kitten', 'hyperfine', 'patch']:
        if not shutil.which(name):
            parser.error('missing prerequisite: ' + name + '; see README.md')
    if subprocess.check_output(['zig', 'version'], text=True).strip() != ZIG:
        parser.error('the pinned Ghostty revision requires Zig ' + ZIG)
    args.cache.mkdir(parents=True, exist_ok=True)
    with (args.cache / 'run.lock').open('w') as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            parser.error('another vtbench run is using this cache')
        bench = VtBench(args)
        try:
            bench.run()
        except (BenchError, OSError, subprocess.SubprocessError, tarfile.TarError) as error:
            print(redact('vtbench: ' + str(error), bench.replacements), file=sys.stderr)
            return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
