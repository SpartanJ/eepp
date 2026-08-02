# eepp Linux Build-Time Optimization Plan

Status: **Planning complete; no implementation or benchmark run has started.**

Last review: 2026-08-02.

## 1. Objective

Reduce clean and incremental Linux compilation time for eepp and ecode without lowering the
current optimization level and without degrading runtime performance, memory use, API quality,
or supported platforms.

The work must be measurement-driven. Do not perform broad include removal, container replacement,
PImpl conversion, unity builds, or precompiled-header adoption without showing which measured cost
the change addresses and whether it improves the relevant end-to-end build.

The primary machine for the initial investigation is:

```text
AMD Ryzen 9 3900X
12 cores / 24 hardware threads
Linux
Clang 22.1.8
Ninja
```

The repository supports Clang time tracing in both generators: `premake4 --time-trace` and
`premake5 --time-trace` both add `-ftime-trace` to compile commands. They serve different normal
workflows in this repository:

- **debug and unit-test builds use Premake 4 with the GNU Make generator**, following
  `.agent/rules/build-project.md`;
- **release build-time and runtime-performance investigations use Premake 5 with the Ninja
  generator**, following the `eepp-linux-ninja` configuration in `.ecode/project_build.json`.

Do not substitute one workflow for the other merely because both generators expose
`--time-trace`. Use Premake 5/Ninja for the primary release compilation-time baseline and Premake
4/GNU Make when specifically measuring or validating the normal debug workflow.

## 2. Scope and non-goals

### In scope

- eepp library C++ translation units;
- ecode C++ translation units;
- eepp modules and tools when they are part of a measured developer workflow;
- public and private header fan-out;
- template parsing and instantiation cost;
- large generated language-syntax translation units;
- clean builds and representative incremental rebuilds;
- compiler-cache integration for normal developer builds;
- selective precompiled headers or unity builds if measurement justifies them;
- link-time measurement, while keeping compilation and linking results separate.

### Out of scope unless evidence changes the decision

- changing release or debug optimization levels;
- optimizing unchanged third-party C libraries;
- rewriting HarfBuzz or maintaining a private HarfBuzz fork;
- treating already up-to-date object files as a recurring build cost;
- replacing containers based only on header size or reputation;
- runtime-performance regressions in exchange for faster compilation;
- global PImpl conversion or other ABI-wide redesign;
- C++20 modules as an initial solution;
- using an AddressSanitizer build as the performance baseline.

Third-party sources are already handled correctly by incremental dependency tracking: unchanged
objects are not rebuilt. Most third-party units are C and compile quickly. HarfBuzz is a known
heavy dependency, but is considered residual cost rather than an optimization target. Report its
time separately so it does not obscure improvements to project-owned code.

## 3. Current evidence and hypotheses

At the time this plan was written:

- `make/linux/release_x86_64/compile_commands.json` contained 1,989 commands: 1,283 C++ and 706 C;
- generated commands invoked `clang` / `clang++` directly rather than consistently using ccache or
  sccache;
- ccache, sccache, mold, and Ninja were installed on the machine;
- no project PCH or unity-build configuration was found;
- historical `.ninja_log` entries showed expensive project-owned objects including `ecode.o`,
  `chatui.o`, `lspclientserver.o`, several ecode plugin objects,
  `stylesheetspecification.o`, `uicodeeditor.o`, `uihtml_tests.o`, and generated language syntax
  objects;
- `syntaxdefinitionmanager.hpp` was directly included by at least 169 C++ sources;
- large foundational public headers included `scene/node.hpp`, `ui/uinode.hpp`, `ui/uiwidget.hpp`,
  `core/string.hpp`, `ui/uiscenenode.hpp`, `ui/uicodeeditor.hpp`, `network/http.hpp`, and
  `ui/doc/textdocument.hpp`.

These observations identify candidates, not conclusions. Ninja durations from a parallel build
include CPU and memory contention and must not be treated as isolated TU compile times.

The leading hypotheses are:

