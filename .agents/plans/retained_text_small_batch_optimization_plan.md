# Retained Text Small-Batch Optimization Plan

Status: proposal; no implementation started.

Date: 2026-07-25

## Goal

Determine whether small retained `Text` objects should submit their already-cached geometry to
`GlobalBatchRenderer` instead of issuing independent direct draw calls, and implement that hybrid
path only if controlled benchmarks show a meaningful improvement without regressing large text,
ordinary grayscale rendering, LCD subpixel rendering, color emoji, or memory use.

The initial threshold to investigate is 512 rendered glyph quads, but 512 must not become a fixed
policy until measurements establish the crossover point. The decision should use rendered geometry
size rather than `String::size()` because shaping, whitespace, fallback glyphs, decorations, and
outlines break the one-code-point/one-quad assumption.

In this document, “retained text” means a `Text` object with cached geometry that currently draws
its arrays directly. It is not OpenGL instanced rendering (`glDraw*Instanced`). “Static text” means
the static `Text::draw()` convenience path that emits glyphs into `GlobalBatchRenderer`.

## Current tradeoff

### Static `Text::draw()`

Advantages:

- Consecutive compatible strings can remain in one global batch.
- Many small labels can become one draw submission instead of one submission per label.
- The renderer already flushes when texture, blend state, or LCD coverage mode changes.

Costs:

- Glyph traversal and geometry emission happen on every call.
- Shaped text may repeat layout work unless another layer caches it.
- Vertices, texture coordinates, and colors are copied into the batch every frame.

### Retained `Text::draw()`

Advantages:

- Layout and vertex construction are cached until invalidated.
- Large stable text avoids rebuilding and copying all of its geometry each frame.
- Per-character colors and mixed render-mode ranges are already represented.

Costs:

- It flushes the global batch before drawing.
- Every retained `Text` normally creates at least one independent draw submission.
- LCD ranges require multiple channel passes; mixed mask/LCD content can create additional ranges.
- A UI containing hundreds of short labels can become draw-call bound even though each label has
  very little geometry.

The proposed hybrid keeps retained layout/geometry caching but batches small cached geometry at draw
time. Its cost is a bounded CPU-side copy into the batch in exchange for fewer flushes and draw
calls.

## Questions the benchmark must answer

1. At what rendered-quad count does copying cached geometry into the global batch become slower than
   drawing the retained arrays directly?
2. Does that crossover differ materially between OpenGL 2 (the primary renderer), GL3/core, and
   GLES2?
3. How much does LCD rendering move the crossover because each LCD batch flush performs RGB and
   alpha passes?
4. Do many small labels benefit more than a single string of the same total glyph count?
5. How much time is actually spent in layout/geometry generation by static `Text::draw()` compared
   with submission and driver overhead?
6. Does batching retained text reduce draw calls in realistic ecode UI scenes, or do intervening
   textures, clipping changes, and render modes force immediate flushes and erase the benefit?
7. Is a fixed threshold sufficient, or should the decision also consider whether the cached text can
   join the batch currently being built?

## Candidate solutions

### Option A: Geometry-count threshold

For retained text below a configurable/internal threshold, append cached quads and colors to
`GlobalBatchRenderer`. Draw larger text through the existing direct path.

Conceptually:

```cpp
const size_t quadCount = mVertices.size() / GLi->quadVertex();
if ( quadCount <= retainedTextBatchThreshold )
	submitCachedGeometryToBatch();
else
	drawCachedGeometryDirectly();
```

This is the simplest useful experiment. Candidate thresholds should include 0 (always direct), 32,
64, 128, 256, 512, 1024, and an always-batched mode. The production value should be an internal
constant unless runtime tuning proves necessary.

### Option B: Threshold plus batch compatibility

Batch only when the retained geometry can join a compatible pending batch. Compatibility includes:

- font atlas texture and coordinate type;
- blend mode;
- clipping/scissor state;
- primitive representation;
- LCD versus ordinary mask rendering;
- active shader and any other renderer state that currently forces a flush.

If submitting a `Text` would create an otherwise empty batch that must be flushed immediately,
direct drawing is likely cheaper. This option should follow Option A only if instrumentation shows
that incompatible state transitions frequently defeat the size-only heuristic.

### Option C: Store render-mode runs instead of one mode per quad

Retained LCD text currently needs enough metadata to map every quad to its render mode. A compact
run list can represent the common cases more efficiently:

