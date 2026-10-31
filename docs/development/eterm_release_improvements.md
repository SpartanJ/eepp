# eterm integration implementation review

Status as of 2026-10-30, based on the supplied terminal improvements plan. The three features are
implemented; the performance and manual release checks below remain open. No commits were created.

## Implemented

- OSC 8 links use bits 16–30 of glyph mode. Glyphs remain 16 bytes. The worker owns a bounded
  registry, groups explicit IDs by URI and ID, and reclaims unreachable entries across both screens
  and history. Immutable snapshots own only visible URI mappings, including across ID reuse.
- Default-modifier clicks and context-menu open/copy actions use snapshot hit testing.
  Navigation permits HTTP, HTTPS and mailto. Held-button repeats cannot turn an already forwarded
  TUI mouse press into a hyperlink gesture. Popup callbacks are detached during terminal teardown.
- Hover/click gestures use `KeyMod::getDefaultModifier()`. Modifier-hover shows a hand cursor, a single thicker bottom underline, and a bottom-left target
  preview. The preview has a one-pixel top/right border, rounded top-right corner and open
  left/bottom edges. Modifier changes update feedback even when the pointer stays still.
- `enable_osc8_hyperlinks = true` is persistent and hot-applied. Disabling it immediately blocks UI
  actions and queues the parser policy change. New sessions receive the policy before parsing.
- New standalone tabs/splits resolve the original source before structural changes. Verified local
  OSC 7 reports take precedence over the source launch directory, then the existing launch default.
  Malformed/remote reports clear the previously published directory. OSC-only reports publish
  snapshots without requiring subsequent printable output.
- `inherit_working_directory = true` is persistent. Initial CLI launches, executable launches and
  execute-in-shell workflows retain their directory semantics. ecode exposes the same three
  default-on settings, including editor tabs/splits, status-panel terminals and OSC policies for
  assistant terminal widgets. Explicit build/run and context-menu directories retain priority.
- `allow_osc52_clipboard_write = true` matches Ghostty's default write policy. Initial and hot
  policies are enforced before worker clipboard events are emitted. Queries and unsupported
  selectors are ignored. Strict Base64, 256 KiB decoded limits and NUL rejection replace the old
  permissive decoder. Clipboard reads remain unsupported.
- Terminal options and shared terminal menu strings were added to all four ecode locale catalogs. Usage is documented in
  [eterm shell integration](../articles/eterm_integration.md).

Additional fixes needed by these paths: RIS now resets the viewport before clearing screens;
reflow preserves linked trailing spaces; standalone context-menu commands execute once; and
`Engine::openURI()` recognizes authority-less schemes such as mailto while retaining drive paths.

## Validation

- Configured SDL3 release Ninja build: passed, including eterm, ecode and unit tests.
- Linux native release: **143 terminal tests passed**.
- MinGW x86_64 release: eterm and unit tests built successfully; **143 terminal tests passed under
  Wine**, with hidden windows.
- Full Linux native suite run: **2,269 passed, 6 skipped, 2 failed** out of 2,277. Both failures are
  TLS handshake assertions in `TLSSecurity.HTTPSAcceptsTrustedPeerAndRejectsWrongHostAndUntrustedCertificate`
  and `TLSSecurity.RestoredSessionsResumeOnTLS12AndTLS13`. TLS sources were not changed. This full
  run preceded the last terminal-only regression additions; those additions passed focused runs.
- Headless standalone inspector checks passed for CLI launch directories, source-specific new tabs,
  remote-directory fallback, all four split directions, disabling inheritance, all three default-on
  settings, and GUI toggle/persistence in both directions. Tests used an isolated configuration and
  a PTY fixture; no desktop browser or user clipboard was changed.
- ecode headless inspector checks passed for default-on settings and persistence in both
  directions, editor-tab/split inheritance, remote-report fallback, disabled inheritance, initial
  status-panel and add-button inheritance, startup OSC 8 denial, and live OSC 8 changes across
  editor and status terminals. Linux and MinGW ecode release builds passed. Assistant terminal
  startup uses the same policies; no external agent session was launched for validation.
- ecode allocation/layout review: `TerminalConfig` grows from 304 to 312 bytes (8-byte alignment)
  for three grouped boolean flags. No terminal-manager state or per-frame work was added. Scene
  traversal uses synchronous reference captures; launch-directory ownership is limited to terminal
  creation.
- Regression coverage includes fragmented BEL/ST sequences, URI semicolons/Unicode, SGR reset,
  edits/erasures, both halves of wide cells, 10,000-line history/reflow, sustained churn, all 32,767
  live IDs exhausted without aliasing, retained snapshots, policy updates and mouse-report balance.
- Menu teardown in the test is synchronous; it does not wait or advance a simulated frame clock.
- Hover checks cover policy changes, snapshot replacement, wide cells, unsupported schemes,
  pointer bounds and modifier matching. A focused headless test verifies the hand cursor and its
  restoration on modifier release. Headless visual checks cover direct and framebuffer rendering,
  long-URI truncation and exact pixel restoration on Ctrl release. With an application underline
  already present, hover changes only the adjacent pixel row, avoiding a line through descenders.