1. generated syntax-definition units repeatedly parse more manager/UI infrastructure than needed;
2. large ecode and UI source files receive expensive transitive dependency trees;
3. implementation-only types are exposed through common headers;
4. substantial inline or template code is instantiated repeatedly;
5. developer rebuilds are missing avoidable compiler-cache hits;
6. a carefully scoped PCH may reduce repeated standard-library and stable framework parsing;
7. selective unity grouping may help stable, homogeneous source families, but could hurt
   incremental builds and peak memory.

## 4. Required operating rules

Before any work, read:

```text
.agent/SOUL.md
.agent/rules/project-introduction.md
.agent/rules/build-project.md
.ecode/project_build.json
```

Also follow these rules:

1. Run all Premake and build commands from the repository root unless the command explicitly uses
   `-C make/linux`.
2. Record `git status --short` before touching files. Preserve all pre-existing user changes.
3. Do not commit, push, reset, or discard user work.
4. Regenerate project files after source or Premake changes, as required by the build rules.
5. Format every modified C/C++ file with `clang-format` before compilation.
6. Keep benchmark artifacts outside tracked source directories, preferably under
   `/tmp/eepp-build-time-<date>/`.
7. Do not mix ASan results with build-time baseline results.
8. Do not compare runs generated with different compilers, flags, backends, target sets, cache
   modes, or background load.
9. Repeat important measurements at least three times and report the median. If variance exceeds
   5%, investigate noise before claiming a small improvement.
10. Separate clean-build, no-op, incremental, cache-hit, and isolated-TU results.
11. Preserve `-O3` for release experiments. Build-time changes must not come from reducing
    optimization.
12. After every structural C++ change, run an allocation and runtime-performance audit as required
    by `.agent/SOUL.md`.
13. Preserve the repository's generator split: Premake 4/GNU Make for normal debug and unit-test
    workflows, and Premake 5/Ninja for release build-time investigations. Do not compare their
    timings as though they were the same build configuration.

## 5. Benchmark matrix

Do not use a single `ninja release` duration as the only metric. Establish the following named
workflows.

| ID | Workflow | Purpose |
|---|---|---|
| B1 | clean eepp-owned library build | framework clean-build cost |
| B2 | clean ecode build including required dependencies | real application clean-build cost |
| B3 | no-op repeat of B2 | generator/dependency overhead sanity check |
| B4 | edit/touch one leaf `.cpp`, rebuild ecode | common local edit latency |
| B5 | touch a widely used eepp core header, rebuild | public-header blast radius |
| B6 | touch a widely used UI header, rebuild | UI dependency blast radius |
| B7 | touch syntax-definition interface, rebuild | generated syntax fan-out |
| B8 | isolated compilation of each top slow C++ TU | remove parallel contention |
| B9 | cache-enabled rebuild after removing only selected outputs | compiler-cache benefit |
| B10 | link-only relink | keep link cost separate from compile cost |
| B11 | normal Premake 4/GNU Make debug build and representative incremental rebuild | ensure improvements also help or at least do not harm the debug workflow |

The executing agent must first inspect `make/linux/build.ninja` with `ninja -t targets` and identify
the exact target names for eepp, ecode, and their configurations. Do not guess target names. Store
the resolved commands in the benchmark report.

For header invalidation tests, prefer `touch` followed by restoring the original timestamp if that
can be done safely, or make a reversible whitespace change in a clean file. Never overwrite user
changes. Record the exact header and why it represents the workflow.

## 6. Phase 0: Prepare the investigation

### Tasks

1. Capture repository state and tool versions:

   ```bash
   git status --short
   premake4 --version
   premake5 --version
   clang --version
   clang++ --version
   ninja --version
   ccache --version
   sccache --version
   mold --version
   nproc
   lscpu
   ```

2. Re-read `.ecode/project_build.json` and use `eepp-linux-ninja` as the source of truth.
3. Inspect available Ninja targets:

   ```bash
   ninja -C make/linux -t targets all
   ```

4. Inspect representative compile commands and confirm:
   - compiler;
   - `-O3` in release;
   - debug-symbol setting;
   - SDL backend;
   - architecture;
   - whether ccache/sccache is actually in the command;
   - whether the target compiles only required dependencies or the whole workspace.