```text
first quad | quad count | mode
0          | 34         | Subpixel
34         | 1          | Color/Mask
35         | 37         | Subpixel
```

Uniform LCD text needs one run; all-mask text can keep the existing empty-vector fast path. The run
list maps directly to drawing and batch submission. It avoids retaining a mode value per glyph but
adds construction and invalidation complexity. Implement it only if memory measurements show the
current sparse mode vector is material or if a run API substantially simplifies hybrid submission.

### Option D: Batch command references without copying geometry

Teach the renderer to queue references to immutable retained buffers and merge or multi-draw
compatible commands later. This could avoid CPU copies, but it introduces lifetime, invalidation,
ordering, clipping, and backend complexity. It is not the first implementation candidate. Consider
it only if Option A demonstrates that draw-call reduction is valuable but vertex copying prevents a
useful crossover.

## Recommended implementation sequence

### Phase 1: Instrument the existing paths

Add benchmark-only or opt-in counters for:

- `Text` draw calls split by static and retained paths;
- rendered quads and vertices;
- global batch flush count and reason;
- underlying `drawArrays()` calls;
- LCD channel-pass draw calls;
- texture, shader, clipping, blend, and render-mode transitions;
- bytes copied into batch arrays;
- retained geometry rebuild count and time.

Counters must be disabled or compile away in normal production builds unless their measured overhead
is negligible. Do not add atomics or logging to the render loop.

### Phase 2: Build a dedicated text-rendering benchmark

Add a focused benchmark to `src/benchmarks/` and the existing `eepp-benchmarks` target. It should
create one window/context, preload every glyph and atlas page, warm up shader creation, and run all
strategies against identical geometry.

The benchmark must expose three forced policies so results are comparable:

- `direct`: existing retained draw path;
- `batch`: force cached retained geometry through the proposed batch submission path;
- `auto-N`: hybrid path with threshold `N`.

The forcing mechanism should be benchmark-only or a narrow internal testing hook, not a permanent
public `Text` API.

### Phase 3: Implement cached-geometry batch submission

Add a bulk submission API to `BatchRenderer` rather than calling its per-quad API in a loop if the
bulk API can preserve existing array growth and state invariants. It should accept non-owning spans
for the duration of the call and copy once into already-reserved batch storage. It must not allocate
per glyph.

Submit retained render-mode runs in original order:

- mask/decorations through the normal batch mode;
- LCD glyph ranges through `setSubpixelText( true )`;
- color glyphs through their established normal/color behavior;
- outline, shadow, and fill in their existing paint order.

The path must preserve transforms, per-vertex colors, atlas coordinates, blend mode, clipping, and
the current pixel alignment rules. Avoid converting cached geometry into temporary vectors.

### Phase 4: Tune and select the policy

Run the complete benchmark matrix on at least the primary OpenGL 2 renderer. Test another
programmable backend where available. Select a threshold only when it improves the small-label
workloads and does not materially regress large-text workloads.

Start with a simple fixed threshold. Add compatibility-aware logic only if counters demonstrate a
real problem. Prefer the smallest policy that captures most of the measured benefit.

### Phase 5: Validate in ecode

Run ecode with its benchmark mode and representative layouts:

- empty editor with menus, tabs, status bar, and side panels;
- a code document with typical visible line lengths;
- multiple splits and minimap enabled;
- terminal output with frequent updates;
- menus and nested context menus containing many short labels;
- grayscale and subpixel antialiasing;
- color emoji embedded in UI/editor/terminal text.

Compare frame time and counters against the direct-retained baseline, not only maximum FPS.

## Benchmark matrix

### Geometry sizes

Use rendered glyph-quad counts near likely crossover points:

- 4, 8, 16, 32, 64, 128, 256, 512, 1024, and 4096 quads.

Include both one object of size `N` and many objects with the same aggregate glyph count. Examples:

- 1 × 512 glyphs;
- 8 × 64 glyphs;
- 32 × 16 glyphs;
- 128 × 4 glyphs.

This distinguishes vertex-volume cost from per-object draw-call cost.

### Rendering content

Test each relevant coverage/state pattern:

- grayscale/mask-only text;
- LCD subpixel-only text;
- LCD text with occasional color emoji or grayscale fallback glyphs;
- alternating render modes as an intentional worst case;
- outline and shadow;
- underline and strike-through;
- per-character colors;
- one shared atlas page versus strings spanning multiple atlas pages/fonts.

### Update patterns

