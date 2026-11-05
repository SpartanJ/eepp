# VT throughput benchmark

From the repository root:

```sh
python3 projects/benchmarks/vtbench/run.py
```

The runner downloads a pinned Ghostty source archive, applies the checked-in benchmark-controls
patch, builds both engines, captures/generates the workloads, checks that they finish, and runs
hyperfine. A complete default run replaces the plan's current matrix and this directory's
`results.json` (gitignored). It never appends historical tables. Downloads, corpora, builds and raw reports
are cached in the ignored `.cache/vtbench` directory; subsequent runs reuse them.

## Prerequisites

- macOS or Linux, including WSL. Native Windows is not supported by Ghostty's comparison tools.
- Python 3.12+, Git, `patch`, `hyperfine`, and `kitten` (the kitty companion executable).
  Kitten must support `__benchmark__`; 0.49.2 is tested. Hyperfine 2.0.0 is tested, with both
  modern and older JSON exports supported.
- Zig **0.17.0**, matching the pinned Ghostty revision. The runner checks the version rather than
  silently using a different compiler. Dependencies declared by Ghostty are fetched by Zig.
- A working eepp development checkout, including its submodules and normal C++20 build dependencies.
  See the repository's build instructions in `README.md`.
- macOS: Xcode command-line tools, Premake 4 or 5, and SDL2 (`sdl2-config`). Ghostty's Metal backend
  also requires the Xcode Metal toolchain; install it through Xcode or
  `xcodebuild -downloadComponent MetalToolchain` if the Ghostty build reports it missing.
  The runner selects `release` for Premake 4 or the native architecture's release configuration
  for Premake 5; both generators define `eterm-vtbench`.
- Linux: Premake 5, Ninja, a C++20 compiler, SDL3 development files and eepp's normal platform
  dependencies. The runner reads the release Ninja commands from `.ecode/project_build.json`.
  Include `pkg-config` and OpenGL development headers/libraries. Ghostty is built with
  `-Dapp-runtime=none -Demit-exe=false`, so its GTK application dependencies are unnecessary;
  Zig fetches the bundled dependencies used by the benchmark tools.

No packages are installed with administrator privileges. The first run needs network access for
the pinned archive and Zig dependencies. Existing source checkouts and remotes are not modified.
Builds and measurements run sequentially. No terminal window is opened.

## Useful commands

```sh
# Check whether the published matrix still matches the parser and harness sources.
python3 projects/benchmarks/vtbench/run.py --check

# Prepare everything without timing or replacing the matrix.
python3 projects/benchmarks/vtbench/run.py --prepare-only

# Small smoke test; never replaces the reference matrix.
python3 projects/benchmarks/vtbench/run.py --quick --runs 1 --warmups 0 --workloads ascii utf8_wide

# Focused measurements; incomplete matrices are kept in the report only.
python3 projects/benchmarks/vtbench/run.py --workloads ascii unicode utf8_wide

# Reverse engine order when checking small differences.
python3 projects/benchmarks/vtbench/run.py --reverse --no-update-plan

# Use --replay with a user-supplied raw VT file for an optional real-TUI row.
python3 projects/benchmarks/vtbench/run.py --replay capture.vt

# Measure on another machine without replacing the repository's reference matrix.
python3 projects/benchmarks/vtbench/run.py --no-update-plan
```

`--cache` chooses a different cache directory. The cache's `latest-report.txt` identifies the
latest completed report. Each report contains the matrix, JSON medians, raw hyperfine measurements,
preflight logs and build logs. Published metadata and reports replace machine-specific absolute
paths with labels. Private replay contents are never checked in or uploaded.
`latest-attempt.txt` also identifies failed or preparation-only attempts and their diagnostic logs.

For parser differential checks, `bin/benchmarks/eterm-vtbench CAPTURE SCREEN-DUMP` writes canonical
visible cells and the cursor. It resolves cluster codepoints and hyperlink values instead of
comparing registry IDs. Compare dumps from baseline and candidate binaries on identical captures.
Use a prepared corpus file for `CAPTURE`; send raw user captures through `--replay` first.

## Measurement contract

- Both engines stream the **same files**, with 120 columns, 80 rows, 10,000 requested history rows
  and grapheme mode 2027 enabled by default, including after RIS. eterm reads at most 64 KiB per
  call. There is no up-front whole-file read or skipped input.
- The Ghostty patch only exposes history controls and reports effective state. It does not change
  parser algorithms. The source commit and patch checksum are recorded with each run.
- Kitten repetitions are fixed per workload. Ghostty-generated workloads use seed 42 and 64 MB
  of generator output. Generator arguments, tool versions and capture checksums are recorded.
- Every file ends with a parser recovery terminator and six DSR status queries. eterm fails if it
  cannot consume the complete file or does not answer at least six queries. Preflight checks also
  verify final grapheme mode in both engines. These are completion checks, not a substitute for
  parser tests or differential screen checks.
- Rates use **whole-process wall-clock medians**, decimal MB/s, one warmup and five runs. Engine
  order alternates between workloads. Small proposed improvements need a second comparison with
  engine/candidate order reversed; one favorable result is not enough.
- This measures VT processing, not PTY latency, rendering, GPU time or interactive frame latency.
  Application startup and file I/O are included, so very fast workloads can be startup-sensitive.
- Dense unique-link traffic is excluded until registry exhaustion stops making retained link
  attributes timing-dependent. `CSI 22 J` erase/scroll traffic is excluded because eterm ignores
  it. Neither is a valid comparable result today.
- A TUI replay is optional because private session output cannot be part of a redistributable
  corpus. A different capture is a different workload; compare its checksum before comparing rates.

## Keeping the reference current

After a parser, decoder, core-library or harness change, run the full benchmark before reporting
the reference matrix as current. `--check` compares the plan's embedded source fingerprint and
table checksum, so it works in a fresh checkout without the ignored local JSON. Application source edits
and documentation changes do not invalidate parser numbers, while build-configuration changes do.
Partial runs, smoke tests, failed preflights or source changes during timing never replace the
reference matrix. Record a pending
refresh explicitly if the source changes before a full run is possible.

Keep the plan as a current working reference: one generated matrix, retained implementation facts,
detailed next experiments and a concise rejected-experiment table. Do not add machine-local paths,
embedded scripts, build transcripts or historical matrices. Detailed run artifacts belong in the
ignored cache. Update this README and the pinned Ghostty commit/patch together when changing the
benchmark protocol; regenerate captures and refresh the complete matrix afterward.

The full pipeline is validated on macOS, including both Premake generators. Linux validation
covers Ninja target generation, GCC syntax and the runner's protocol tests; a full Linux
comparison still needs the documented Zig, kitten and hyperfine prerequisites.