5. Confirm both time-trace options remain defined before relying on them:

   ```bash
   premake4 --help | rg 'time-trace'
   premake5 --help | rg 'time-trace'
   ```

6. Create an untracked results directory under `/tmp`, containing:
   - `environment.txt`;
   - `commands.txt`;
   - `baseline.tsv`;
   - `traces/`;
   - `reports/`.

### Exit criteria

- Exact target names are known.
- Benchmark commands are reproducible.
- Existing user changes are documented and protected.
- No source code has changed.

## 7. Phase 1: Establish baselines

### Build generation

Use the current non-ASan Premake 5/Ninja configuration for the primary release baseline:

```bash
premake5 --disable-static-build --with-debug-symbols --with-backend=SDL3 ninja
```

If `.ecode/project_build.json` changes before execution, follow the updated configuration instead
and document the difference.

The normal debug workflow is separate. Generate it with Premake 4 and GNU Make, using the exact
current command prescribed by `.agent/rules/build-project.md` and including the conditional mold
flag when required. This debug build may use AddressSanitizer because that is the project's normal
debug/test configuration, but never use its timings as release-performance measurements or compare
them directly with the Premake 5/Ninja release baseline.

### Timing method

Use `/usr/bin/time` so wall time, CPU time, and maximum resident set size are captured. A template
is:

```bash
/usr/bin/time -f 'wall=%e user=%U sys=%S cpu=%P maxrss_kb=%M exit=%x' \
  ninja -C make/linux <resolved-target>
```

Run each meaningful baseline three times under the same conditions. For clean builds, use the
narrowest safe Ninja clean operation for the measured target/configuration. Inspect the clean
command before running it; do not delete the repository or broad directories manually.

Record:

- run ID and timestamp;
- exact generation and build command;
- cold/warm filesystem-cache state, without forcibly dropping kernel caches;
- compiler-cache enabled/disabled state;
- wall, user, system, CPU%, and peak RSS;
- number of commands executed;
- target result size and link duration where available;
- relevant system load and CPU frequency governor.

Do not use `ninja -d stats` output as a replacement for wall-clock timing, but capture it where
useful.

### Analyze historical Ninja data

Use `.ninja_log` only as a candidate generator. Since it can contain repeated historical entries,
group by output and report the latest run or distribution rather than blindly taking the maximum.

Suggested extraction starting point:

```bash
awk 'NR > 1 && $2 >= $1 { print $2-$1, $4 }' make/linux/.ninja_log | sort -nr
```

Classify entries as:

- project-owned eepp;
- project-owned ecode/tools/modules/tests;
- generated syntax definitions;
- HarfBuzz;
- other third-party C/C++;
- linking.

### Exit criteria

- B1 through B7 and B11 have commands and initial measurements, with debug and release results
  kept separate.
- No-op B3 executes zero unexpected compiler commands.
- Third-party and link costs are separated from project-owned compilation.
- Variance is understood well enough to evaluate later changes.

## 8. Phase 2: Capture and aggregate Clang time traces

### Generate traced build files

Regenerate the primary release trace build with Premake 5 while preserving all baseline options:

```bash
premake5 --disable-static-build --with-debug-symbols --with-backend=SDL3 --time-trace ninja
```

Confirm a representative C++ compile command contains both `-O3` and `-ftime-trace`.

If tracing the debug workflow as a separate investigation, add `--time-trace` to the current
Premake 4 debug generation command from `.agent/rules/build-project.md`, then build with GNU Make.
Keep those trace reports in a separate `debug-premake4/` results directory. Debug/ASan traces may
identify dependency fan-out, but their total durations and optimization/backend costs are not
comparable to the Premake 5 release traces.

Run a clean, scoped traced build. Trace instrumentation adds overhead, so do not compare traced
wall time directly with the untraced baseline. Its purpose is attribution.

Before compiling, determine where this Clang version writes trace JSON files. Copy them to the
temporary results directory after the build while retaining a mapping from trace to source/object.
Do not add trace JSON files to Git.

### Aggregate these categories

For every C++ trace, extract at least:

- total compiler duration;
- frontend duration;
- backend/optimizer duration;
- source/header parsing totals;
- template instantiation totals;
- code generation totals;
- expensive individual headers;
- expensive template specializations where Clang names them.

Produce these reports:

```text
reports/tu-total.tsv
reports/frontend.tsv
reports/backend.tsv
reports/header-self.tsv
reports/header-cumulative.tsv
reports/template-instantiation.tsv
reports/project-vs-third-party.tsv
```

If no existing repository tool aggregates traces adequately, create a small standalone analysis
script under the temporary results directory first. Only add a reusable script to the repository
later if it proves valuable. The parser must stream or process files one at a time rather than load
all trace files into memory simultaneously.

### Required ranking method

Rank headers by both:

1. expensive appearance in one TU;
2. cumulative cost across all project-owned TUs.

Also record include fan-out. A 100 ms header parsed in 200 units is usually more valuable than a
one-second header parsed once.

### Isolated TU confirmation

For the top 10–20 project-owned C++ objects, extract the exact compile command from
`compile_commands.json` or Ninja and execute it serially with `/usr/bin/time`. Preserve its output
path safely or direct experimental output into `/tmp`; do not corrupt normal build dependencies.

Run each top candidate at least three times. This distinguishes intrinsic cost from contention in
the original parallel build.

### Exit criteria

- At least 80% of project-owned C++ compilation time is classified by subsystem or candidate.
- Top headers are ranked by cumulative cost and fan-out.
- Top slow TUs have isolated measurements.
- Frontend-heavy and backend-heavy candidates are separated.

## 9. Phase 3: Header dependency and syntax-definition investigation

This is the first source-level optimization phase.

### 9.1 Generated syntax definitions

Start here if traces confirm the current hypothesis.

Inspect the generated language source family and answer:

- Why does each source include `syntaxdefinitionmanager.hpp`?
- Does registration require the full manager definition?
- Can definition construction use a small declaration/value header?
- Can manager registration move to one aggregation `.cpp`?
- Are large initializer expressions causing frontend or backend cost?
- Are identical template specializations emitted repeatedly?
- Can data be expressed in a representation that compiles faster without adding startup work,
  heap churn, or runtime parsing?

Preferred low-risk direction:

```text
small syntax-definition declaration/data interface
        -> individual generated language units
full manager implementation
        -> one or a few aggregation/registration units
```

Do not merge all languages into one huge source unless an isolated experiment shows acceptable
incremental behavior and memory use.

### 9.2 High cumulative-cost headers

For every top header, use preprocessing/include-tree tools to identify why it is present. Useful
commands include the exact compile command augmented with one of:

```text
-H
-E
-ftime-trace
```

Investigate:

- includes needed only by `.cpp` implementation;
- pointer/reference members that can use forward declarations;
- inline functions whose definitions require heavy dependencies;
- nested type references that force full includes;
- callbacks using `std::function` in ubiquitous public APIs;
- private concrete containers exposed in class layout;
- umbrella/convenience headers included by lower layers;
- templates that can be explicitly instantiated;
- duplicated helper templates or traits.

### Change rules

Make one logical dependency change per benchmarkable patch. For each change:

1. record the baseline candidate metrics;
2. implement the smallest correction;
3. regenerate build files;
4. rebuild the affected target;
5. run relevant tests;
6. repeat isolated TU measurement;
7. repeat the relevant clean/incremental benchmark;
8. record runtime/allocation/API consequences;
9. revert changes that do not produce a repeatable useful gain.

### PImpl warning

Do not introduce PImpl solely to hide includes when it adds per-object allocation or indirection to
hot eepp types. Prefer, in order:

1. forward declaration without layout changes;
2. moving out-of-line function bodies;
3. small non-owning interface types;
4. splitting stable data from heavy behavior;
5. PImpl only for cold, coarse-grained objects where allocation and ABI tradeoffs are acceptable.

### Exit criteria

- At least the top five cumulative project header costs have been explained.
- The syntax-definition hypothesis has either produced a measured improvement or been rejected
  with evidence.
- Accepted changes improve B1/B2/B5/B6/B7 as applicable, not merely preprocessing byte count.
- All affected tests pass.