- Stable retained geometry: draw the same objects for every measured frame.
- Position-only movement: cached geometry remains valid but transforms change.
- Color-only updates.
- One object mutated per frame.
- Every object mutated per frame.
- Static `Text::draw()` baseline to quantify the full geometry-generation cost.

### Scene/state patterns

- All compatible text drawn consecutively.
- Text interleaved with rectangles/icons using the same clipping region.
- Text interleaved across different font textures.
- Frequent clipping/scissor changes, representative of widgets and editor lines.
- Empty batch before every label as a worst case for hybrid submission.
- Existing compatible pending batch before every label as the best case.

## Measurement methodology

- Build and measure an optimized release configuration. Debug/ASan builds remain mandatory for
  correctness but are not performance evidence.
- Disable VSync and frame-rate limits.
- Preload fonts, glyphs, atlas pages, and the lazy LCD shader before timing.
- Run warm-up frames until allocations and shader/driver initialization stabilize.
- Use a fixed number of frames or submissions large enough to exceed timer noise.
- Repeat each case several times and report median plus a tail measure such as p95; retain raw
  samples where practical.
- Randomize or alternate policy order to reduce thermal and clock-frequency bias.
- Keep window size, pixel density, font, font size, atlas contents, and visible output identical.
- Prevent dead-code elimination and ensure submitted work is consumed.
- Separate CPU submission time from completed GPU time. Use an explicit synchronization boundary
  outside the timed inner loop when comparing total frame completion, or GPU timer queries where
  supported. Do not put `glFinish()` after every individual text draw.
- Record hardware, driver, renderer backend, build flags, and commit identifier with results.
- Report milliseconds/frame and draw calls in addition to FPS. Very high FPS compresses meaningful
  differences and can obscure CPU/GPU synchronization effects.

## Metrics and decision criteria

Primary metrics:

- median and p95 CPU frame/submission time;
- median completed frame time or GPU time;
- GL draw calls per frame;
- global batch flushes per frame;
- bytes copied into batch storage per frame;
- retained geometry rebuilds and allocations.

Suggested acceptance criteria:

- At least a repeatable 10% CPU-frame improvement in the many-small-label workload, or a substantial
  draw-call reduction that produces a measurable ecode improvement.
- No more than a 2% regression in representative large stable editor/terminal workloads.
- No measurable regression in the ordinary grayscale path when the hybrid path is disabled or the
  object exceeds the threshold.
- No additional per-frame heap allocations after warm-up.
- Exact visual equivalence in the existing subpixel golden test and new hybrid-specific golden
  cases.

If no threshold meets those criteria, keep the existing direct retained path and remove the
experiment rather than retaining unused complexity.

## Correctness tests

Extend rendering tests before enabling the automatic policy:

- Render identical content through forced direct and forced batched-retained paths and compare the
  resulting images exactly where the backend is deterministic.
- Cover text immediately below, at, and above the selected threshold.
- Cover direct/static and retained text on both light and dark backgrounds.
- Cover first-glyph LCD text to preserve the render-mode bookkeeping regression test.
- Cover mixed LCD plus color emoji/fallback glyphs.
- Cover mask-only text and verify it never enters the LCD compositor.
- Cover outlines, shadows, underline, strike-through, per-character colors, fractional positions,
  clipping, transforms, and transparent framebuffers.
- Verify invalidation after antialiasing changes rebuilds both geometry and texture coordinates.

The existing `FontRendering.subpixelCoverageCompositesPerChannel` golden is the baseline guard and
must remain byte/pixel identical throughout threshold experiments.

## Performance and allocation constraints

- The normal mask-only path must not allocate render-mode metadata merely to support LCD text.
- Bulk batch submission must reserve geometrically and avoid temporary per-draw vectors.
- Do not add `std::function`, callbacks, dynamic dispatch, logging, or per-glyph heap activity to the
  render loop.
- A run-based mode representation should use compact trivially copyable records and merge adjacent
  equal modes during geometry construction.
- Threshold checks must be constant-time and based on already available geometry counts.
- Any compatibility inspection must use cached renderer/batch state, not expensive GL state queries.

## Expected outcome

The likely useful design is a hybrid: small, compatible retained text copies cached geometry into
the global batch, while large retained text continues to draw directly. The exact threshold may be
well below 512 quads and may differ for LCD and mask text. Measurements, not the initial estimate,
will decide whether one threshold, separate mode-specific thresholds, or no hybrid path is the best
production choice.