- The pinned formatter and `git diff --check` pass. All locale XML files parse and contain the keys.

## Performance review: still open

An isolated benchmark compared original HEAD terminal sources with the implementation, using the
same GCC `-O3 -g -std=c++20 -fPIC` library flags and the configured release core library. Three
alternating baseline/current runs were pinned to the same CPU; each throughput result is itself
the median of seven trials. Workloads use a 120×40 terminal, 10,000 history rows and 180,000 lines.
The linked stream adds an explicit-ID span every twentieth line. Original code discards OSC 8.

| Parser workload | Original | Implementation | Change |
| --- | ---: | ---: | ---: |
| No OSC 8 | 131.61 MiB/s | 125.11 MiB/s | **−4.9%** |
| Periodic OSC 8 | 130.55 MiB/s | 125.00 MiB/s | −4.3% |

The no-link result does **not** establish the plan's within-noise performance acceptance criterion.
It needs further profiling/optimization and measurement on representative real output before release.
Run-to-run variation was substantial: no-link run medians ranged from 126.24–131.63 MiB/s for the
original and 112.15–128.31 MiB/s for the implementation. Earlier controlled runs showed about 3.6%
overhead; do not interpret the latest percentage as a precise universal cost.
Printable attributes are cached when SGR/cursor/link state changes, so the ASCII path performs no
URI lookup, allocation or per-character ID packing.

The snapshot check measures an ordered focus command through publication/observation, including
thread scheduling; it is not a pure `drawEnd()` measurement or a GPU/frame-latency measurement.
The dense case assigns a distinct URI to every cell in half the grid (960/2,400/9,600 URIs).
Values below are medians of the three controlled runs, in microseconds.

| Grid | Original, no links | Implementation, no links | Implementation, dense links |
| --- | ---: | ---: | ---: |
| 80×24 | 7.24 | 7.96 | 45.42 |
| 120×40 | 13.98 | 15.21 | 110.06 |
| 240×80 | 48.38 | 52.95 | 438.76 |

The standalone benchmark source and raw transcripts are retained in `/tmp/eterm-perf/` on the
development host. These measurements do not cover hover/menu/click frame latency or long-term
registry memory profiling.

## Allocation and layout review

Verified Linux x86_64 sizes: `TerminalGlyph` 16, `TerminalHyperlink` 64, `VisibleHyperlink` 40,
`TerminalSnapshot` 320 (original 296), `TerminalSessionOptions` 2 and `TerminalConfig` 168 bytes.
No packed/misaligned layout or extra per-cell buffer was introduced. Existing factory signatures
remain available; initial parser options use additional overloads.

Registry URI/explicit-ID bytes are capped at 8 MiB; IDs are capped at 32,767. Registry descriptors,
the free-ID vector and the explicit-ID hash index are also bounded by that ID count. Mark/sweep
uses a 4 KiB stack bitset, runs every 1,024 allocation attempts or after five seconds with metadata,
and never runs per frame. Snapshot deduplication uses another 4 KiB stack bitset and a small vector
of distinct IDs, skipping the cell scan entirely when the registry has no metadata.

Registry strings own parser-buffer slices. Snapshot URI copies are required because the worker can
reclaim/reuse IDs while the UI retains older snapshots. Menu/pressed-link strings similarly own
their targets across popup/input lifetimes. New callbacks capture only an owner pointer or booleans;
there are no asynchronous captures of large buffers. Factory forwarding moves owned program and
argument temporaries. Clipboard event text must own its decoded data until the UI drains it.

The hover addition changes `TerminalDisplay` from 880 to 888 bytes on Linux x86_64; `UITerminal`
remains 2,000 bytes. Compiler layout data verifies the lazily allocated `HyperlinkHover` cache at
488 bytes and its row span at 12 bytes. Unhovered terminals allocate no hover cache. URI text and
row spans are rebuilt only when the hovered link or snapshot generation changes; repeated motion
within the same link uses borrowed snapshot lookups without URI copies or worker commands. The
cached preview owns its text across snapshot replacement and moves newly decoded strings into
`Text`; font/width changes invalidate that geometry. Drawing the open-edge border uses a local
borrowed lambda with no heap-backed callback or async handoff. Decorations render after the
framebuffer, so changing hover never alters terminal glyphs, SGR or cached terminal contents.

Codex's label-only link rendering depends on its own terminal-identity allowlist; `eterm` remains
`TERM_PROGRAM=eterm`. No Codex compatibility patch or terminal-identity impersonation was added.

## Remaining release checks

- Investigate the measured no-link throughput difference and quantify representative few-link
  snapshot costs, registry memory under long churn, and UI frame latency.
- Resolve or establish a clean baseline for the two full-suite TLS failures.
- Run native macOS and Windows GUI checks; Wine does not replace hardware/desktop validation.
- Exercise real shells/prompts, Vim/Neovim, tmux, Codex/Claude interfaces and external URI launching
  on the target desktops. Kitty/selection/search regressions pass the automated terminal suite.
- Native FreeBSD/Haiku builds and working-directory behavior remain unverified.

Session restoration, vertical tabs, multi-window support and standalone release packaging remain
outside this implementation, as specified in the supplied plan.