## 10. Phase 4: Template and container cost

Only begin this phase if time traces show meaningful template parsing or instantiation cost.

### Investigate first

- which exact templates dominate;
- number of unique versus repeated specializations;
- whether cost is parsing, instantiation, optimization, or debug information;
- whether the specialization is required in public headers;
- whether explicit instantiation is legal and useful;
- whether a non-template interface boundary would preserve runtime performance.

### Candidate techniques

- `extern template` declarations plus explicit instantiation in one `.cpp`;
- moving template-heavy operations out of common headers;
- reducing accidental type variation that creates near-duplicate specializations;
- replacing a container only when both build-time traces and runtime requirements support it;
- using spans/views at API boundaries to avoid exporting container implementation choices.

Do not assume eepp's `UnorderedMap` or `UnorderedSet` is compile-time cheaper merely because it is
runtime-preferred. `include/eepp/thirdparty/unordered_dense.h` is itself substantial. Compare small
representative compilations and affected end-to-end targets before changing types.

### Acceptance gate

A container/template change is accepted only if:

- it improves a named build metric beyond noise;
- runtime benchmarks do not regress materially;
- memory/allocation behavior is equal or better, or a tradeoff is explicitly approved;
- public API and serialization behavior remain correct;
- cross-platform compilers remain supported.

## 11. Phase 5: Compiler-cache integration

This phase improves developer workflow but must be reported separately from structural cold-build
improvements.

### Procedure

1. Inspect how Premake's Ninja generator selects `CC` and `CXX`.
2. Prototype ccache first because the machine already has it and it is straightforward locally.
3. Prefer a compiler launcher or generated-command prefix over replacing the compiler identity in
   a way that breaks dependency generation.
4. Confirm commands actually invoke ccache.
5. Clear only the experimental cache namespace when a cold-cache test is required; do not erase a
   user's global cache without explicit permission.
6. Record `ccache -z`, run the workflow, then record `ccache -s`.
7. Test:
   - empty-cache clean build;
   - immediate rebuild after removing selected objects;
   - rebuild after a source edit and revert;
   - rebuild after switching between debug and release;
   - cache invalidation after a common header changes.
8. Compare cache overhead and hit rate.

Evaluate sccache only if remote/shared caching or its operational model is desired. Do not enable
both simultaneously.

### Acceptance gate

- no dependency correctness regressions;
- no stale-object behavior;
- measurable warm-build benefit;
- negligible cold-cache regression;
- documented opt-in/default policy;
- structural benchmark results remain available with cache disabled.

## 12. Phase 6: Selective precompiled-header experiment

Attempt PCH only after trace aggregation identifies a stable common header prefix.

### Candidate selection

A PCH candidate should be:

- parsed by many TUs;
- expensive cumulatively;
- stable across ordinary edits;
- compatible across all commands in the target group;
- mostly standard-library or stable project configuration headers;
- free of order-dependent macros and per-TU configuration.

Do not place frequently edited eepp UI headers into the first PCH.

Prototype separate PCH scopes where appropriate:

- eepp core/library;
- ecode;
- generated syntax definitions.

### Measure

- clean target build;
- leaf `.cpp` incremental build;
- common-header invalidation rebuild;
- PCH-generation time;
- peak memory at `-j24`, `-j12`, and one lower concurrency if memory pressure appears;
- binary output and runtime parity.

### Acceptance gate

Keep a PCH only when end-to-end benefit remains significant after including PCH generation and its
invalidation cost. Avoid a PCH that improves clean builds but makes the dominant edit/rebuild loop
worse.

## 13. Phase 7: Selective unity-build experiment

Unity builds are optional and lower priority than dependency hygiene and PCH.

Good initial candidates are homogeneous, stable source families with shared includes, especially
generated syntax definitions if Phase 3 shows repeated frontend cost.

Do not begin with a monolithic eepp or ecode unity file.

### Required checks

- static/anonymous namespace symbol collisions;
- macro leakage and include-order dependence;
- warning changes;
- peak compiler memory;
- loss of parallelism;
- incremental rebuild amplification;
- debug experience and source attribution;
- generated-code update behavior.

