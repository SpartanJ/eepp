# eterm VT Throughput vs Ghostty Plan - 2026-10-30

## Status

Investigation and measurement are done; no throughput optimization is implemented yet.

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

**First task on the Mac: re-run this baseline there** (M-series). Expect higher absolute numbers for
both terminals; keep using ratios.

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

### 3. Bulk UTF-8 decode, faster widths, batched non-ASCII runs

Use simdutf (already vendored, `simdutf-static`) to decode runs up to the next control/ESC, then
classify widths with a two-stage lookup table instead of `wcwidth()`'s current lookup. Batch runs
of width-1 and width-2 codepoints like item 2; send width-0 and anything special to the slow path.
Avoid the per-character `utf8encode()` in `tputc()`; it only matters for `MODE_PRINT`.

- Expected: most of the 5.2x Unicode gap.

### 4. Images: SIMD base64 and buffer reuse

Profile `KittyGraphicsProtocol` transmit: replace scalar `Base64::decode` on the hot path with a
SIMD decoder (or simdutf's base64), and reuse the decode/image buffer across transmissions of the
same size instead of allocating and freeing 4 MiB each time.

### 5. CSI dispatch (only after 1-4)

Profile first. Candidates: consuming back-to-back escape sequences without returning through
`twrite()`'s per-byte loop, and cheaper parameter parsing in `csiparse()`.

### 6. Grapheme clustering (correctness, not speed)

Attach width-0 codepoints (combining marks, ZWJ sequences, variation selectors) to the previous
cell. Needed before the Unique Unicode workload means anything, and important for real text. Design
storage carefully: `TerminalGlyph` must stay 16 bytes, so extra codepoints need a side table keyed
like the hyperlink registry (IDs, mark/sweep), not per-cell heap data.

### Also noticed

- eterm prints an `erresc: unknown csi` diagnostic for every unknown sequence (kitten sends
  `CSI * x` and `CSI ? r`). Consider rate-limiting or debug-only logging.

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
    r = {x['command']: x['median'] for x in json.load(open(f'h-{b}.json'))['results']}
    g = size / r['ghostty'] / 1e6; e = size / r['eterm'] / 1e6
    print(f"{b:18s} {g:13.1f} {e:11.1f} {g / e:5.1f}x")
EOF
```

On Linux prefix both commands with `taskset -c 2`. Both are timed as whole processes: Ghostty
streams its file while parsing, eterm reads it up front with one `read()`. Do not load the file with
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
