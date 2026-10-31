# eterm VT Throughput vs Ghostty Plan - 2026-10-30

## Status

Items 1-5 are committed in `cef605021`; grapheme phase 1 is committed in `b4b4aa8c1`.
Grapheme shaping and review fixes are in the working tree. The mode-controlled follow-up below
corrects the apparent performance regression caused by comparing different reset defaults.

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

### Also noticed

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
