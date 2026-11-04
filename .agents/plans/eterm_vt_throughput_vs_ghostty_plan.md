# eterm VT Throughput vs Ghostty Plan - 2026-10-30

## Status

Items 1-5 are committed in `cef605021`; grapheme phase 1 is committed in `b4b4aa8c1`.
Grapheme shaping and review fixes are committed, including mode-2027 extension batching in
`5bd79ba75`. The latest controlled-scrollback baseline (2026-11-03) pins Ghostty `91973f2b2`
and supersedes the older comparison with unlike history budgets. Runtime-dispatched SIMD
row clears are committed in `620c74bfc` and four-cell narrow writes in `a996bb737`.
Equal-width Unicode batching and bounded SIMD UTF-8 decoding are committed in `ad9365499`.
Local tuning increases the bounded decode buffer to 2,048 slots;
measurements and validation are recorded below. The mode-controlled follow-up explains the
apparent regression caused by comparing different reset defaults.

The latest head-to-head is the [refreshed workload matrix](#refreshed-workload-matrix-2026-11-04)
after adaptive decoding, snapshot reuse, reply budgeting and blank-row tracking. Older tables
below retain their original binaries and measurements.

Context: the OSC 8 hyperlink / OSC 7 working-directory / OSC 52 policy work (see
`docs/articles/eterm_integration.md`) made eterm comparable with Ghostty's feature set, so the next
step is comparing raw VT processing throughput. That work also produced these fixes, which are part
of the same local change set:

- Hyperlink GC no longer reads past narrowed history lines (ASan-confirmed heap over-read).
- GC runs only when the registry is exhausted (bounded to a ~20% duty cycle) or after 5 s with new
  output, never per N opens and never on idle terminals. A distinct link on every line went from
  ~43 to ~101 MiB/s on x86_64 and ~48 to ~126 MiB/s on ARM64.
- Link URIs are shared immutable strings (`std::shared_ptr<const std::string>`) between the worker
  registry, snapshots and the hover cache.
- `CSI 5 n` (DSR operating status) now replies `CSI 0 n`. Without it, `kitten __benchmark__` hangs
  forever inside eterm.

## Goal

Close the parser-level throughput gap with Ghostty on the six `kitten __benchmark__` workloads,
without dropping or skipping input. Every byte must still update terminal state.

The reference comparison is Mitchell Hashimoto's "VT processing throughput" chart (Ghostty vs
Velocitty): headless processing of the six kitten workloads, Ghostty measured with
`ghostty-bench +terminal-stream`, 120x80 cells, median of 3 runs, M4 Max. His published Ghostty
numbers: ASCII 618.6 MB/s, Unicode 914.7, Unique Unicode 155.6, CSI 224.5, Images 4.65 GB/s, Long
escape codes 13.40 GB/s. Velocitty's numbers were invalid because it parsed only the last 64 KiB
per tick and discarded ~99% of its input. Our harness must never repeat that mistake; see the
completeness check below.

## Baseline (Linux x86_64, Ryzen 9 3900X, `powersave` governor)

Identical captured kitten streams (66-76 MB each), whole-process hyperfine timing for both,
`--warmup 1 --runs 3`, pinned with `taskset -c 2`, MB = 10^6 bytes.

| Workload | Ghostty main (fbed180) MB/s | eterm MB/s | Gap |
| --- | ---: | ---: | ---: |
| ASCII | 333 | 122 | 2.7x |
| Unicode | 468 | 90 | 5.2x |
| Unique Unicode | 70 | 73 | not comparable |
| CSI | 106 | 50 | 2.1x |
| Images | 3,708 | 787 | 4.7x |
| Long escape codes | 7,233 | 161 | 45x |

Unique Unicode is not comparable: eterm does not cluster graphemes. A width-0 combining mark is
written into the cell at the cursor instead of attaching to the previous base character, so eterm
does less work than Ghostty, which stores the whole grapheme per cell. Ghostty's stream benchmark
does execute kitty graphics commands (`stream_terminal.zig` `apcEnd`), so Images is comparable.

## Baseline (macOS, Apple M2 Pro)

Fresh captures with kitten 0.49.2 (66-75 MB each), Ghostty main `c911f96`, eterm at `9eee2afca`,
hyperfine `--warmup 1 --runs 5`, two rounds with the command order swapped (agreed within ~2%).
Work tree: `~/programming/vtbench` (`ghostty/`, `data/*.vt`, `capture.py`, `vtbench.cpp`,
`build-vtbench.sh`, `report.py`).

| Workload | Ghostty MB/s | eterm MB/s | Gap |
| --- | ---: | ---: | ---: |
| ASCII | 436 | 125 | 3.5x |
| Unicode | 721 | 162 | 4.5x |
| Unique Unicode | 102 | 140 | not comparable |
| CSI | 153 | 67 | 2.3x |
| Images | 2,399 | 646 | 3.7x |
| Long escape codes | 4,157 | 145 | 28.7x |

All six eterm runs pass the completeness check.

## Where eterm spends the time (Superluminal, Linux release build)

- **ASCII:** `twrite` -> `tputc` per character is ~86% of the run (`tputc` 294 ms exclusive,
  `twrite` 77 ms, of ~650 ms). The existing `tputc` ASCII fast path still pays per-character
  dispatch, UTF-8 decode, selection checks and cursor bookkeeping.
- **Unicode:** the same per-character path, plus every non-ASCII character takes the slow path:
  `tsetchar` 182 ms, `tmoveto` 67 ms, `twrite` UTF-8 decode 150 ms, and `tputc` 346 ms including
  `wcwidth()` and `utf8encode()` per character.
- **Long escape codes:** every byte of each 8 KB `OSC 6` string goes through `tputc` and is
  appended to `mStrescseq` one byte at a time (`tputc` 238 ms + `twrite` 146 ms for 66 MB). Only
  kitty APC payloads have a bulk path today (`findKittyAPCControl()` in `twrite`).
- **Images:** scalar `EE::System::Base64::decode` (41 ms) plus page-fault churn
  (`__zap_vma_range`, `folio_zero_user`) from allocating and freeing a 4 MiB image buffer on every
  repetition.
- **CSI:** per-byte CSI parsing and dispatch. Smallest gap and least obvious win.

## What Ghostty does (src/terminal/stream.zig, Terminal.zig)

- In the ground state, `nextSliceCapped()` SIMD-decodes UTF-8 up to the next ESC into a 4096
  codepoint buffer (`simd.vt.utf8DecodeUntilControlSeq`).
- It splits that buffer into printable runs with a vector scan (a codepoint is a C0/C1 control
  exactly when `(cp & ~0x9F) == 0`) and calls `handler.vt(.print_slice, ...)` once per run.
- `Terminal.printSlice()` takes a fast path when the state allows it (main screen, wraparound on, no
  insert mode, no single shift, no active hyperlink) and writes narrow/wide cells directly per row;
  anything unusual falls back to per-codepoint `print()`.
- `consumeAllEscapes()` handles back-to-back escape sequences without returning to the scalar loop.
- String-type sequences (OSC/DCS/APC) are consumed in bulk up to their terminator with SIMD scans
  for terminator bytes (`0x07`, `0x18`, `0x1A`, `0x1B`, and C1 in non-UTF-8 mode).
- Kitty graphics payloads are base64-decoded with SIMD in bulk.
- Since 1.4.0, the PTY is drained on one thread and parsed on another (macOS reads are capped at
  ~1 KiB per read). eterm already parses on a worker thread separate from the UI.

## Work items (in recommended order)

### 1. Bulk OSC/DCS/APC string bodies

Generalize the kitty APC bulk path in `TerminalEmulator::twrite()` to every `ESC_STR` sequence:
scan for the next byte that can end or abort the string (`BEL`, `ESC`, `CAN`, `SUB`, and C1 `ST`
`0x9C` only when not in UTF-8 mode; check `strhandle()`/`tputc()` for the exact set) and append the
run to `mStrescseq` at once, respecting `STR_BUF_SIZE`/discard limits and the existing size caps.

