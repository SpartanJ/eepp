# eterm VT throughput vs Ghostty

## Purpose and current state

Measure complete VT processing on identical input, then make small, verified improvements where
real output benefits. This is a current working reference, not an experiment diary. Setup and
runnable tools live in [the benchmark guide](../../projects/benchmarks/vtbench/README.md).

Retained work includes bulk string scanning, ASCII/four-cell Unicode writes, SIMD UTF-8
conversion with adaptive 2,048-slot spans, cached character properties, batched combining-mark
extensions, decoded-run retention after grapheme boundaries, SIMD row clears, blank-row erase
tracking, SIMD base64/image-buffer reuse and CSI/REP parsing improvements. Mapping character
properties to mode bits improves regular Unicode modestly; fixed-width store specialization
was removed.

Interactive improvements also retain readiness-based PTY waits, dirty-row snapshot reuse and
bounded POSIX terminal-generated replies. User input, including large pastes, remains unrestricted;
pending writes use offsets instead of repeatedly copying the remaining buffer. These changes
need separate latency/render measurements beyond parser throughput.

## Current benchmark matrix

<!-- vtbench:begin -->

Measured **2026-11-05** on **Darwin arm64, Apple M2 Pro**.
eterm `84b47822d`; source fingerprint `468db11c0bc7` includes local changes when present. Ghostty `c7f68a7f4` with the benchmark-only controls patch.

Whole-process median, decimal MB/s; 1 warmup(s), 5 runs, 120×80, 10,000 history rows, mode 2027 on.

| Workload | eterm MB/s | Ghostty MB/s | eterm / Ghostty |
| --- | ---: | ---: | ---: |
| ASCII | 399.3 | 509.2 | 0.78× |
| Unicode | 530.5 | 648.1 | 0.82× |
| Unique Unicode | 116.0 | 83.2 | 1.39× |
| CSI | 254.7 | 196.1 | 1.30× |
| Images | 2,515.9 | 2,291.4 | 1.10× |
| Long escapes | 3,010.4 | 4,287.7 | 0.70× |
| Styled | 275.6 | 279.9 | 0.98× |
| Styled Unicode | 146.5 | 144.6 | 1.01× |
| Styled with links | 144.5 | 122.6 | 1.18× |
| Erase | 226.9 | 193.6 | 1.17× |
| Random wide UTF-8 | 111.3 | 264.5 | 0.42× |
| TUI replay (optional) | 283.8 | 256.0 | 1.11× |

The optional TUI replay requires a user-supplied capture; it is not part of the public reproducible corpus.

<!-- vtbench-state: 468db11c0bc7bdd2c471a97a5fca2ad27bc80ed9a2b52876f4d9decfc6863d60 cc8aa29326ee029bb0e244712b6bea26c433a9a90a9cba7fe8425ee45cfe8fed -->

<!-- vtbench:end -->

Generated local `results.json` reports contain capture fingerprints, tool versions and medians;
they are gitignored. The matrix embeds its source fingerprint and table checksum, so freshness
checks work in a fresh checkout without those files. Keep one date and one comparison protocol;
never mix rows from separate runs. Check source freshness with:

```sh
python3 projects/benchmarks/vtbench/run.py --check
```

## Measurement and correctness rules

- Run the [benchmark script](../../projects/benchmarks/vtbench/README.md) from the repository root.
  It prepares the pinned Ghostty build, builds eterm through the project workflow, generates the
  public corpus and refreshes the complete matrix. Partial/smoke runs never replace it.
- Both engines stream identical files at 120×80, 10,000 requested history rows and mode 2027 on,
  including after reset. Use whole-process decimal MB/s, one warmup and five runs. This does not
  measure PTY latency, rendering, GPU work or interactive frame latency.
- Each eterm run must consume every byte and pass the trailing DSR-response checks. Run existing
  `eterm*` release tests for parser changes. Compare baseline/candidate screens and cursors using
  resolved clusters/links, fragmented reads and both grapheme modes where relevant. Completion
  checks alone do not establish semantic equivalence.
- Reverse candidate order before keeping small gains. Run builds and benchmarks serially. Apple
  Silicon code placement, scheduling and host activity can overwhelm tiny differences.
- Dense unique-link traffic is excluded: unchanged runs can retain different final links under
  registry exhaustion and time-limited collection. `CSI 22 J` erase/scroll is also excluded
  because eterm ignores that operation.
- Preserve observable row clears, snapshots, history, selections, wide-cell repair, kitty
  placeholders, wrap padding, malformed UTF-8 recovery and grapheme boundaries. Keep 16-byte
  cells. No delayed clears or style-table redesign is planned.

## What to test next

Work from the smallest isolated changes to larger ones. Keep a candidate only when its benefit
repeats on the intended workload without unacceptable regressions elsewhere.

### 1. Locate the remaining Unicode writer cost

The previous profile put much of random-wide time in the inlined `tputrun()` writer, but predates
the cluster-boundary fix. Refresh it before trusting its percentages.

1. Compare random wide UTF-8 with controlled assigned CJK, alternating widths and text with/without
   combining marks. Use the same source revision and corpus in both engines.