Test multiple group sizes rather than only on/off. Compare clean time and leaf-edit latency at
realistic parallelism.

### Acceptance gate

Unity mode should be optional unless it improves both the dominant developer workflow and clean
builds without unacceptable memory, diagnostics, or incremental penalties.

## 14. Phase 8: Parallelism and linker tuning

This phase does not change optimization levels and should be done after source improvements.

### Parallelism sweep

Run the chosen clean target with at least:

```text
-j8
-j12
-j16
-j20
-j24
```

Measure wall time and peak RSS. The fastest setting on a 3900X may be below 24 because large Clang
jobs compete for memory bandwidth and cache. Recommend the best default separately for clean and
incremental builds if they differ.

### Linker

Measure mold versus the current linker only for B10 and full target wall time. Linker selection
does not explain compilation hotspots. If mold is already active, simply document the residual
link fraction.

## 15. Validation after every accepted code change

For C/C++ edits:

```bash
git diff --name-only -- '*.c' '*.cpp' '*.h' '*.hpp' | xargs clang-format -i
```

Regenerate the release build using the current approved Premake 5/Ninja command and compile the
narrow affected target. Regenerate debug/tests using the current Premake 4/GNU Make command, then
run focused unit tests followed by the full relevant unit-test suite when the phase is ready.

At minimum verify:

- clean build succeeds;
- incremental dependencies rebuild everything required and nothing obviously unrelated;
- debug and release configurations compile when the change affects shared build logic;
- no warnings were introduced;
- unit tests pass;
- public headers remain self-contained where expected;
- binary behavior and runtime performance remain unchanged;
- allocation audit is complete;
- `git diff --check` passes.

For Premake changes, inspect generated commands rather than assuming the intended flag or launcher
was applied.

## 16. Reporting format

Maintain one result table for accepted and rejected experiments:

| Experiment | Metric | Before median | After median | Delta | Variance | Decision |
|---|---|---:|---:|---:|---:|---|
| example | B2 wall | 100.0 s | 91.0 s | -9.0% | 1.2% | accept |

Each experiment report must include:

- hypothesis;
- exact files changed;
- exact commands;
- machine/environment differences;
- affected trace categories;
- clean and incremental results;
- peak-memory result;
- tests run;
- runtime/allocation considerations;
- decision and rationale.

Track improvements cumulatively, but periodically rerun the original baseline command from the
same branch state to detect benchmark drift.

## 17. Final deliverables

The executing agent should produce:

1. a reproducible benchmark script or documented command set;
2. baseline results for B1 through B11 where applicable, with Premake 4 debug and Premake 5 release
   results clearly separated;
3. aggregated Clang trace reports;
4. a ranked list of project-owned TU, header, and template costs;
5. accepted source/build-system improvements, each independently measured;
6. a list of rejected experiments and why they failed;
7. recommended cache configuration for normal development;
8. recommended Ninja job count for the 3900X;
9. final clean-build and incremental-build comparison;
10. remaining known costs, including HarfBuzz, clearly separated from actionable eepp costs.

## 18. Execution order and stop conditions

Execute in this order:

```text
Phase 0 environment and targets
  -> Phase 1 reproducible baselines
  -> Phase 2 trace aggregation and isolated confirmation
  -> Phase 3 dependency/syntax refactoring
  -> Phase 4 template/container work if justified
  -> Phase 5 compiler cache
  -> Phase 6 selective PCH if justified
  -> Phase 7 selective unity if justified
  -> Phase 8 parallelism/link tuning
  -> final validation and report
```

Stop or request guidance when:

- existing user changes overlap a required file and cannot be preserved safely;
- the active build configuration differs materially from this plan;
- a proposed change adds runtime allocation or indirection to a hot type;
- public API/ABI changes appear necessary;
- a measurement cannot be reproduced within reasonable variance;
- a change improves one workflow but materially harms a more important workflow;
- completing an experiment would require destructive cache or build-directory deletion not
  explicitly authorized.

Do not declare success based on trace reduction alone. Success means repeatably lower wall time in
the named developer workflows, with correct builds and no unacceptable runtime or maintenance
cost.