- Must keep: fragmented terminators across reads, `ESC \` vs bare `ESC` + other byte, discarded
  oversized strings, OSC 8/7/52 handling (all parse after the string completes).
- Expected: long escape codes from ~160 MB/s to multiple GB/s.
- Tests: existing OSC 8/52/7 and kitty tests cover most edges; add one for a long OSC split across
  many reads with the terminator split too.

**Done.** What landed in `terminalemulator.cpp`:

- `findStringSequenceStop()` (AVX2 / NEON / scalar) replaces the kitty-only `findKittyAPCControl()`
  and serves every `ESC_STR` type. Stops: `BEL`, `CAN`, `SUB`, `ESC`, plus every byte >= 0x80 in
  UTF-8 mode (so multi-byte characters, malformed input and the two-byte C1 `U+009C` go through
  `utf8decode()`/`tputc()`), or raw C1 0x80-0x9F otherwise. This also fixed a latent kitty-path bug:
  it copied a `0xC2` lead byte in bulk and then decoded the following `0x9C` alone, so a UTF-8
  encoded ST did not end the APC. The NEON scanner classifies C0 stops with one `vqtbl2q_u8` and
  scans 64 bytes per iteration.
- `strappend()` is the single size-limit/growth path for `tputc()` and the bulk path. An oversized
  string keeps the prefix that fits (at most limit - 1 bytes), so a discarded `OSC 8`/`OSC 7` is
  still recognized by `strhandle()`. The first APC byte stays on the slow path because it selects
  the kitty size limit.
- `strparse()` splits with `memchr()` and checks for embedded NUL only up to the last separator
  used (differentially tested against the old loop on 5M random inputs).
- C1 ST (`0x9C`, raw or decoded) now dispatches the pending string like `ESC \`; it used to abort
  it silently.
- Tests: `string_sequence_bodies_survive_fragmented_reads_and_terminators`,
  `oversized_osc_hyperlink_clears_the_active_link`.
- Verified: eterm tests 147/147 on macOS ARM64 and on Linux x86_64 (Docker `eepp-linux-amd64`,
  Rosetta; it reports no AVX2, so that run covers the scalar scanner). The SIMD scanners are
  differentially tested against the scalar one by `~/programming/vtbench/scan` (it extracts the
  scanner from the source): NEON natively, AVX2 as a static x86_64 binary under arm64
  `qemu-x86_64 -cpu max` (SIGILL under `-cpu qemu64` confirms the AVX2 path runs). Do not run
  `qemu-x86_64` inside the amd64 (Rosetta) container: it hangs.

Results, M2 Pro (whole process, streamed harness):

| Workload | HEAD MB/s | Item 1 MB/s | Ghostty MB/s | Gap now |
| --- | ---: | ---: | ---: | ---: |
| Long escape codes | 150 | 2,726 | 3,629 | 1.33x |
| Images | 748 | 1,296 | 2,222 | 1.71x |
| ASCII / Unicode / CSI | unchanged | unchanged | | |

In-process parse rate on long escape codes is ~5.4 GB/s. What remains is fixed process cost
(startup ~9.7 ms vs Ghostty's ~7.9 ms), kernel `read()` copies, and `strparse()` on unknown OSCs.
Skipping `strparse()` for OSC numbers `strhandle()` does not handle would close most of the rest, but
needs a handled-number check that does not duplicate the dispatch switch.

### 2. Printable-run fast path for ASCII

In `twrite()`, when not in a string/escape state, not `MODE_PRINT`/`MODE_INSERT`, charset is not
`CS_GRAPHIC0` and no special per-cell state applies, SIMD-scan for the next byte that is `< 0x20`,
`0x7F`, `>= 0x80` or ESC. Write that run into the current line in row-sized chunks: one
selection/dirty/wrap decision per chunk, then fill glyphs with `mPrintAttributes` (keeps the OSC 8
link ID). Handle `CURSOR_WRAPNEXT` and soft wrap exactly like `tputc()`; fall back to `tputc()` for
the byte that stops the run.

- Watch: wide-cell repair when overwriting half of a wide glyph (`tsetchar()` handles it per cell),
  kitty placeholder metadata, `mTerm.lastc` for `REP`, selection clearing, `ATTR_WRAP` marking,
  `mDirty`/`mTerm.dirty[y]`.
- Expected: roughly 2x on ASCII.

**Done.** `twrite()` tests the current byte inline, then `findPrintableAsciiEnd()` (AVX2 / NEON /
scalar, 0x20-0x7E) finds the run and `tputprintable()` writes it one row segment at a time when
`canPutPrintable()` holds (ground state, `MODE_WRAP`, no `MODE_INSERT`/`MODE_PRINT`, charset not DEC
graphics). It keeps `tputc()`'s exact semantics: wrap via `ATTR_WRAP` + `tnewline()`, the selection
test at the cursor cell *before* wrapping, one `tmoveto()` per segment then `CURSOR_WRAPNEXT` at the
edge, `lastc`, dirty marking. Wide cells, wide dummies and kitty placeholders stop the segment and
take `tputc()`; so does origin mode with the cursor outside the scroll region. The per-character
ASCII fast path in `tputc()` is gone (it now only fired with wraparound off, and it overwrote
placeholder cells without dropping their metadata).

- REP (`CSI b`) of printable ASCII goes through `tputprintable()` in 256-byte stack chunks. The
  kitten CSI stream is dominated by `CSI 100 b`; without this CSI regressed 27%.
- x86_64 scanners are resolved once into file-scope function pointers (like
  `String::stripAnsiCodes()`, but outside the function so calls pay no static guard).
- Unicode / Unique Unicode measure 2-6% below HEAD with the default link order and within 1% (or
  ahead) with the object order reversed: code placement, not work.
- Verified: final screens identical to HEAD on the six kitten streams and on 60 random streams from
  `~/programming/vtbench/fuzz/gen.py` (wraps, wide cells, DECAWM, IRM, origin mode + DECSTBM, REP,
  DEC graphics, OSC 8, alt screen); scanner differential test on NEON and AVX2; eterm tests 149/149
  on macOS and Linux x86_64. New tests: `printable_runs_clear_only_overwritten_selections`,
  `printable_runs_replace_kitty_placeholders`.

Results after items 1+2 (superseded by the table under item 3), M2 Pro (whole process, MB/s):

| Workload | HEAD | Now | Ghostty | Gap now |
| --- | ---: | ---: | ---: | ---: |
| ASCII | 127 | 288 | 437 | 1.51x |
| Unicode | 168 | 158 | 721 | 4.58x |
| Unique Unicode | 143 | 140 | 102 | not comparable |
| CSI | 68 | 90 | 153 | 1.69x |
| Images | 715 | 1,186 | 2,301 | 1.94x |
| Long escape codes | 150 | 2,953 | 4,122 | 1.40x |

### 3. Bulk UTF-8 decode, faster widths, batched non-ASCII runs

Use simdutf (already vendored, `simdutf-static`) to decode runs up to the next control/ESC, then
classify widths with a two-stage lookup table instead of `wcwidth()`'s current lookup. Batch runs
of width-1 and width-2 codepoints like item 2; send width-0 and anything special to the slow path.
Avoid the per-character `utf8encode()` in `tputc()`; it only matters for `MODE_PRINT`.

- Expected: most of the 5.2x Unicode gap.

**Done** (scalar decode; simdutf was not needed yet). The item 2 writer became
`template <typename Run> tputrun()`, shared by `AsciiRun` (the bytes, `Narrow`) and `UnicodeRun`
(512 decoded codepoints with width and cell mode, on the stack of the out-of-line `tpututf8()`).

- `decodeUnicodeRun()` accepts printable ASCII and exactly the well-formed sequences
  `utf8decode()` accepts; it stops at controls, C1 (U+0080-009F), malformed or incomplete input
  and the kitty placeholder U+10EEEE.
- Widths come from `GlyphProperties`: a 64 KiB BMP table (width with `tputc()`'s negative-width
  mapping, emoji and box-drawing flags), built lazily in a function-local static (~0.17 ms, first
  non-ASCII character only); non-BMP codepoints use `GlyphProperties::compute()`, which also fills
  the table. That was worth ~25% on Unicode. `is_emoji()` now tests its two blocks directly.
- Zero-width characters stay in runs: `tputc()` writes them at the cursor without advancing
  (the item 6 grapheme behavior is unchanged). While a kitty placeholder is pending, characters
  go through `tputc()` so diacritics still attach.
- Wide characters match `tputc()`: one that does not fit in the last column wraps like a pending
  wrap; the dummy cell gets `\0`, `ATTR_WDUMMY` and the link ID but keeps its colors; the cursor
  stays on a wide character that reaches the edge. Wide/dummy/placeholder cells under either half
  fall back to `tputc()`. Known difference: after a wrap onto such a cell, the fallback also tests
  the selection at the new cell (it can only clear a selection covering exactly that cell).
- `tputrun()` copies `mTerm.col`, `mPrintAttributes` and the selection state into locals: glyph
  stores are `uint32_t` and may alias `mTerm`'s `int` fields, which forced reloads per cell (~10%
  on ASCII). Narrow runs test the selection in one pass before the fill.
- `hasSelection()` is inline in the header.
- Codegen sensitivity: adding the Unicode path made `tputc()` ~15% slower on CSI-heavy input even
  though CSI never reaches it (inlining `wcwidth()` into `tputc()` changed with a second caller). The
  robust fix was the CSI parameter collection below, not compiler tuning.
- Verified: kitten screens identical to HEAD; 80 random streams identical, including combining
  marks, emoji, box drawing, encoded C1, malformed UTF-8 (overlong, surrogates, stray and truncated
  sequences) and wide characters at the last column, with read sizes of 1-7, 4093 and 64 KiB
  (`CHUNK=` in the harness); eterm tests 150/150 on macOS and Linux x86_64. New test:
  `unicode_runs_keep_wide_cell_and_wrap_semantics`.

Results after items 1-3 and the CSI collection, M2 Pro (whole process, MB/s):

| Workload | HEAD | Now | Ghostty | Gap now |
| --- | ---: | ---: | ---: | ---: |
| ASCII | 125 | 284 | 436 | 1.53x |
| Unicode | 167 | 397 | 723 | 1.82x |
| Unique Unicode | 143 | 368 | 103 | not comparable |
| CSI | 68 | 100 | 153 | 1.53x |
| Images | 764 | 1,360 | 2,428 | 1.78x |
| Long escape codes | 151 | 2,968 | 4,192 | 1.41x |

### 4. Images: SIMD base64 and buffer reuse

Profile `KittyGraphicsProtocol` transmit: replace scalar `Base64::decode` on the hot path with a
SIMD decoder (or simdutf's base64), and reuse the decode/image buffer across transmissions of the
same size instead of allocating and freeing 4 MiB each time.

**Done.**

- `EE::System::Base64::decode()` (eepp core) gained SIMD block decoders (NEON: `vld4q`/two
  `vqtbl4q` lookups/`vst3q`, 64 digits per block; AVX2: Muła's pshufb validation as in
  aklomp/base64, 32 digits per block), resolved once into a file-scope function pointer. They
  decode only whole blocks of valid digits and stop at the first block with padding, whitespace or
  an invalid byte; the scalar decoders finish from that 4-digit-group boundary, so all three modes
  return exactly what they returned before. simdutf's base64 was not used: it always skips
  whitespace (WHATWG forgiving base64), and kitty/OSC 52 need strict rejection.
- `KittyGraphicsProtocol`: deleted images and `reset()` return uniquely owned pixel buffers to
  `mDecodedScratch` (`recyclePixels()`); the transfer reserve has 4 bytes of slack so the final
  chunk no longer reallocates and copies the whole image; `strreset()` retains up to 256 KiB so a
  128 KiB chunk's buffer is not shrunk and regrown per chunk.
- Verified: new `base64_tests.cpp` (round trips in all modes, invalid bytes/whitespace at every
  position, truncation), written and passed against the scalar decoder first; a differential
  check of SIMD+scalar against pure scalar on 180k random decodes (NEON native; AVX2 under arm64
  `qemu-x86_64 -cpu max`, and `-cpu qemu64` confirming the scalar fallback); ArchiveProvider, SFTP,
  Zip* and FontRendering suites; eterm 150/150 on macOS and Linux x86_64.
- The vtbench HEAD binary links the same `libeepp.dylib`, so HEAD numbers measured after this
  change include the SIMD decoder; compare Images against the 764 MB/s baseline.

### 5. CSI dispatch (only after 1-4)

Profile first. Candidates: consuming back-to-back escape sequences without returning through
`twrite()`'s per-byte loop, and cheaper parameter parsing in `csiparse()`.

**Done.** `twrite()` handles whole CSI sequences with `tputc()`'s exact steps: `ESC [` from the
ground state (what `tcontrolcode()` + `eschandle()` do; not with `show_ctrl`), parameter and
intermediate bytes (0x20-0x3F, stopping one byte before the length at which `tputc()` dispatches),
and the final byte (0x40-0x7E) appended and dispatched. Not with `MODE_PRINT`; controls and other
bytes still take `tputc()`. Also:

- `csireset()` no longer clears the 512-byte buffer (read only up to `len`); `arg`/`sep` are still
  cleared in full because `DEFAULT()` writes past `narg`.
- `csiparse()` parses plain decimals by hand (`parseCsiNumber()`); signs, leading whitespace, no
  digits or 19+ digits go to `strtol()` from the start, so overflow behaves as before.
- `tclearregion()`'s fill fast path now covers any range when `skip_clear || !hasSelection()`
  (EL/ED on partial rows), not only full-row scroll clears.
- REP (`CSI b`) of printable ASCII goes through `tputrun()`.
- Verified: 100 random streams identical to HEAD including intermediates, private prefixes,
  subparameters, signs, leading whitespace, 19-25 digit numbers, 40 and 300 parameters
  (buffer limit), controls inside sequences and sequences cut by ESC, with 1-byte to 64 KiB reads.

CSI: 68 to 197 MB/s (Ghostty 153).

### Notes for later

- Row clears: each scroll clears a recycled 120-cell row (`tclearregion()` fast path, ~1.9 KB at
  16 bytes per cell) that is then mostly overwritten; on ASCII it costs ~70% as much as writing the
  text. Ghostty's cells are 8 bytes. Not a fairness problem: eterm measured the same with 10 and
  10000 history lines on the M2 (Ghostty's bench uses its default 10 kB scrollback; harness option
  `HISTORY=`).
- In-loop paths in `twrite()` restart with `charsize = 0; continue;` so the next byte can take any
  path; each must consume at least one byte.

### Results after items 1-5

M2 Pro, whole process, MB/s. "HEAD" is `9eee2afca`; for Images the HEAD figure is from before the
SIMD Base64 decoder landed in the shared `libeepp.dylib`.

| Workload | HEAD | Now | Ghostty | Gap now |
| --- | ---: | ---: | ---: | ---: |
| ASCII | 127 | 290 | 442 | 1.52x |
| Unicode | 168 | 398 | 724 | 1.82x |
| Unique Unicode | 144 | 372 | 102 | not comparable |
| CSI | 69 | 201 | 155 | eterm 1.29x ahead |
| Images | 764 | 2,243 | 2,231 | parity |
| Long escape codes | 152 | 3,027 | 4,010 | 1.32x |

Structural option for the remaining ASCII/Unicode gap: Ghostty's cell is a packed `u64`
(21-bit codepoint, 16-bit style ID into a deduplicated, reference-counted per-page style set,
wide/hyperlink/grapheme flags with side tables), half of `TerminalGlyph`'s 16 bytes. That halves
the row-clear and fill traffic but changes every reader (renderer, history/reflow, snapshots,
selection, search) and the public `TerminalGlyph` ABI: a separate project.

### Follow-up candidates (review, 2026-10-31)

- REP through a `RepeatRun` (one character, any count) instead of a 256-byte stack buffer: within
  noise on CSI (196 vs 193 MB/s), kept because it is simpler.
- `UnicodeRun` stores the `GlyphProperties` byte per codepoint (5 bytes per character instead of 9)
  and derives width and mode bits in the writer: Unicode +3% (389 to 400 MB/s, both link orders),
  Unique Unicode flat. Item 6 can carry its "joins" bit in the same byte.
- Profiles after both: Unicode is writer 46%, decode 40%, row clear 13%; ASCII is scan + writer
  (inlined into `twrite()`) ~60%, row clear 34%.
- simdutf `convert_utf8_to_utf32_with_errors()` decode (exact, screens identical): Unicode -9%,
  Unique Unicode -25%, because it converts a whole 512-byte span and discards everything after the
  first newline or control byte. Rejected; it would also need an exported eepp API for eterm to
  link simdutf on Windows.
- Not done: skipping `strparse()` for unhandled OSCs (mostly helps the synthetic long-escape
  benchmark), removing `decodeBase64()`'s zero fill (minor after buffer recycling), fusing row
  clears with the following writes (largest ASCII lever; needs its own design because snapshots,
  selection, search and history observe the cleared row).

### 6. Grapheme clustering (correctness, not speed)

Attach width-0 codepoints (combining marks, ZWJ sequences, variation selectors) to the previous
cell. Needed before the Unique Unicode workload means anything, and important for real text. Design
storage carefully: `TerminalGlyph` must stay 16 bytes, so extra codepoints need a side table keyed
like the hyperlink registry (IDs, mark/sweep), not per-cell heap data.

**Phase 1 done (storage, segmentation, mode 2027, text).** Renderer shaping is phase 2.

- Storage: a cluster cell's codepoint is `GLYPH_CLUSTER_BASE` (0x110000) + ID; the worker
  registry (`mClusters`, `TerminalCluster` = `shared_ptr<const std::u32string>`, base first)
  interns identical clusters through a content-hash index. Up to 65535 live IDs and 32
  codepoints per cluster; beyond that codepoints are dropped and the base kept. One mark pass
  (`collectGlyphReferences()`, renamed from `collectHyperlinks()`) reclaims link and cluster IDs
  with the same triggers; a full registry retries collection every 256 attaches instead of reading
  the clock per attach.
- Rules (Ghostty's): without mode 2027 zero-width codepoints attach to the previous character
  (cursor cell with a pending wrap, else the one to the left; a wide dummy points at its base),
  VS15/VS16 only after Extended_Pictographic, nothing to attach to drops them. With mode 2027 a
  codepoint above U+00FF joins when `String::isGraphemeBoundary()` finds no boundary; VS16 widens
  and VS15 narrows (base must have an emoji property), a codepoint with its own width widens the
  cluster, a widening cluster in the last column moves to the next row, and zero-width
  codepoints at a boundary are dropped. `tattach()` interns before changing any layout.
- eepp `String` gained `graphemeBreakProperty()`, `isExtendedPictographic()` and
  `hasEmojiProperty()` (cpp-unicodelib). Its emoji table keeps one property per codepoint (the
  last listed), so "has the Emoji property" cannot be read exactly; any emoji property is used as
  the variation-sequence base check. `Font::isEmojiCodePoint()` stays a font-fallback heuristic.
- Fast paths: `GlyphProperties` gains ZeroWidth, Joins (Extend, ZWJ, SpacingMark, RI, Hangul V/T)
  and Attracts (Prepend, ZWJ, Hangul L) bits. The run decoder (templated on the mode) stops at
  zero-width codepoints and, in mode 2027, at Joins or after Attracts; `tpututf8()` checks the
  previous cell before a run's first codepoint above U+00FF. Any other pair is a UAX #29 boundary,
  so precomposed Hangul and CJK stay on the fast path. Without mode 2027 a burst of zero-width
  codepoints attaches in one `tattachZeroWidth()` call (one intern).
- Readers: `getSelection()` builds a `std::string` (the old `getsel()` assumed 4 bytes per cell,
  which clusters would overflow; removed), dump and word snap expand or use the base, search maps
  every cluster codepoint to its cell, snapshots publish `visibleClusters` (`findCluster()`,
  `glyphBaseCodepoint()`), and the display draws the base codepoint for now.
- DECSET/DECRST 2027 and DECRQM (`CSI ? Ps $ p`, `CSI Ps $ p`) for the modes `tsetmode()`
  implements.
- Verified: eterm 156/156 and String tests on macOS; kitten screens and 60 fuzz streams without
  zero-width input identical to the previous commit; batched attaches identical to per-codepoint
  attaches on 60 streams (compared by cluster codepoints, IDs differ); 30 cluster-heavy streams
  (mode 2027 toggles, ZWJ, flags, VS, Indic, jamo, Prepend) clean under ASan + UBSan.
- Historical throughput (M2 Pro, MB/s): Unique Unicode 372 -> 122, Unicode 404 -> 370. These
  captures contain RIS resets, so the bulk of the committed parser's run uses its default mode
  2027 setting (off), even if the file starts with DECSET 2027. They do not measure full mode 2027
  processing. See the mode-controlled follow-up below before comparing with the working tree.

**Phase 2:** shape cluster cells through `TextLayout` (HarfBuzz with font fallback and its LRU cache)
and draw them with `getGlyphDrawableFromGlyphIndex()`; clusters that shape to several glyphs need
quads beyond the renderer's one-per-cell VBO slot.

### Mode-controlled follow-up (2026-10-31)

The apparent Unique Unicode regression from ~121 to ~47 MB/s was a benchmark-state mismatch.
The captured Unicode and Unique Unicode files contain three RIS (`ESC c`) resets: the first is
at byte 12,629 / 12,621, before almost all the payload, and two follow the payload. A single
DECSET/DECRST 2027 prefix is undone by the first reset. Committed `b4b4aa8c1` resets mode 2027 off;
the working tree resets it to its configured default, on. Thus both of the earlier "on" and "off"
tests mostly measured each binary's reset default. Ghostty's earlier mode comparison was affected
by the same reset trap.

Fresh release harnesses for HEAD and the working tree, sharing the same current `libeepp.dylib`;
M2 Pro; whole-process hyperfine, one warmup and five measured runs. These numbers keep mode 2027
**on after every reset** for all three terminals, using identical bytes:

| Workload (MB/s) | HEAD | Working tree | Ghostty |
| --- | ---: | ---: | ---: |
| ASCII | 298.0 | 290.4 | 457.2 |
| Unicode | 347.9 | 344.1 | 665.5 |
| Unique Unicode | 48.9 | 48.8 | 79.3 |
| CSI | 202.3 | 201.9 | 159.1 |
| Images | 2,354.3 | 2,356.7 | 2,553.6 |
| Long escape codes | 3,121.8 | 3,100.6 | 4,564.8 |

With mode 2027 held **off after every reset**:

| Workload (MB/s) | HEAD | Working tree | Ghostty |
| --- | ---: | ---: | ---: |
| Unicode | 380.7 | 370.6 | 740.4 |
| Unique Unicode | 120.8 | 122.4 | 103.4 |

All eterm runs passed the harness's byte-consumption and six-DSR-response completeness checks.
The large reported regression disappears at equal modes. Small differences elsewhere in this
round are not enough to attribute a code regression on Apple Silicon; use alternating rounds and
different link orders before acting on them (see measurement pitfalls below).

For these captured VT streams, generate mode-controlled copies without changing the originals:

```python
from pathlib import Path

source = Path("data")
for enabled in (False, True):
    target = Path("data-mode-on" if enabled else "data-mode-off")
    target.mkdir(exist_ok=True)
    mode = b"\x1b[?2027h" if enabled else b"\x1b[?2027l"
    for name in ("ascii", "unicode", "unique_unicode", "csi", "images", "long_escape_codes"):
        captured = (source / (name + ".vt")).read_bytes()
        controlled = mode + captured.replace(b"\x1bc", b"\x1bc" + mode)
        assert controlled.count(b"\x1bc" + mode) == captured.count(b"\x1bc")
        (target / (name + ".vt")).write_bytes(controlled)
```

Original tools, captures and the previous agent's JSON are in `~/programming/vtbench/`.
This follow-up's fresh binaries, controlled captures, `final-*.json`, `measure.py`, `report.md`
and `profile.txt` are in `/tmp/negen-vtbench/`.

A separate three-second `sample` of mode-on Unique Unicode points to `tattach()` and
`internCluster()` (allocation, hashing and reference collection) as the main cost. Mode-off
processing batches zero-width bursts; mode-on processing interns intermediate cluster prefixes
one codepoint at a time. This motivated the batching experiment below. No parser changes were
needed to resolve the apparent regression caused by the different reset defaults.

### Batched mode-2027 extensions (2026-10-31)

Implemented on top of `482156091`: the existing zero-width burst decoder and attachment helper
now also batch mode-2027 codepoints whose width is zero and whose grapheme-break property is
Extend, except VS15/VS16. One spare bit in `GlyphProperties` classifies them in the existing BMP
table. The first boundary is checked once (GB4 must still reject Extend after a Control base such
as soft hyphen); GB9 then joins the rest without changing width. Only the final cluster is
interned, avoiding allocations and IDs for every intermediate prefix.

Width-changing selectors, ZWJ, non-Extend input, malformed/incomplete UTF-8 and pending Kitty
placeholder diacritics keep their existing handling. Clusters still retain at most 32 codepoints.
No class/cell layout or heap scratch storage changes: the path reuses the existing 32-codepoint
stack buffers and the 64 KiB properties table. Registry exhaustion retains the existing rule of
dropping extensions when no ID is available; fewer transient prefixes now compete for those IDs.

M2 Pro, fresh release binaries, mode 2027 on after every RIS, identical streams, whole-process
hyperfine with one warmup and five measured runs; MB/s:

| Workload | Before | Batched | Ghostty |
| --- | ---: | ---: | ---: |
| ASCII | 279.3 | 282.1 | 453.4 |
| Unicode | 343.1 | 365.7 | 663.2 |
| Unique Unicode | 47.6 | 121.9 | 79.3 |
| CSI | 201.4 | 200.1 | 158.4 |
| Images | 2,334.2 | 2,313.8 | 2,556.8 |
| Long escape codes | 3,104.4 | 3,083.2 | 4,519.1 |

Unique Unicode improves 2.56x; Unicode improves 6.6% in this round. The other workload differences
are around 1% or less. Mode-off controls: Unicode 376.1 -> 383.2 MB/s; Unique Unicode 122.5 ->
127.8 MB/s (do not attribute small mode-off differences to batching, since its algorithm is
unchanged there).

Validation: all 160 eterm tests pass on macOS, including a bulk-versus-byte-at-a-time regression
case for selection clearing, Control bases, the cluster cap, VS15/VS16, ZWJ, Indic conjuncts,
pending wrap, malformed UTF-8 and Kitty diacritics. Before/after resolved cells, cluster
codepoints, modes, colors and cursors match on 100 random 120 KiB streams with read sizes of 1,
7, 4093 and 65536 bytes, and at six checkpoints inside the Unicode/Unique Unicode payloads
before their cleanup/reset sequences. Every benchmark passes byte-consumption and DSR checks.

Fresh binaries, correctness driver, hyperfine JSON/logs, post-change profile and report are in
`/tmp/negen-batch/` (`compare.py`, `measure.py`, `profile-after.txt`, `report.md`). Captures are
the reset-controlled copies in `/tmp/negen-vtbench/`. Linux and Windows were not run in this
experiment.

### Controlled-scrollback baseline (2026-11-03)

Fresh optimized binaries on the Apple M2 Pro: eterm `b416667db`, Ghostty
`91973f2b29be42a2a8713406e18c56238b5f6c19` (the commit in Mitchell's November 2
Hotaru comparison). eterm uses the project's release `-O3` archive; Ghostty uses
`ReleaseFast`. Same captured kitten payloads, 120x80 cells, mode 2027 on after every
RIS; Ghostty also has `--grapheme-width=unicode`. Whole-process hyperfine medians,
one warmup and five runs; MB = 1,000,000 bytes. Builds and measurements ran serially.
Every eterm measurement passed full-byte-consumption and six-DSR-response checks.

These are still the kitten workloads, not Mitchell's newer Hotaru/ghostty-gen corpus.
The earlier refresh in `/tmp/negen-vt-refresh/` used Ghostty `c911f96` and unlike
history budgets; use this controlled baseline for subsequent comparisons instead.

Ghostty's benchmark CLI was extended only in an isolated `/tmp` copy to expose its
existing history byte/row options, plus an opt-in diagnostic. No Ghostty parser or
eterm production code changed. The isolated archive lacks Git metadata, so its
version string ends in `0000000`; the source is pinned to the full SHA above.

#### Requested history limits in physical rows

Zero means disabled in both engines. Nonzero Ghostty row caps round at page
boundaries, whereas eterm retains exactly the requested cap. A separate 50,000-row
probe verified actual retained rows: 0/0, 1,000/817, and 10,000/9,925
(eterm/Ghostty). Thus the nonzero columns match requested content limits, not exact
retained-row counts or memory budgets. Ghostty's more compact storage is retained.

| Workload | eterm 0 rows | Ghostty 0 rows | eterm 1,000 rows | Ghostty 1,000 rows | eterm 10,000 rows | Ghostty 10,000 rows |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| ASCII | 292.0 | 448.8 | 298.6 | 440.5 | 295.4 | 439.9 |
| Unicode | 367.4 | 649.8 | 364.4 | 626.6 | 360.6 | 634.3 |
| Unique Unicode | 120.7 | 82.7 | 118.7 | 82.7 | 119.4 | 82.7 |
| CSI | 195.0 | 181.5 | 193.7 | 180.9 | 195.0 | 180.5 |
| Images | 2,521.5 | 2,519.6 | 2,534.3 | 2,516.3 | 2,493.8 | 2,471.6 |
| Long escape codes | 2,842.2 | 4,501.3 | 3,053.3 | 4,474.4 | 2,905.1 | 4,226.6 |

At 10,000 requested rows, eterm/Ghostty ratios are 0.67x ASCII, 0.57x Unicode,
1.44x Unique Unicode, 1.08x CSI, 1.01x Images (effectively tied), and 0.69x long
escape codes. There is no large history-size slowdown in eterm on these captures;
small differences between cases are not enough to claim an optimization or regression.
This result does not establish history-size complexity on arbitrary workloads.

#### Ghostty byte-budget presets (separate sensitivity measurement)

These do not imply equal memory or retained content in eterm. The scrolling probe
retained 421 rows at 10,000 bytes and 47,941 at 50,000,000 bytes. Zero/nonzero
history selects different Ghostty scroll paths even when effective capacities are small.

| Workload | Ghostty 10 KB | Ghostty 50 MB |
| --- | ---: | ---: |
| ASCII | 440.3 | 440.1 |
| Unicode | 636.3 | 634.8 |
| Unique Unicode | 82.3 | 81.7 |
| CSI | 177.1 | 178.9 |
| Images | 2,468.6 | 2,494.5 |
| Long escape codes | 4,496.7 | 4,272.2 |

Artifacts: `/tmp/negen-vt-controlled/` contains `report.md`, `metadata.json` (capture
SHA-256 hashes and revisions), `summary.json`, all hyperfine JSON/logs, `probes.json`,
`probe.py`, `measure.py`, `vtbench.cpp`, the optimized binaries and source archive,
and `ghostty-benchmark-controls.patch`. Captures remain in `/tmp/negen-vtbench/`.
Run `python3 /tmp/negen-vt-controlled/measure.py` to repeat the timing matrix.

Example for the 10,000-row case:

```sh
HISTORY=10000 DYLD_LIBRARY_PATH=libs/macosx /tmp/negen-vt-controlled/eterm-current /tmp/negen-vtbench/ascii-on.vt 1
/tmp/negen-vt-controlled/ghostty-91973f2b2/zig-out/bin/ghostty-bench +terminal-stream --data=/tmp/negen-vtbench/ascii-on.vt --terminal-cols=120 --terminal-rows=80 --grapheme-width=unicode --scrollback-lines=10000
```

### Scalar clear-loop unrolling (2026-11-03)

First measured a portable four-glyph unroll in `tclearregion()`; its scalar tail
clears one to three remaining cells. The SIMD follow-up below supersedes this
inline implementation while retaining the unroll in its scalar fallback. This changes loop overhead only: clearing remains
eager, every cell receives the same blank as before, and the 16-byte glyph layout
is unchanged. No allocations or additional persistent state are introduced.

The refreshed ASCII profile had 989/3,430 top-of-stack samples in `tclearregion()`
(28.8%). Disassembly showed two 8-byte stores and a branch per cell in the old
`std::fill_n` loop. Four explicit assignments reduce branches to one per four
cells; Apple Clang still emits scalar stores. A fixed-size `memcpy` block compiled
into the same loop, so the direct assignments were retained. The optimized sample
had 861/3,442 samples in clearing (25.0%). These profiles do not establish DRAM
bandwidth saturation, and the gain does not depend on changing the cell size.

Same source revisions, captured streams, mode-2027 handling, release settings and
120x80 dimensions as the controlled baseline above; 10,000 requested history rows
in both engines, one warmup and five runs, whole-process median MB/s. The saved
baseline binary was measured again beside the candidate and Ghostty; do not
calculate the improvement from the earlier table measured at another time.

| Workload | eterm before | eterm after | Change | Ghostty |
| --- | ---: | ---: | ---: | ---: |
| ASCII | 300.0 | 317.4 | +5.8% | 455.4 |
| Unicode | 367.8 | 375.5 | +2.1% | 649.7 |
| Unique Unicode | 122.2 | 121.6 | -0.5% | 84.6 |
| CSI | 199.6 | 206.9 | +3.6% | 180.9 |
| Images | 2,507.7 | 2,541.1 | +1.3% | 2,511.3 |
| Long escape codes | 3,020.9 | 3,050.1 | +1.0% | 4,339.3 |

Across three timing rounds ASCII improved 5.5-6.5%, Unicode 2.1-2.7%, and CSI
3.4-3.9%. Unique Unicode measured 0.5-0.8% lower; its final timing distributions
overlap, so this remains a small possible tradeoff rather than evidence of no
regression. Images and long escape codes stayed near baseline. Linux/GCC and
Windows were not measured.

Correctness: all 178 terminal tests passed before and after. The candidate matched
the baseline on 100 random parser streams (read sizes 1, 7, 4,093 and 65,536), plus
12 ASCII/Unicode/Unique Unicode/CSI checkpoints before final cleanup; comparisons
include cursor, visible cells, cluster codepoints, modes and colors. Four normal-
screen history probes (capacities 0, 1, 1,000, 10,000) matched final cells/cursor,
consumed bytes, DSR replies and retained-row counts. Image update counts, pixel
bytes and sampled hashes also matched. Every benchmark consumed the full input
and passed the six-DSR-response gate.

The timing captures predominantly scroll the alternate screen, so the separate
normal-screen probes are necessary; their history-limit sensitivity alone does
not characterize history performance. Avoiding the clear entirely was not
implemented: recycled history buffers can be uninitialized, and selection and
scroll callbacks must not observe a partly initialized row. That remains a
separate, more involved candidate.

Artifacts: `/tmp/negen-vt-clear/` contains `measure-final.py`, `final-summary.json`,
`*-final.json`/logs, both profiles and disassembly, `compare.py`, the optimized
binary, build/test logs and `extra-checks.log`. The saved baseline and pinned
Ghostty build remain in `/tmp/negen-vt-controlled/`.

### Runtime-dispatched SIMD clears (2026-11-03)

The current implementation supersedes the inline scalar unroll above with file-local
fill helpers. A file-scope `static const` function pointer resolves the CPU once at
startup, matching the existing scanners. ARM64 checks `CPU::hasNEON()`; x86-64
checks `CPU::hasAVX2()` and otherwise uses baseline SSE2. Other architectures use
a scalar fallback retaining the four-glyph unroll. There are no CPU checks or
function-local static guards per clear.

NEON/SSE2 store four 16-byte cells per iteration; AVX2 stores two 32-byte blocks.
All paths handle the exact remaining cells, use unaligned-safe accesses and stay
inside the requested region. ARM64 disassembly contains paired vector stores;
`tclearregion()` calls the cached pointer once per row. The change adds no heap
allocations and preserves the cell layout. The measured change includes outlining
and vector tails as well as the wider bulk stores.

Final serial comparison, same setup and pinned Ghostty build as above:

| Workload | eterm HEAD | Scalar unroll | Runtime SIMD | SIMD vs scalar | Ghostty |
| --- | ---: | ---: | ---: | ---: | ---: |
| ASCII | 299.3 | 318.3 | 350.9 | +10.3% | 446.9 |
| Unicode | 367.0 | 373.2 | 384.8 | +3.1% | 648.6 |
| Unique Unicode | 122.3 | 121.3 | 120.8 | -0.4% | 82.9 |
| CSI | 201.1 | 207.7 | 215.8 | +3.9% | 180.5 |
| Images | 2,521.8 | 2,535.6 | 2,529.4 | -0.2% | 2,492.7 |
| Long escape codes | 3,043.8 | 2,989.0 | 3,022.8 | +1.1% | 4,315.8 |

The additional reversed-order ASCII round measured HEAD 300.4, scalar
317.8, SIMD 349.7, Ghostty 446.9 MB/s:
SIMD improved 10.1% over scalar and 16.4% over HEAD.
The forward round improved 10.3% over scalar and 17.3% over HEAD. Unique Unicode
measured 0.4% below scalar (1.2% below HEAD); images and long escapes remained
near scalar, without the larger apparent losses from the abandoned inline-SIMD
experiment. These are M2 Pro results, not native x86 performance measurements.

Validation: all 178 terminal tests pass. The final runtime-dispatched parser
matches HEAD on 100 random streams and 12 workload checkpoints, plus the four
normal-screen history probes and image checks described above. Full-byte and
six-DSR-response gates passed for every timed eterm run.

An external probe extracted the actual helper implementations from the source
and compared them with `std::fill_n`, including surrounding guards: counts 0..260,
four valid alignments, eight offsets and four patterns. Scalar and NEON passed
66,816 cases natively; scalar and SSE2 passed 66,816 under Rosetta. The full
parser translation unit passes x86-64 syntax checking, and the probe compiles
AVX2 machine code. This host does not advertise AVX2, so its runtime checks were
skipped; native x86 performance, AVX2 execution, Linux/GCC and Windows remain
unverified. No probe scaffolding was added to production code.

Artifacts in `/tmp/negen-vt-clear/`: `measure-simd-final.py`,
`simd-final-summary.json`, `*-simd-final.json`/logs, `eterm-dispatch`,
`compare-dispatch.py`/log, `extra-checks-dispatch.log`, `dispatch-tests.log`,
`fill-probe.cpp`, native/x86 probe binaries/logs and `x86-syntax.log`.
The abandoned inline-SIMD measurements remain in `*-simd.json`; the intermediate
dispatched run is in `*-dispatch.json`. Use `*-simd-final.json` for the final table.

### Portable four-cell printable batches (2026-11-03)

Baseline: committed eterm `620c74bfc0cf76e764d1a61b86c806295dc661cf`, including
runtime-dispatched SIMD clears. The working-tree change batches four narrow glyphs
in `tputrun()` for both ASCII and REP. Four old mode fields are combined into one
wide/dummy test; placeholder checks still run when metadata exists. If any of the
four cells needs repair, the existing per-cell path handles it. Selection checks
remain before the segment, including the existing pending-wrap behavior. Glyph
construction is shared through an inlined lambda capturing only references.

No new allocations, persistent state, ABI changes or ISA-specific code remain.
The original scalar path handles the tail and all exceptional cells. Apple Clang
emits four paired general-register stores per batch, replacing per-cell branches;
it does not need explicit SIMD for the measured improvement.

Fresh ASCII and Unicode profiles support targeting the writer: ASCII is still
primarily `twrite()` (including its inlined writer) and the printable scanner;
Unicode remains primarily in `tpututf8()` (decoder and writer). The baseline
profiles and disassembly are saved with the artifacts below.

Final whole-process median MB/s on M2 Pro, one warmup and five runs, 120x80,
mode 2027 enabled after every RIS, 10,000 requested history rows in both engines.
Ghostty is the pinned `91973f2b2` build from the controlled comparison above. All
builds, profiles, tests and timings ran serially.

| Workload | eterm before | Portable batch | Change | Ghostty |
| --- | ---: | ---: | ---: | ---: |
| ASCII | 347.9 | 401.6 | +15.4% | 448.6 |
| Unicode | 378.8 | 385.2 | +1.7% | 649.7 |
| Unique Unicode | 119.6 | 122.1 | +2.1% | 83.6 |
| CSI | 215.3 | 254.4 | +18.1% | 181.2 |
| Images | 2,511.1 | 2,526.0 | +0.6% | 2,489.1 |
| Long escape codes | 3,052.2 | 2,965.5 | -2.8% | 4,342.6 |

Reversing ASCII command order measured 350.7 before, 406.0 after,
and Ghostty 452.9 MB/s (+15.8% over eterm before). The earlier
portable-only round improved ASCII 17.0% and CSI 18.4%. The final forward/reverse
ASCII rounds improve 15.4-15.8%, reaching about 90% of Ghostty throughput.

The final short long-escape case measured 2.8% lower, with a higher after-run
standard deviation. Three additional alternating-order comparisons measured
+0.3%, +1.8% and -2.7%; they do not show a consistent regression. Keep the final
full-matrix number visible rather than substituting a favorable recheck.

An explicit NEON writer was also tried using the existing startup function-pointer
dispatch pattern. It widened exactly four input bytes and stored interleaved
codepoint/mode/fg/bg vectors. It passed the extended terminal tests, but ASCII
was flat (403.4 -> 403.9 MB/s), Unique Unicode measured -3.5%, and long escapes
-3.1%. The trial was removed; its implementation and measurements remain in
`terminalemulator-neon-trial.cpp` and `*-simd.json` under the artifact directory.
This rejects that concrete implementation, not every possible SIMD writer.

Correctness: all 178 terminal tests pass. Existing tests now exercise four-
character writes over kitty placeholders and selected/unselected text, including
a pending wrap. The wide-overwrite test also checks a wide glyph whose second
half lies beyond the batch, and a write starting on a dummy whose first half
lies before the batch. Test inputs use string views and allocate nothing.

The portable candidate matches the committed baseline on 100 random streams
(read sizes 1, 7, 4,093 and 65,536) and 12 workload checkpoints before cleanup:
cursor, cells, cluster codepoints, modes and colors. The final rebuilt writer
has identical assembly to that validated candidate. Four normal-screen history
probes (capacities 0, 1, 1,000, 10,000) match final cells/cursor, consumed bytes,
DSR replies and retained-row counts; image update counts, pixel bytes and sampled
hashes also match. Every timed eterm run consumes the full input and passes the
six-DSR-response gate. The parser translation unit passes x86-64 Clang syntax
checking; native x86 performance, Linux/GCC and Windows were not run.

Artifacts: `/tmp/negen-vt-writer/` contains `eterm-before`, `eterm-after`,
`measure-final.py`, `final-summary.json`, `*-final.json`/logs,
`long-recheck-summary.json`, `compare-batch.py`/log, `extra-checks.log`,
`final-tests.log`, `x86-syntax.log`, baseline profiles and before/after writer
disassembly. `*-batch.json` is the initial portable comparison and `*-simd.json`
is the rejected NEON trial. `metadata.json` records revisions and capture hashes.

### Equal-width Unicode batches and bounded SIMD decoding (2026-11-03)

Baseline: `a996bb7371898124e1f34fc37fe7cc57a105145f`, which committed the
four-cell narrow writer above. The current local change extends that writer to
four equal-width Unicode glyphs (one or two cells each), and uses bounded
simdutf conversion behind an exported, nonallocating `String::tryToUtf32()` API.
`TerminalGlyph` remains 16 bytes; no style table or cell representation change.

A diagnostic profile with the decoder and writer outlined puts regular Unicode
roughly evenly between `decodeUnicodeRun<true>` and `tputrun<UnicodeRun>` (1,171
and 1,169 leaf samples, respectively; NEON clears 141). These are sampling
counts from a diagnostic binary, not precise percentages for the final binary.
Unique Unicode remains dominated by combining-mark attachment, cluster
interning and allocation/GC. Batching narrow Unicode alone gained about 2%;
including four wide glyphs gained about 21% before SIMD decoding. The wide
batch checks all eight old cells for wide/dummy/placeholder repairs before
writing, preserves the scalar selection rules and retains dummy-cell colors.
Apple Clang unrolls the four dummy writes into four paired stores; no loop
branch remains. Forcing `tpututf8()` inline was tried and removed: regular
Unicode measured about 2.3% slower without a useful CSI gain.

The SIMD helper is deliberately out of line. After four accepted scalar
codepoints, it considers at most the remaining output capacity in input bytes,
bounded before LF/ESC and trimmed to a UTF-8 boundary. It requires at least
64 bytes. The converted codepoints still pass the same control, width and
grapheme-property checks; a stopped prefix is mapped back to its byte end.
Malformed conversion discards partial output and keeps the scalar recovery
path. No controls or cluster boundaries are consumed early. Mark-leading and
short runs keep the scalar path without calling the helper.

`String::tryToUtf32()` validates while converting into caller-provided storage,
without a terminator or BOM removal. eterm bounds input bytes by output slots,
so its call needs only that validating conversion. Tighter general-purpose
buffers first validate/count to establish capacity. Keeping simdutf in core
avoids depending on private third-party symbols across Windows DLL boundaries.
The library's existing runtime SIMD dispatch selects the implementation.
`UnicodeRun` uses `char32_t` storage rather than aliasing a `Rune` buffer; it is
still 2,560 bytes. The helper returns an 8-byte end/count result by value.
Native ARM64 and x86-64 Clang checks verify those sizes and the 16-byte glyph.

Final whole-process medians on M2 Pro: one warmup, five runs, 120x80, mode 2027
enabled after every RIS, 10,000 requested history rows in both engines, same
captures and pinned Ghostty `91973f2b2` as above. MB = 1,000,000 bytes. All heavy
work ran serially. Every timed eterm run passed the full-byte and six-DSR gates.

| Workload | Committed eterm | Local eterm | Change | Ghostty | eterm / Ghostty |
| --- | ---: | ---: | ---: | ---: | ---: |
| ASCII | 398.3 | 392.3 | -1.5% | 438.0 | 0.90x |
| Unicode | 375.0 | 480.1 | +28.0% | 628.0 | 0.76x |
| Unique Unicode | 113.7 | 114.8 | +0.9% | 80.6 | 1.42x |
| CSI | 247.8 | 246.1 | -0.6% | 176.5 | 1.39x |
| Images | 2,181.1 | 2,225.3 | +2.0% | 2,170.1 | 1.03x |
| Long escape codes | 2,877.6 | 2,871.6 | -0.2% | 4,238.4 | 0.68x |

Reversed Unicode order measured 371.5 -> 481.4 MB/s (+29.6%), Ghostty 612.7
(0.79x). Reversed Unique Unicode measured 113.5 -> 112.2 MB/s (-1.1%),
Ghostty 79.9 (1.41x). Thus the final pair of rounds shows a 28-30% regular
Unicode gain and about +/-1% on Unique Unicode. Absolute rates vary with the
host; the earlier direct-call prototype reached 504.4 MB/s, but use the matched
final table rather than mixing its rate with another Ghostty run.

The first SIMD prototype added about 5% over the wide writer but cost about
3.6% on Unique Unicode; the user explicitly accepted that tradeoff for the
regular-text gain. Outlining the helper, returning count by value and hoisting
the BMP lookup in zero-width runs reduced that cost in subsequent rounds.
The initial exported-API matrix had a noisy Unique result of -8.3% overall;
three alternating direct-call/API checks then put the API build +0.5%, +6.6%
and +3.0% ahead of the prototype. These do not establish a wrapper penalty.
All those runs remain in the artifacts; the final matrix and reverse checks
above are a separate complete measurement, not substituted favorable rows.

Correctness also uncovered an existing display-update bug: setting ATTR_WRAP
or ATTR_WRAP_PADDING when output arrived in separate reads did not mark the
previous row dirty. The same eight test assertions failed on the committed
baseline. Both `tputc()` wrap cases and `tputrun()` now dirty that row before
moving on; the fragmented/bulk write test passes without forcing redraw.

Validation:

- All 180 terminal and 20 String tests pass; release eterm, ecode and unit tests
  build with the project's no-framework script.
- New terminal tests cover equal-width writes, old dummy colors, selections,
  padding/wrap snapshots and mixed UTF-8 across the SIMD limit, with mode 2027
  on and off. Controls, marks, ZWJ, C1, malformed/incomplete UTF-8, truecolor
  and hyperlink metadata survive fragmented reads.
- The String test covers tight/insufficient capacity with guards, invalid UTF-8,
  embedded NUL, BOM preservation, empty input and a 256-byte ASCII buffer.
- 100 random streams (read sizes 1, 7, 4,093 and 65,536) and 12 workload
  checkpoints match the committed parser's canonical full-model cells,
  resolved cluster text, modes, colors and cursor. The dump harness forces a
  redraw to distinguish the old snapshot-dirty bug from parser state.
- History capacities 0, 1, 1,000 and 10,000 match cells/cursor, bytes, DSR
  responses and retained rows. Image update counts, pixel bytes and hash match.
- Allocation audit: no new heap allocations or retained state in the parser or
  conversion API. Existing lazy BMP storage is reused; the run stack size is
  unchanged. Local lambdas capture references or the existing boolean flag,
  with no ownership handoffs or dynamic function wrappers.
- Native ARM64 builds/tests and x86-64 Clang syntax/layout checks pass.
  Native x86 performance, Linux/GCC and Windows remain unverified.

Artifacts: `/tmp/negen-vt-unicode/` contains `eterm-before`, `eterm-after`,
`eterm-value` (direct-vendor prototype), source snapshots, `measure-accepted.py`,
`accepted-summary.json`, all `*-accepted.json`/logs, preliminary trial matrices,
`final-summary.json`, `wrapper-recheck-summary.json`, diagnostic samples and
assembly. `compare-final.py`/log, `extra-checks.log`, test/build logs and
`layout-check.cpp` preserve validation. `metadata.json` records source revisions,
capture/binary hashes and the final source snapshot; `report.md` has this record.

### Mixed-width batching retry experiment (2026-11-03, rejected)

Baseline `ad936549912b5e12bf764b9e4df93b9a6cb53e1d`. A small writer-only
prototype retried four-glyph batches after scalar Unicode writes, rather than
leaving the rest of the row scalar after the first width mismatch. Every row
clear, scroll and snapshot boundary stayed unchanged. No delayed clearing was
implemented; the user requested preserving that behavior.

All 180 terminal tests passed, and 100 random streams plus 12 workload
checkpoints matched the committed parser (cells, resolved clusters, modes,
colors and cursor). The existing fragmented-input test now also includes a
narrow glyph followed by eight wide glyphs and eight narrow glyphs in one row.
That coverage remains after removing the prototype.

Performance did not justify keeping the change. The first two-engine matrix
measured Unicode +4.0% forward and +2.3% reverse, Unique Unicode -3.9%, ASCII
+0.05% forward and -1.4% reverse. The final matched three-engine matrix below
measured Unicode +0.4% forward and +3.7% reverse, with repeatable losses on
Unique Unicode and a lower ASCII result. These are whole-process median MB/s,
one warmup/five runs, serial native M2 Pro, 120x80, mode 2027 on after RIS,
10,000 requested history rows, and pinned Ghostty `91973f2b2` as above.

| Workload | Committed eterm | Rejected trial | Change | Ghostty |
| --- | ---: | ---: | ---: | ---: |
| ascii | 406.3 | 396.7 | -2.4% | 444.6 |
| unicode | 495.9 | 498.0 | +0.4% | 646.6 |
| unique_unicode | 120.0 | 114.7 | -4.4% | 84.7 |
| csi | 252.4 | 252.0 | -0.2% | 180.8 |
| images | 2,492.3 | 2,486.6 | -0.2% | 2,460.9 |
| long_escape_codes | 3,027.3 | 3,062.1 | +1.2% | 4,296.7 |
| unicode-reverse | 490.2 | 508.4 | +3.7% | 649.6 |
| unique_unicode-reverse | 118.6 | 115.4 | -2.6% | 83.2 |

The production writer was restored to the committed implementation. The trial
introduced no allocations or object-layout changes, but its additional loop
checks and changed code placement did not produce a strong enough net result.
Do not describe it as a retained optimization or mix its rates into the accepted
table above.

Artifacts: `/tmp/negen-vt-mixed/` contains baseline/trial binaries and sources,
`measure.py`/`summary.json` (initial pairwise matrix),
`measure-final.py`/`accepted-summary.json` (final, despite the filename the
trial was rejected), raw JSON/logs, `compare.py`/log and `tests.log`.
`restored-build.log` and `restored-tests.log` validate the restored writer with
the expanded mixed-width test. All timed runs passed byte and DSR gates.

### SIMD span/threshold tuning (2026-11-03)

The parser baseline is committed `ad9365499`; measurements started from the
same parser at `a62d81f9f`. The user was using the OS lightly, so all builds and
measurements ran serially, each candidate was compared with the baseline in the
same hyperfine invocation, and promising candidates were rechecked together
with reversed command order. Small deltas and absolute MB/s remain subject to
host activity; the larger reverse-order gain is not a promised steady-state gain.

Five configurations were screened against the current 512-codepoint buffer,
four-codepoint scalar seed and 64-byte minimum. Initial matched percentage changes:

| Trial | Unicode | Unique Unicode | ASCII | CSI |
| --- | ---: | ---: | ---: | ---: |
| seed2 | +3.4% | -1.7% | +2.1% | +0.8% |
| min128 | -0.9% | -1.3% | -0.7% | +0.4% |
| min32 | +3.6% | +1.5% | +0.9% | -2.0% |
| span1024 | +1.5% | -10.3% | -3.8% | -1.7% |
| span2048 | +6.4% | +1.3% | +1.1% | -4.7% |

`seed2` lowers the scalar prefix from four to two codepoints. `min128`/`min32`
change both minimum-byte checks. `span1024`/`span2048` increase the existing
codepoint capacity, which also bounds the input byte span. All other behavior
stays unchanged. Threshold-only candidates offered smaller or inconsistent
benefits; the 1,024-slot trial lost substantially on Unique Unicode.

A second comparison used seven runs, included baseline/seed2/min32/span2048 in
one invocation, and repeated Unicode in reverse order. The 2,048-slot candidate
improved Unicode 7.1% forward and 4.7% reverse; seed2 improved 3.6%/2.1% and
min32 1.2%/0.4%. For span2048, Unique Unicode measured -1.5%, ASCII -1.3% and
CSI +0.7%. The larger span was selected; thresholds remain at their committed
values. No row clears, scrolling behavior, writer grouping or snapshot timing
changed.

Final whole-process median MB/s, one warmup/five runs, native M2 Pro, 120x80,
mode 2027 enabled after every RIS, 10,000 requested history rows in both engines,
same captures and pinned Ghostty `91973f2b2` as above:

| Workload | Committed eterm | Local eterm | Change | Ghostty | eterm / Ghostty |
| --- | ---: | ---: | ---: | ---: | ---: |
| ASCII | 398.4 | 401.9 | +0.9% | 435.0 | 0.92x |
| Unicode | 495.6 | 514.0 | +3.7% | 634.4 | 0.81x |
| Unique Unicode | 120.8 | 115.4 | -4.5% | 82.4 | 1.40x |
| CSI | 247.0 | 249.9 | +1.2% | 177.9 | 1.40x |
| Images | 2,492.5 | 2,535.3 | +1.7% | 2,495.6 | 1.02x |
| Long escape codes | 3,042.9 | 3,072.6 | +1.0% | 4,375.6 | 0.70x |

Reverse-order Unicode: 470.0 -> 521.4 MB/s (+10.9%), Ghostty 646.0 (0.81x).
Reverse-order Unique Unicode: 119.0 -> 117.0 MB/s (-1.6%), Ghostty 83.3 (1.40x).
Unicode improved in every matched pass for this candidate (+3.7% to +10.9%),
with +4.7%/+7.1% in the multi-candidate recheck. Unique Unicode varied from
+1.3% in screening to -1.5%, -4.5% and -1.6% in subsequent checks. Keep that
tradeoff visible: the user accepted combining-mark throughput losses for
regular Unicode gains, but these are separate workloads.

The retained production change is `UnicodeRun::Capacity = 2048`. It amortizes
conversion and run setup with a 10,240-byte local buffer instead of 2,560 bytes.
ARM64 and x86-64 Clang syntax/layout checks confirm those sizes, the unchanged
8-byte helper result and 16-byte cell. ARM64 disassembly shows `tpututf8()`'s
stack frame growing from 2,944 to 10,624 bytes including saved registers, and
introduces a Darwin stack-probe call. This is bounded per invocation, with no
recursion added. It is a real stack/call cost, not a free buffer increase.
Allocation audit: no new heap allocations, persistent state, copies, captures,
async handoffs or public ABI changes. Existing buffer storage alone grows.

Validation: release eterm, ecode and tests build; all 180 terminal tests pass.
The fragmented UTF-8 test now crosses the 2,048-byte limit using mixed 2/3/4-byte
codepoints (250 repetitions). Both grapheme modes still cover controls, marks,
ZWJ, C1, malformed/incomplete input, links and colors. 100 random streams
(chunks 1, 7, 4,093, 65,536) and 12 workload checkpoints match baseline cells,
resolved cluster text, modes, colors and cursor. History capacities 0, 1,
1,000 and 10,000, and image update/pixel/hash checks match. All timed runs
pass byte-consumption and six-DSR gates. Windows/Linux/GCC execution and native
x86 performance remain unverified.

Artifacts: `/tmp/negen-vt-tuning/` holds `screen.py`/`screen-summary.json`,
`recheck.py`/`recheck-summary.json`, `measure-final.py`/`accepted-summary.json`,
all raw JSON/logs, baseline and five trial binaries/source snapshots, the final
`eterm-after` and `terminalemulator-after.cpp`, `compare.py`/log,
`extra-checks.log`, `final-tests.log`, `final-build.log`, layout checks and
`tpututf8-before.asm`/`tpututf8-after.asm`. `metadata.json` records hashes;
`report.md` contains this record. No benchmark scaffolding is in production.

### Real-PTY repaint latency after producer gaps (2026-11-03)

A user reproduction exposed temporary blank frames with the cursor at (0,0)
while a large Codex session repaints after showing/hiding the tab bar. The user
also observed the intermediate frame in Ghostty, but it recovers much faster.
Bulk parsing benchmarks do not measure producer/consumer wakeup latency.

The worker polls the PTY nonblockingly, then waits on its command condition
variable for 8 ms when drained. PTY output does not notify that condition. A
single yield/reprobe already exists in `TerminalEmulator::update()`, but when
it misses the producer's next write, every new gap can pay the idle interval
and backpressure the child.

An external harness replays the recorded 302 KiB Codex stream through a real
`ProcessFactory` PTY and `TerminalSession`, at 177x52 with 10,000 history rows.
The child writes an OSC title marker after the data; the harness waits for a
snapshot containing it. Producer duration is logged separately. This is a
paced replay benchmark, not a measurement of live Codex's complete relayout.
The requested 0.2 ms sleeps can oversleep under macOS scheduling; compare
interleaved runs rather than assuming the requested sleep is the elapsed time.

A permanent 1 ms wait trial showed a large gain but would increase idle-tab
wakeups. The initial adaptive candidate used 1 ms for 50 ms after any input
was read, then returned to the original 8 ms. The spinner review below shortens
that activity window to 15 ms. Command enqueue notifications still
interrupt either wait. There is no busy loop and presentation/synchronization
semantics are unchanged. It adds one local steady-clock time point; no heap
allocation, capture, async ownership handoff, class-layout or public ABI change.

Initial 50 ms-window native M2 Pro medians of three alternating before/after runs:

| Producer write size | Producer gap | Before | Adaptive | Speedup |
| --- | --- | ---: | ---: | ---: |
| 65,536 bytes | none | 41.3 ms | 32.4 ms | 1.27x |
| 1,024 bytes | 0.2 ms requested | 1388.7 ms | 212.7 ms | 6.53x |
| 128 bytes | 0.2 ms requested | 2443.0 ms | 641.8 ms | 3.81x |

The producer itself stalls longer with the old wait, establishing transport
backpressure rather than just delayed snapshot observation. A quiet-session
read counter measured 107 polls/second after a 200 ms idle settling period,
confirming fallback to the 8 ms idle cadence rather than permanent 1 ms polling.
The initial trials sometimes reported a TTY reply failure during child teardown;
the final producer keeps its PTY open briefly after the completion marker and
all final runs are free of those diagnostics.

Release eterm, ecode and unit tests build; all 181 terminal tests pass, including
session wakeup, command latency, synchronization, concurrent shutdown and the
separate height-only resize regression. The user subsequently confirmed that
live Codex relayout appeared fixed. This remains separate from the paced replay
measurement; it does not establish that every possible flicker is eliminated.

Artifacts in `/tmp/negen-pty-latency/`: `latency.cpp`, `producer.py`, the local
capture, `before`, rejected `idle1ms`, retained `adaptive`, trial and final
comparison JSON, `idle.cpp`/`idle-probe`, build/test logs and source snapshots.
`metadata.json` records hashes and the benchmark setup. No harness scaffolding
is added to production.

#### Spinner-cost review and 15 ms active window

Review identified that a spinner updating every 100 ms repeatedly restarts the
active window. A fully quiet session is therefore not sufficient to assess
polling overhead. An external `SpinnerPty` injects one synchronized short repaint
every 100 ms for four seconds and counts calls to `read()`. This measures PTY
polls, not kernel wakeups or CPU energy, and simulates the periodic update
pattern rather than running Claude Code itself.

| Session pattern | 50 ms active window | 15 ms active window |
| --- | ---: | ---: |
| Fully quiet, after settling | 106.5 polls/s | 108.6 polls/s |
| Spinner output every 100 ms | 474.2 polls/s | 233.2 polls/s |

The shorter window reduces spinner polling by 50.8% overall; compared with the
quiet baseline, the excess polls fall about 66%. Repeated producer gaps within
a repaint still refresh the activity time. The retained implementation now uses
15 ms, preserving the original 8 ms wait after activity subsides.

A new matched comparison, medians of three alternating runs over the same
captured stream, confirms that repaint speed is retained. The 1,024- and
128-byte cases request 0.2 ms producer gaps; 65,536-byte writes are continuous.

| Producer write size | Original 8 ms wait | 50 ms window | 15 ms window | 15 ms / original speedup |
| --- | ---: | ---: | ---: | ---: |
| 65,536 bytes | 43.5 ms | 31.5 ms | 32.8 ms | 1.33x |
| 1,024 bytes | 1374.5 ms | 213.2 ms | 213.8 ms | 6.43x |
| 128 bytes | 2456.3 ms | 642.0 ms | 642.3 ms | 3.82x |

All 181 terminal tests pass with the 15 ms window; release eterm and ecode build.
The user reported that live Codex relayout appeared fixed with the earlier
adaptive wait; the later shorter-window comparison above is a paced replay.
No additional allocation or layout changes were introduced by tuning the window.

Follow-up: replace timer polling with a wait for PTY readability plus command
notification. Unix needs a PTY/command-wakeup descriptor wait (poll/kqueue or the
existing platform event mechanism); Windows needs the corresponding ConPTY
read-completion and command wakeup handling. That can remove both the output
latency and repeated idle polling. Windows execution and timer granularity for
SDL2/SDL3 remain unverified; this review does not establish their effective wait
resolution.

The complete local experiment is also preserved outside `/tmp` at
`/Users/prognoz/programming/vtbench/pty-latency/`, including the capture,
harness/producer/spinner sources, measured binaries, core dylib, raw measurement
JSON/logs and source snapshots. `REPLAY.txt` explains how to restore the original
temporary paths and rebuild the harnesses. The private conversation capture is
kept in that local benchmark directory, not added to the eepp repository.

### Readiness-based PTY waits (2026-11-03)

Implements the follow-up above on POSIX and Windows. The worker no longer sleeps on a timer while
idle; it waits until output arrives, pending input can be written, a command is queued, or the
next timer that `update()` services is due (`TerminalEmulator::getMaximumIdleWait()`: deferred
PTY resize, synchronized-update timeout, image animations, deferred link/cluster collection,
capped at 250 ms because child exit is polled).

- POSIX: `poll()` on the PTY master and a nonblocking self-pipe. `enqueue()` and `shutdown()`
  write one byte while no wakeup is pending; the worker drains the pipe and clears the flag
  before taking the queue, so no command wakeup is lost and a full pipe already implies one.
  `POLLOUT` is requested while `PseudoTerminal` buffers input. After `POLLHUP` the PTY is dropped
  from the wait set at the next wait, which always follows an `update()` that drained it, so
  buffered output survives and a kept-open exited terminal does not spin (Linux may report the
  hung-up master readable). Restart clears the state; the PTY handle is re-read every iteration.
- Windows: anonymous ConPTY pipes cannot be read asynchronously, so `PseudoTerminal` runs a reader
  thread blocked in `ReadFile()`, buffering at most 4 MiB (beyond that the pipe holds the child
  back, as before) and signaling a manual-reset event under the buffer lock. The session waits on
  an auto-reset command event, that event and the child's process handle (while running) with
  `WaitForMultipleObjects()`. Destruction stops buffering, closes the console while the reader
  keeps draining, and cancels the blocked read if the pipe stays open.
- Mock PTYs (and a ConPTY reader that fails to start) keep the 15 ms adaptive timer path.
- Review fixes: the ConPTY buffer is a `PtyOutputQueue` that reclaims consumed bytes once they
  are at least as many as the unread ones (it previously compacted only when emptied, so a queue
  that never drained grew without bound); and an animation loading (`s=2`) on its last frame no
  longer yields a zero deadline, which made the worker poll with a zero timeout.

Real-PTY harness (`~/programming/vtbench/pty-wait/wake.cpp`): a pass-through PTY counts reads
(one per worker iteration); a child is quiet, then prints a synchronized spinner frame every
100 ms; an idle raw `cat` echoes single bytes. Baseline is HEAD `8e4fbd1e7` (15 ms adaptive
wait), built from the same harness. M2 Pro:

| Measure | 15 ms adaptive | Readiness |
| --- | ---: | ---: |
| Quiet tab, worker reads/s | 106 | 4 |
| 100 ms spinner, reads/s | 220 | 27-28 |
| Idle command to echoed output, median | 9.2 ms | 0.20 ms |
| Exited child, kept-open tab, iterations/s | n/a | 4 |

Paced Codex replay (same capture and producer as above), medians of three alternating runs:

| Producer writes | 15 ms adaptive | Readiness | Speedup |
| --- | ---: | ---: | ---: |
| 64 KiB, continuous | 39.7 ms | 30.2 ms | 1.31x |
| 1 KiB, 0.2 ms gaps | 221.8 ms | 103.2 ms | 2.15x |
| 128 B, 0.2 ms gaps | 648.8 ms | 645.4 ms | producer-bound |

Native Linux x86_64 (openSUSE, kernel 7.2): quiet 4 reads/s, spinner 30/s, echo 0.04 ms,
exited 4 iterations/s. Haiku R1/beta6+ (hrev60147) runs the same POSIX path: its TTY driver
selects readable and disconnected on master and slave (src/add-ons/kernel/generic/tty/tty.cpp),
and poll() maps disconnected to POLLHUP (src/system/kernel/events/wait_for_objects.cpp).
Measured there: quiet 4 reads/s, spinner 30/s, echo 0.10 ms, exited 4 iterations/s. Windows (MinGW x86_64 under Wine 11.19): the real-ConPTY tests pass,
including 3,000 lines followed by exit and command-to-output under 150 ms after a warm-up
command; Wine's `cmd` takes ~0.65 s for its first command. Native Windows timing is unmeasured.

Tests: `idle_wait_is_bounded_by_update_timers`, `readiness_wait_wakes_for_commands_and_shutdown`
(fails without the wakeup signal) and `readiness_wait_keeps_output_buffered_at_hangup`, the
latter two on POSIX and ConPTY. macOS, Linux and Wine pass all `eterm.*` and `eterm_session.*`
tests; Wine cannot run the UI tests without a display.

### Workload matrix against Ghostty main (2026-11-03)

Ghostty `c7f68a7f4` (main, 1.4.1-dev) with the benchmark controls patch, `ReleaseFast`, against
eterm `98d0f6817` and the earlier pinned `91973f2b2`, in the same hyperfine invocation: M2 Pro,
120x80, 10,000 history rows, mode 2027 on, one warmup, five runs. New streams are 64 MB from
Ghostty's `ghostty-gen` (seed 42), plus the private Codex capture repeated with a reset between
repeats. Scripts and streams: `~/programming/vtbench/matrix/` (`gen.sh`, `measure.py`, `ab.py`).

| Workload | eterm | Ghostty main | pinned | eterm / main | main / pinned |
| --- | ---: | ---: | ---: | ---: | ---: |
| ASCII | 403.0 | 482.0 | 439.7 | 0.84x | 1.10x |
| Unicode | 519.3 | 624.9 | 616.2 | 0.83x | 1.01x |
| Unique Unicode | 114.0 | 82.0 | 83.6 | 1.39x | 0.98x |
| CSI | 250.9 | 197.0 | 180.9 | 1.27x | 1.09x |
| Images | 2,533.0 | 2,227.4 | 2,214.9 | 1.14x | 1.01x |
| Long escape codes | 3,076.0 | 3,825.0 | 3,849.9 | 0.80x | 0.99x |
| Styled | 274.8 | 276.5 | 256.7 | 0.99x | 1.08x |
| Styled Unicode | 144.7 | 145.3 | 144.8 | 1.00x | 1.00x |
| Styled with links | 139.8 | 122.8 | 123.2 | 1.14x | 1.00x |
| Dense links | 169.3 | 63.7 | 61.4 | 2.66x | 1.04x |
| Erase (CSI 2J, line clears) | 51.4 | 194.8 | 132.7 | 0.26x | 1.47x |
| Random wide UTF-8 | 37.8 | 264.3 | 264.1 | 0.14x | 1.00x |
| Codex capture | 269.7 | 265.1 | 240.4 | 1.02x | 1.10x |

`erase_scroll` is excluded: it uses `CSI 22 J`, which eterm ignores, so eterm did less work.
The real Codex stream is level; the plain-text gaps remain only in the kitten streams. Findings
from profiling these:

- Erase: 77% in `fillGlyphsNEON` refilling cells that are already blank. This is the case for
  blank-row tracking; the Codex stream also spends 14% there.
- Random UTF-8 spent 61% re-converting: each run converted up to 2,048 codepoints, kept the prefix
  before the first stop and converted the rest again. The conversion span now halves when a stop
  wasted most of it and doubles when fully used: 37.8 -> 88.2 MB/s (2.38x), other streams within
  1%. See "Random wide UTF-8 breakdown" for what remained: unassigned and private-use codepoints
  turned out not to be the cause.
- Real repaint (paced Codex replay in a hidden window): about 16% of a core, mostly on the worker,
  not rendering. 47% of busy samples were `memmove`, largely from copying the grid to publish each
  snapshot, and 16% were `selected()` per cell without a selection, now skipped (busy samples
  532 -> 447). vtbench cannot see either cost: it has no session or snapshots.
- That replay found a hang: `fcntl( master, ... )` converted `AutoHandle` through `operator bool`
  to fd 1, so the master was never `O_NONBLOCK` or `FD_CLOEXEC`, and stdout was changed instead. A
  child that does not read its input could block the worker in `write()` forever.

Snapshot reuse, bounded pending PTY writes and blank-row tracking are implemented below.

### Snapshot cell reuse (2026-11-04)

Publishing copied the whole grid every time, about half of the worker's busy samples in the paced
Codex replay. A snapshot's cells now go back to a pool when nobody holds it, and the next publish
copies only the rows changed since those cells were written. Rows record the first generation that
shows them as they are now. A resize marks every row, and a search highlight marks its row, since
highlights exist only in the snapshot's cells. Held snapshots stay immutable.

Paced replay in a hidden window, 5 s `sample`, three runs before and six after, M2 Pro. This is CPU
evidence only, not a GPU measurement:

| | Before | After |
|---|---|---|
| Busy samples | 567-613 (mean 596) | 365-494 (mean 430, -28%) |
| `memmove` samples | 280-300 (mean 287) | 115-151 (mean 127, -56%) |

Stale-row bugs are covered by tests that fail with an off-by-one row generation. The highlight mark
is defensive: every search change currently redraws all rows, so no test can force it. The same
applies to marking rows on resize.

### Bounded pending PTY input (2026-11-04)

User input and terminal-generated replies share one FIFO, so they stay in order, but only replies
are bounded. User input, including a large paste, always queues: it is already in memory and a
paste must not be lost. Replies have a 4 MiB POSIX budget, counted per byte of queued reply and not
reduced by queued input, so a paste delays a reply instead of dropping it.

`TerminalEmulator::ttyreply()` marks replies explicitly, independently of `mayEcho` (the OSC color
reply enables echo). Its `beginReply()` decides before any byte is written: a reply that does not
fit is dropped whole, so the child never sees a truncated sequence. Overflow never terminates the
child, and replies resume as queued ones are delivered. Mouse, key and focus reports count as input.
ConPTY's write path is unchanged and accepts every reply.

Review also routed Kitty graphics acknowledgements through this budget. They previously called
raw `write()` directly, bypassing it. They retain their raw semantics (no local echo, newline
translation or viewport scroll); the whole-reply test covers refusing and accepting them.

An earlier version of this patch terminated the child when pending input passed 4 MiB. It rejected
writes by size alone, so a 5 MiB paste into a healthy `cat` killed it within 3 ms.

Pending input uses the existing `PtyOutputQueue` with direct access and consumption of its unread
span. Partial writes advance its offset; compaction happens only after at least half the stored
bytes have been consumed, replacing the previous allocating `substr()` after every partial write.

Regression coverage uses a real raw-mode PTY for partial-drain ordering and the budget's boundaries
and release, a mock PTY for dropping whole replies while input passes, and sessions for an 8 MiB
paste delivered intact and for a reply flood past the budget that leaves the child responsive. The
existing sustained queue test also exercises compaction. Parser-only benchmarks do not exercise
pending writes. Pasting into a reading child, one run each, before and after the offset change:
3 MiB 0.44 -> 0.17 s, 8 MiB 2.78 -> 0.49 s, 32 MiB from a timeout at 20 s to 1.80 s. The old
`substr()` copied the remainder after every partial write.

Remote verification found a Haiku TTY-driver bug on hrev60147: a nonblocking master write can
deliver earlier 256-byte blocks and then return EAGAIN for a later block without reporting those
earlier bytes. Retrying duplicates input. A standalone 1 MiB probe reproduced this outside eterm;
limiting each native Haiku write to one 256-byte block delivered the stream exactly. The same
bound now applies to initial and queued PTY writes on Haiku only. Ordinary pipes delivered the
same probe exactly, and core `EE::System::Process` uses pipes, so that path needs no such bound.

Final targeted validation: macOS passes all 196 terminal tests; Linux x86_64/GCC/SDL3 passes
142 `eterm.*` and 32 `eterm_session.*`; MinGW x86_64/Wine 11.19 passes 138 and 29; Haiku passes
142 and 32, including the 8 MiB paste (0.74-0.84 s) and flood recovery (1.30-1.34 s). Haiku's
session suite passed under a PTY; two redirected-output runs aborted with
`std::system_error: No such process`, whose cause remains unconfirmed. Wine uses the project's
SDL2.dll: the system MinGW SDL2.dll failed during initialization. Native Windows/MSVC and Wine
UI tests were not run. Remote copies are `/home/programming/eepp-negen` and
`/boot/home/programming/eepp-negen`; user checkouts were left unchanged. Logs and standalone
probes are under `/tmp/negen-reply-budget-*` and `/tmp/negen-haiku-*` on the respective hosts.

### Blank-row tracking (2026-11-04)

The erase workload spent 77% of its time rewriting cells that were already blank, and the Codex
stream 14%. A row is now marked once a whole-row erase leaves it all one blank glyph, and erasing a
marked row in that glyph does nothing, including no redraw.

The mark is a second bit in `mTerm.dirty`. Every write to a row assigns `dirty[y] = 1`, which has
to happen for the row to be drawn, so the same assignment clears the mark and a new writer is safe
by default. Three rules keep that true:

- Only `terase()`, used by ED, EL and ECH, reads or sets marks. `tclearregion()` keeps filling
  unconditionally: `tscrollup()` calls it on recycled history buffers, whose rows are not what a
  mark described.
- Marking resets `mAllDirty`, because `tsetdirt()` returns early under it and would leave marks on
  rows that a scroll moved. So `mAllDirty` implies there are no marks.
- Marks describe one glyph. Partial erases in another color leave other rows marked; the marks are
  only given up to mark a row in a new blank. Resetting on every color change walked all rows per
  `CSI K` and cost Codex 2%.

120x80, 10,000 history rows, M2 Pro, medians of three to five runs (Ghostty main: erase 194.8, Codex 265.1 MB/s
in the matrix above, not re-measured):

| Stream | Before | After |
|---|---|---|
| Erase | 51.7 MB/s | 235-238 MB/s (4.6x) |
| Codex capture | 288-292 MB/s | 304-306 MB/s (+5%) |

Final screens and cursors matched the previous build for the erase, Codex, styled, styled links
and wide UTF-8 streams, and for 120 comparisons of a fuzz stream weighted toward erases mixed with
scroll regions, IL/DL, ICH/DCH, DECALN, alternate screens, BCE colors, wide cells and RIS, at three
chunk sizes. Unit tests fail for seven of eight mutations, including using `terase()` in
`tscrollup()`, keeping `mAllDirty` and `|=` in `tsetdirt()`. The survivor removes the selection
guard, which is redundant: changing a selection dirties its rows, which clears their marks.

### Refreshed workload matrix (2026-11-04)

Fresh matched measurements of the working tree based on `103773956` plus the staged reply-budget
and blank-row erase changes, against the same Ghostty main `c7f68a7f4` and older pinned
`91973f2b2`. Both Ghostty binaries retain their benchmark controls patch. The eterm harness was
linked against the actual release `libeterm.a`, rebuilt through
`projects/macos/make_no_fw.sh config=release eterm-static eepp-shared`.

M2 Pro, 120x80, 10,000 requested history rows, mode 2027 enabled, the same captures and seed-42
generated streams as the previous matrix. Each workload ran all three engines in one hyperfine
invocation with one warmup and five measured runs; engine order alternates between workloads.
These are whole-process wall-clock medians in decimal MB/s, including process setup and teardown.
`erase_scroll` remains excluded because eterm does not implement its `CSI 22 J` operation.

| Workload | eterm | Ghostty main | Ghostty pinned | eterm / main |
| --- | ---: | ---: | ---: | ---: |
| ASCII | 404.4 | 504.8 | 447.7 | 0.80x |
| Unicode | 514.2 | 644.2 | 641.7 | 0.80x |
| Unique Unicode | 115.5 | 82.4 | 83.6 | 1.40x |
| CSI | 255.7 | 195.6 | 179.9 | 1.31x |
| Images | 2,485.5 | 2,344.6 | 2,444.5 | 1.06x |
| Long escape codes | 3,017.3 | 4,289.6 | 4,239.2 | 0.70x |
| Styled | 270.1 | 275.2 | 255.4 | 0.98x |
| Styled Unicode | 137.9 | 141.5 | 143.6 | 0.97x |
| Styled with links | 136.3 | 118.3 | 117.9 | 1.15x |
| Dense links | 156.0 | 63.7 | 61.3 | 2.45x |
| Erase | 226.7 | 193.7 | 130.9 | 1.17x |
| Random wide UTF-8 | 85.1 | 256.6 | 261.2 | 0.33x |
| Codex capture | 283.3 | 254.5 | 232.2 | 1.11x |

Reversing each headline workload's engine order, again one warmup and five runs, confirmed the
ratios: erase 225.7 vs 194.1 MB/s (1.16x), Codex 284.5 vs 258.4 (1.10x), and dense links
158.2 vs 63.5 (2.49x). These confirmation runs are separate from the primary matrix above.

All 13 eterm preflights passed byte-consumption and trailing-DSR checks; Ghostty main's state
report confirmed grapheme mode remained enabled. Executable, library and C++ source hashes were
unchanged across the run. This is parser/file throughput: snapshots, real PTY backpressure and
rendering are not measured here, so this matrix cannot quantify the snapshot or paste improvements.

Artifacts: `~/programming/vtbench/matrix/refresh-20261104-xvcd31oj/` contains the harness source
and executable, `measure.py`, `reverse-check.py`, per-workload raw JSON, preflight/timing logs,
`summary.json`, `reverse-summary.json`, `matrix.md`, source/control patches and `metadata.json`
with commits, flags, capture sizes and SHA-256 hashes. Previous matrix results were not overwritten.

### Random wide UTF-8 breakdown (2026-11-04)

At 0.33x, half of the gap was combining marks and half the base print cost. Against Ghostty main,
same machine; removing one property from the stream at a time:

| Stream | eterm | Ghostty | Ratio |
|---|---|---|---|
| Full stream (26.8M codepoints) | 87.1 | 261.0 | 0.33x |
| No combining marks or format characters | 151.8 | 389.2 | 0.39x |
| No unassigned or private-use | 77.5 | 212.1 | 0.37x |
| Assigned BMP without marks | 149.2 | 341.6 | 0.44x |
| Same, wide characters only | 276.3 | 569.1 | 0.49x |
| Same, alternating narrow and wide | 225.1 | 387.6 | 0.58x |

Marks are 3-4% of the codepoints and took 44% of eterm's time. After a mark the previous cell is a
cluster, and `tpututf8()` discarded the whole decoded run for `tputc()`: 525,057 times, 33.4M
codepoints decoded for 26.8M in the stream. It now asks UAX #29 about the first codepoint and keeps
the run at a boundary, the same answer `tattach()` reaches: 89 -> 115 MB/s (+29%). Final screens
matched `HEAD` in 240 fuzz comparisons, half weighted toward clusters that must grow.

The rest is the base print path: about 14 vs 6 ns per codepoint on clean text. History size and
scrolling (5% of samples) do not explain it, and neither do random widths. 55% of the time is in the
inlined `tputrun()` cell loop and 23% in decoding, but the profile does not divide that loop.

### Also noticed

- The framework retry linked the test binary against `@rpath/SDL2.framework`,
  but its runtime lookup failed. SDL2 is installed at
  `/Library/Frameworks/SDL2.framework`; it is not missing. The generated macOS
  makefiles carry a literal `$ORIGIN` RPATH, and the framework setup supplies
  `-F /Library/Frameworks` without the corresponding runtime search path.
  Rebuilding the test target through `make_no_fw.sh` restored its Homebrew SDL2
  linkage and all tests passed. The framework RPATH issue is recorded separately;
  this performance change does not modify the build scripts.
- `erresc: unknown csi` diagnostics (kitten sends `CSI * x` and `CSI ? r`) only print in debug
  builds or with `ETERM_LOG_ERRORS` set (`terminalDiagnosticsEnabled()`); release runs are quiet
  and pay one cached check. Nothing to do.

## How to test and compare against Ghostty

### Tools

- `kitty` (for `kitten __benchmark__`, used only to capture payloads), `hyperfine`, `python3`.
- Zig **0.17.0** for Ghostty main (`build.zig.zon` `minimum_zig_version`). Download from
  `https://ziglang.org/download/index.json` if `zig version` differs.
- macOS profiling: Superluminal CLI is used on Linux; on macOS use `sample <pid> 4 -file out.txt`
  (look at "Sort by top of stack") or Instruments (`xctrace record --template 'Time Profiler'`).

### 1. Build Ghostty's benchmark tools

```sh
git clone --depth 1 https://github.com/ghostty-org/ghostty.git
cd ghostty
zig build -Demit-bench -Doptimize=ReleaseFast -Demit-macos-app=false   # drop the last flag on Linux
ls zig-out/bin   # ghostty-bench, ghostty-gen
```

Record the Ghostty commit used. `ghostty-bench` options use `--key=value` syntax (`--data=file`,
not `--data file`).

On macOS the bench binaries link the Metal renderer; if the build fails with "missing Metal
Toolchain", run `xcodebuild -downloadComponent MetalToolchain` (~700 MB) and rebuild.

### 2. Capture the kitten payloads once

Save as `capture.py`. It runs the kitten on a 120x80 PTY, answers each `CSI 5 n` with `CSI 0 n`,
and stores every byte the kitten wrote:

```python
#!/usr/bin/env python3
"""Capture the exact bytes `kitten __benchmark__` writes to its terminal.

Runs the kitten on a 120x80 PTY, answers each DSR 5n "finished" query with
DSR 0n, and stores everything the kitten wrote to <out>."""
import fcntl, os, pty, select, struct, sys, termios
out, reps, name = sys.argv[1], sys.argv[2], sys.argv[3]
pid, fd = pty.fork()
if pid == 0:
    os.execvp("kitten", ["kitten", "__benchmark__", "--repetitions", reps, name])
fcntl.ioctl(fd, termios.TIOCSWINSZ, struct.pack("HHHH", 80, 120, 0, 0))
query = b"\x1b[5n"
with open(out, "wb") as f:
    tail = b""
    while True:
        r, _, _ = select.select([fd], [], [], 30)
        if not r:
            sys.exit("timeout")
        try:
            data = os.read(fd, 1 << 20)
        except OSError:
            break
        if not data:
            break
        f.write(data)
        window = tail + data
        n = window.count(query)
        if n:
            os.write(fd, b"\x1b[0n" * n)
        tail = window[-(len(query) - 1):]
os.waitpid(pid, 0)
```

Repetitions chosen so every file is ~64 MB or more (run under bash; zsh does not word-split
`$spec`):

```sh
bash -c 'for spec in "ascii 32" "unicode 40" "unique_unicode 36" "csi 64" "images 12" "long_escape_codes 8"; do
  set -- $spec; python3 -I capture.py $1.vt $2 $1 && echo "$1 $(wc -c < $1.vt)"; done'
```

Keep the `.vt` files and reuse them for every comparison (Ghostty's own benchmark guidance: never
pipe a generator into the benchmark when comparing).

### 3. eterm harness

Save as `vtbench.cpp`. It feeds a captured stream to `TerminalEmulator` in 64 KiB reads at 120x80
with a no-op display, and **fails unless every byte is consumed and at least 6 `CSI 0 n` replies
were written** (the kitten's 3 warmup + 3 final status queries sit at the end of the stream). That
check is what proves eterm parsed everything, unlike Velocitty.

```cpp
// Headless eterm throughput over captured `kitten __benchmark__` streams.
// Every byte goes through TerminalEmulator::update(); nothing is skipped. Completeness is
// checked by counting the DSR 0n replies to the kitten's trailing DSR 5n queries.
#include <eterm/terminal/terminalemulator.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
using namespace eterm::Terminal;
using namespace eterm::System;
class Pty : public IPseudoTerminal {
  public:
	std::string_view data;
	size_t offset = 0;
	size_t chunk = 64 * 1024;
	std::string replies;
	int getNumColumns() const override { return 120; }
	int getNumRows() const override { return 80; }
	bool resize( int, int, int, int ) override { return true; }
	bool isTTY() const override { return true; }
	int write( const char* s, size_t n ) override { replies.append( s, n ); return n; }
	int read( char* out, size_t n, bool ) override {
		n = std::min( { n, chunk, data.size() - offset } );
		memcpy( out, data.data() + offset, n );
		offset += n;
		return n;
	}
};
class Process : public IProcess {
  public:
	void checkExitStatus() override {}
	bool hasExited() const override { return false; }
	int getExitCode() const override { return 0; }
	void terminate() override {}
	void waitForExit() override {}
	int pid() override { return 1; }
};
class Display : public ITerminalDisplay {
  public:
	bool drawBegin( Uint32, Uint32 ) override { return true; }
	void drawLine( Line, int, int, int ) override {}
	void drawCursor( int, int, TerminalGlyph, int, int, TerminalGlyph ) override {}
	void drawEnd() override {}
};
int main( int argc, char** argv ) {
	std::ifstream in( argv[1], std::ios::binary | std::ios::ate );
	std::string text( static_cast<size_t>( in.tellg() ), '\0' );
	in.seekg( 0 );
	in.read( text.data(), text.size() );
	const int runs = argc > 2 ? atoi( argv[2] ) : 3;
	std::vector<double> rates;
	for ( int run = 0; run < runs; ++run ) {
		auto pty = std::make_unique<Pty>();
		auto* ptr = pty.get();
		ptr->data = text;
		auto term = TerminalEmulator::create( std::move( pty ), std::make_unique<Process>(),
											  std::make_shared<Display>(), 10000 );
		const auto start = std::chrono::steady_clock::now();
		while ( ptr->offset < text.size() )
			term->update();
		term->update();
		const double seconds =
			std::chrono::duration<double>( std::chrono::steady_clock::now() - start ).count();
		size_t acks = 0;
		for ( size_t at = 0; ( at = ptr->replies.find( "\x1b[0n", at ) ) != std::string::npos; at += 4 )
			++acks;
		if ( ptr->offset != text.size() || acks < 6 ) {
			fprintf( stderr, "INCOMPLETE: consumed %zu/%zu, DSR replies %zu\n", ptr->offset,
					 text.size(), acks );
			return 1;
		}
		rates.push_back( text.size() / seconds / 1e6 );
	}
	std::sort( rates.begin(), rates.end() );
	printf( "%s %.1f MB/s (min %.1f max %.1f)\n", argv[1], rates[rates.size() / 2], rates.front(),
			rates.back() );
}
```

Build it against an eterm source tree (`V` = a repository root or a copy of one; compile the eterm
terminal/system sources directly so different revisions can be compared side by side). macOS, after
`projects/macos/make_no_fw.sh config=release` has produced `libs/macosx/libeepp.dylib`:

```sh
#!/bin/bash
# build-vtbench.sh <eterm-tree-root> <output-binary>
set -e
V=$1; OUT=$2; R=$HOME/programming/eepp   # tree providing include/ and libs/macosx/libeepp.dylib
O=$(mktemp -d)
FLAGS="-O3 -g -std=c++20 -DNDEBUG -I$R/include -I$V/src/modules/eterm/include -I$V/src/modules/eterm/src"
for f in $V/src/modules/eterm/src/eterm/terminal/*.cpp $V/src/modules/eterm/src/eterm/system/*.cpp; do
  clang++ $FLAGS -c $f -o $O/$(basename $f).o &
done
clang++ $FLAGS -c "$(dirname "$0")/vtbench.cpp" -o $O/vtbench.o &
wait
clang++ -o $OUT $O/*.o -L$R/libs/macosx -leepp -Wl,-rpath,$R/libs/macosx
rm -rf $O
```

Run with `DYLD_LIBRARY_PATH=$HOME/programming/eepp/libs/macosx` (the dylib install name is
relative). On Linux use `g++`, add `-fPIC -m64`, link `-L$R/libs/linux/x86_64 -leepp
-Wl,-rpath,$R/libs/linux/x86_64 -lpthread`.

Quick completeness check of all workloads:

```sh
for f in *.vt; do ./eterm-vtbench $f 1 2>/dev/null || echo "$f FAILED"; done
```

### 4. Head-to-head

```sh
GB=/path/to/ghostty/zig-out/bin/ghostty-bench
for b in ascii unicode unique_unicode csi images long_escape_codes; do
  hyperfine -N --warmup 1 --runs 3 --export-json h-$b.json \
    -n ghostty "$GB +terminal-stream --data=$b.vt --terminal-cols=120 --terminal-rows=80" \
    -n eterm "./eterm-vtbench $b.vt 1"
done
python3 - <<'EOF'
import json, os
print(f"{'workload':18s} {'ghostty MB/s':>13s} {'eterm MB/s':>11s} {'ratio':>6s}")
for b in ['ascii', 'unicode', 'unique_unicode', 'csi', 'images', 'long_escape_codes']:
    size = os.path.getsize(f'{b}.vt')
    # Older hyperfine: {x['command']: x['median'] ...}; newer exports use name + summary.
    r = {x['name']: x['summary']['time_wall_clock']['median'] for x in json.load(open(f'h-{b}.json'))['results']}
    g = size / r['ghostty'] / 1e6; e = size / r['eterm'] / 1e6
    print(f"{b:18s} {g:13.1f} {e:11.1f} {g / e:5.1f}x")
EOF
```

On Linux prefix both commands with `taskset -c 2`. Both are timed as whole processes and both must
stream the file: the `vtbench.cpp` above loads it up front, which was harmless at ~150 MB/s but
cost eterm ~30% once string bodies ran at GB/s. The current harness in `~/programming/vtbench`
(Mac) reads the file in 64 KiB `read()` calls inside `Pty::read()` and has a `DUMP=<file>` mode
that writes the final cells and cursor, used to diff screens between parser revisions (extract a
pristine tree with `git archive HEAD src/modules/eterm | tar -x -C head`). Never use
`std::istreambuf_iterator`; that alone made eterm look ~2x slower on Images.

### Measurement pitfalls seen so far

- **Code placement on Apple M-series:** one fixed eterm binary measured 19% slower than staged with
  no links; relinking the identical code removed the difference. Before trusting a small ARM64
  delta, rebuild/relink both binaries and re-run alternating rounds.
- **macOS has no `taskset`;** threads can land on efficiency cores. Run several alternating rounds
  and compare medians. `taskpolicy -c utility` forces E-cores and is useful only as a sanity check.
- **Linux `powersave` governor** adds noise; keep comparisons interleaved.
- Run one benchmark or build at a time.

### Correctness gates for every optimization

- `bin/unit_tests/eepp-unit_tests --filter="eterm*"` (145 tests at the time of writing), on Linux
  and macOS (`cd bin/unit_tests && ./eepp-unit_tests ...` on macOS so the dylib is found).
- The vtbench completeness check above for all six streams.
- For parser changes, additionally compare final screen contents and cursor position against the
  pre-change parser for each `.vt` file (dump `TLINE` rows after the run) so fast paths cannot
  silently diverge.
- Keep the OSC 8 regression benchmark below green: no-link throughput within noise of the previous
  build, and distinct-link and dense-snapshot numbers no worse.

## OSC 8 regression benchmark

Used during the hyperlink work. Parser throughput for plain text, a periodic explicit-ID link,
and (with `DISTINCT=1`) a distinct link on every line, plus snapshot round-trips with dense links.
`PARSER_ONLY=1` skips the snapshot part; `TRIALS=n` changes the trial count. Compile it in place of
`vtbench.cpp` with `-DCURRENT` for trees that have `TerminalHyperlink`.

```cpp
#include <eterm/terminal/terminalemulator.hpp>
#include <eterm/terminal/terminalsession.hpp>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
using namespace eterm::Terminal;
using namespace eterm::System;
class Pty : public IPseudoTerminal {
public:
 std::string_view data;
 size_t offset=0;
 int columns=120,rows=40;
 int getNumColumns() const override{return columns;}
 int getNumRows() const override{return rows;}
 bool resize(int c,int r,int,int) override{columns=c;rows=r;return true;}
 bool isTTY() const override{return true;}
 int write(const char*,size_t n) override{return n;}
 int read(char* out,size_t n,bool) override{n=std::min(n,data.size()-offset);memcpy(out,data.data()+offset,n);offset+=n;return n;}
};
class Process : public IProcess {
public:
 void checkExitStatus() override{}
 bool hasExited() const override{return false;}
 int getExitCode() const override{return 0;}
 void terminate() override{}
 void waitForExit() override{}
 int pid() override{return 1;}
};
class Display : public ITerminalDisplay {
public:
 bool drawBegin(Uint32,Uint32) override{return true;}
 void drawLine(Line,int,int,int) override{}
 void drawCursor(int,int,TerminalGlyph,int,int,TerminalGlyph) override{}
 void drawEnd() override{}
};
int main(){
 printf("sizes: glyph=%zu hyperlink=%zu snapshot=%zu options=%zu\n",sizeof(TerminalGlyph),
#ifdef CURRENT
 sizeof(TerminalHyperlink),sizeof(TerminalSnapshot),sizeof(TerminalSessionOptions)
#else
 size_t(0),sizeof(TerminalSnapshot),size_t(0)
#endif
 );
 const bool parserOnly=getenv("PARSER_ONLY")!=nullptr;
 for(bool links:{false,true}){
  if(parserOnly&&links!=(getenv("DISTINCT")!=nullptr))continue;
  std::string text; text.reserve(24*1024*1024);
  for(int i=0;i<180000;i++){
   if(links&&getenv("DISTINCT"))text+="\033]8;;https://example.com/file/"+std::to_string(i)+"\033\\";
   else if(links&&i%20==0)text+="\033]8;id=bench;https://example.com/path?q=1;value\033\\";
   text+="terminal benchmark text 0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz 0123456789\r\n";
   if(links&&(getenv("DISTINCT")||i%20==0))text+="\033]8;;\033\\";
  }
  std::vector<double> samples;
  const int trials=getenv("TRIALS")?atoi(getenv("TRIALS")):7;
  for(int trial=0;trial<trials;trial++){
   auto pty=std::make_unique<Pty>();auto* ptr=pty.get();ptr->data=text;
   auto display=std::make_shared<Display>();
   auto term=TerminalEmulator::create(std::move(pty),std::make_unique<Process>(),display,10000);
   auto start=std::chrono::steady_clock::now();
   while(ptr->offset<text.size())term->update();
   auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
   samples.push_back(text.size()/elapsed/1024/1024);
  }
  std::sort(samples.begin(),samples.end());
  printf("parser links=%d median=%.2f MiB/s range=%.2f..%.2f\n",links,samples[samples.size()/2],samples.front(),samples.back());
 }
 if(parserOnly)return 0;
 for(auto grid:{std::pair{80,24},std::pair{120,40},std::pair{240,80}}){
  for(bool links:{false,true}){
   auto pty=std::make_unique<Pty>();pty->columns=grid.first;pty->rows=grid.second;
   std::string text;
   for(int i=0;i<grid.first*grid.second/2;i++){
    if(links)text+="\033]8;;https://example.com/"+std::to_string(i)+"\a";
    text+='X';
   }
   if(links)text+="\033]8;;\a";
   text+="\033]2;done\a";
   pty->data=text;
   auto session=TerminalSession::create(std::move(pty),std::make_unique<Process>(),10000);
   auto start=std::chrono::steady_clock::now();
   while(session->snapshot()->title!="done")std::this_thread::yield();
   auto ready=session->snapshot();
   while(ready->cursor.y<(grid.second/2-1)){std::this_thread::yield();ready=session->snapshot();}
   auto begin=std::chrono::steady_clock::now();
   for(int i=0;i<200;i++){
    session->setFocus(i%2==0);
    const auto generation=ready->generation;
    while((ready=session->snapshot())->generation<=generation)std::this_thread::yield();
   }
   double us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count()/200;
   printf("snapshot grid=%dx%d links=%d roundtrip=%.2f us\n",grid.first,grid.second,links,us);
  }
 }
}
```

Last results (medians of 3 alternating rounds; HEAD = before the OSC 8 work, which ignored links):

| Workload | x86_64 HEAD / fixed | ARM64 (M-series) HEAD / fixed |
| --- | --- | --- |
| No links (MiB/s) | 127.7 / 125.5 | 144.6 / 154.0 |
| Shared `id=` link every 20 lines | 130.4 / 124.6 | 143.1 / 149.3 |
| Distinct link every line | 114.7 / 100.7 | 140.7 / 126.5 |
| Dense-link snapshot 240x80 (us) | 52 / 401 | 43 / 111 |

## References

- Ghostty 1.4.0 release notes (parser, IO thread split, SIMD):
  https://ghostty.org/docs/install/release-notes/1-4-0
- Third-party reproduction of Ghostty's kitten numbers with `ghostty-bench +terminal-stream`:
  https://github.com/cristicretu/diri/pull/914
- Ghostty benchmark workflow: `src/benchmark/AGENTS.md` in the Ghostty repository.
- kitten benchmark source: `tools/cmd/benchmark/main.go` in the kitty repository (v0.48.2).