2. In a diagnostic build, count batch successes/exits: mixed widths, old wide/dummy repairs, row
   limits, selections, placeholders and cluster boundaries. Keep counters out of timed binaries.
   Determine how much time is in the four-cell versus scalar writer; the fixed-offset trial did
   not answer that split.
3. Inspect per-cell property/codepoint loads and stores. First try loading four accepted source
   properties/codepoints before writing destination cells, to see whether possible aliasing
   prevents load hoisting or scheduling. Check register pressure/spills. Do not add `restrict`
   unless ownership proves non-aliasing for every caller.
4. If neutral, separate decoding/classification from writing in a diagnostic build and inspect
   their costs again. Change one local mechanism at a time rather than rewriting both paths.
5. Gate candidates on the complete matrix, reversed order for small deltas and screen/cursor
   equality. Watch ASCII, regular Unicode, Unique Unicode and an optional real-TUI replay.
   Random-wide contains unassigned/private-use characters; a synthetic-only gain does not justify
   a large redesign.

### 2. Make dense-link benchmarking semantically stable

Reproduce differing retained links on the unchanged parser. Record registry occupancy, failed
registrations and collection timing. Separate a bounded reusable URI set (density) from unique-URI
churn (exhaustion). Add a deterministic semantic check before restoring a dense-link matrix row.
Do not claim a speedup when one run dropped more hyperlinks.

### 3. Measure a visible full-screen TUI repaint

Hidden-window CPU samples do not establish visible frame cost. Measure a large TUI replay in a
visible native window when the user is available. Separate worker parsing, `draw()`/snapshot
publication, main-thread layout/shaping and GPU/frame latency. Record the actual renderer/device
and repeatable window/grid size.

If copying still dominates, attribute copies between emulator cells and snapshots first. Consider
removing one only after proving ownership, retained-snapshot safety, resize handling and dirty-row
generations. Do not delay clears.

### 4. Revisit ASCII and unsupported string payloads with profile evidence

Reprofile ASCII after the writer investigation. Earlier row-clear/copying suspicions predate
blank-row tracking. Prefer a demonstrated local hot path over a cell representation redesign.

For long escapes, distinguish startup/file I/O from `strparse()` work. Skipping unsupported OSC
payload splitting merits a small trial only with centralized dispatch and correct oversized OSC
7/8 state clearing. Check fragmented terminators, C1, discarded strings and malformed payloads.
Real OSC traffic is small; avoid complexity for a synthetic win.

### 5. Revalidate across platforms before release

Run release terminal tests on native Linux/GCC, Windows/MSVC in CI, Windows/MinGW and Haiku.
Exercise x86 SSE2/AVX2 dispatch and ARM64 NEON. Wine timing is not native Windows timing. The
generated matrix identifies its measured host; do not infer other-platform performance from it.
Follow each host's documented project build workflow.

POSIX reply budgeting does not yet apply to ConPTY. Keep that gap explicit when testing Windows:
check reply floods, whole-reply dropping, large intact pastes and write backpressure before
extending the policy. Do not claim that the POSIX memory bound covers Windows.

## Tried and not retained

These summaries prevent repeating experiments; they are not alternative current baselines.

| Experiment | Why it was not kept / lesson |
| --- | --- |
| Unbounded SIMD UTF-8 conversion | Reconverted discarded suffixes around marks, severely hurting Unique Unicode. Bounded/adaptive conversion is retained. |
| Explicit NEON four-cell ASCII writer | ASCII was flat; Unique Unicode and long escapes lost about 3%. Portable batching already generated efficient stores. |
| Retry four-cell batches after mixed-width scalar writes | Variable regular-Unicode gains, repeatable Unique Unicode losses and weaker ASCII. |
| Force-inline `tpututf8()` | Regular Unicode slowed about 2.3% without a useful CSI gain. |
| SIMD seed/minimum threshold changes | Smaller/inconsistent gains than the retained 2,048 slots. The 1,024-slot trial substantially hurt Unique Unicode. |
| Flatten `UnicodeRun::mode()` conditionals | No repeatable gain. Direct bit mapping was tested separately and retained. |
| Fixed-width four-cell stores | Fixed offsets appeared in assembly, but the clean-wide gain vanished on reversal; random-wide stayed within noise and ASCII slowed 0.7–2.1%. |
| Invalidate all blank-row marks on color changes | Cost Codex about 2%. Keep unrelated rows' valid marks. |
| Kill the child at a shared 4 MiB input limit | Terminated healthy large pastes. Budget only whole generated POSIX replies; preserve user input. |
| Timer-only PTY polling refinements | Reduced latency but kept idle/spinner wakeups. Readiness waits supersede the adaptive timer for supported real PTYs. |

## Maintenance

After changes affecting VT throughput, refresh the matrix:

```sh
python3 projects/benchmarks/vtbench/run.py
```

Keep only the generated current matrix here. The runner records source fingerprints, refuses
publication if relevant sources change during timing, and excludes partial/failed runs. State
explicitly when a refresh is pending. Optional private replay contents stay out of version control.

Update detailed next steps as evidence changes. Add one concise row for a rejected idea and why
it failed; do not append full historical tables, source listings or transcripts. Use repository-
relative links and commands. Never include machine-local paths. Raw artifacts belong in the
ignored benchmark cache. Review both Premake generators when changing the benchmark target.
