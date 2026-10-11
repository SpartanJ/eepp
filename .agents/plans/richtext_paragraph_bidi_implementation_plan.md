# RichText Paragraph Bidi Integration Plan

Status: deferred follow-up project; no runtime implementation authorized by this plan.

Date: 2026-10-11.

## 1. Objective and scope

Replace RichText's item-level bidi placement with paragraph-wide analysis shared by shaping,
wrapping, visual placement, decorations, and interaction. Reuse SheenBidi and HarfBuzz; do not
implement another Unicode bidi algorithm.

The current UI RTL implementation remains usable with its documented RichText limitation. This
project is independent of cached UI direction, logical alignment, logical margins/padding, and
RTL control geometry. Those mechanisms do not need to be redesigned.

The required end state is:

- Mixed-direction text works across ordinary style boundaries in both LTR and RTL paragraphs.
- Bidi context survives soft wrapping; each line uses the original paragraph analysis.
- One logical inline item can produce multiple visual fragments on one line.
- Shaping, placement, caret positions, selection, and source mapping agree on character identity.
- Plain text and unchanged retained text preserve their inexpensive execution paths.
- Performance and memory costs are measured before adopting the new representation broadly.

Initial scope covers existing horizontal RichText features, including wrapping, inline boxes,
atomic inlines, floats, whitespace modes, and selection. Full CSS `unicode-bidi`, HTML `bdi`/`bdo`,
`dir=auto`, vertical writing, editor keyboard-navigation policy, and a new batching renderer are
separate extensions. The new internal design must leave room for explicit bidi scopes without
claiming those features are implemented. Existing Unicode controls in text must not be stripped or
silently converted to ordinary characters.

## 2. Current implementation and failure mechanism

Reinspect these locations when starting; names are more stable than line numbers:

| Component | Current responsibility | Required investigation/change |
| --- | --- | --- |
| `include/eepp/graphics/richtext.hpp` | Inline items, render spans, fragments, public queries | Define paragraph storage and logical/visual order contracts |
| `src/eepp/graphics/richtext.cpp` | `buildLayoutRuns()`, `layoutNoFloats()`, `layoutWithFloats()` | Feed both layout paths from the same paragraph analysis |
| `appendTextRenderSpan()` | Constructs a separate substring `Text` for each render fragment | Replace repeated independent analysis/shaping with shared run references |
| `applyRightToLeftLineOrder()` | Analyzes a finished RTL line, selects one level per span, moves spans | Remove after paragraph-derived visual runs cover both base directions |
| `rebuildInlineFragments()` | Reconstructs inline ancestry and decorations | Support several visual fragments from the same logical source |
| `RichText::findCharacterFromPos()/findCharacterPos()` | Maps between spans and character indices | Consume visual geometry with explicit logical mappings |
| `RichText::getSelectionRects()/getSelectionString()` | Selection painting and logical copying | Avoid assumptions that spatial and logical span order coincide |
| `include/eepp/graphics/textlayout.hpp`, `src/eepp/graphics/textlayout.cpp` | Bidi segmentation, fallback, shaping, wrapping, shared LRU | Extract reusable analysis/run shaping without duplicating the engine |
| `include/eepp/graphics/shapedglyph.hpp` | Glyph identity, font, string index, advances | Audit cluster and run metadata needed for fragment views |
| `src/eepp/graphics/textshaperun.cpp` | Font fallback run iteration | Preserve fallback decisions and shaping context |
| `src/eepp/ui/uirichtext.cpp` | DOM-to-inline stream, whitespace, selection source identity | Preserve source mappings and invalidation generations |
| `src/eepp/ui/blocklayouter.cpp` | `positionRichTextChildren()`, fragment buckets, atomic placement | Remove sequential-source assumptions where visual splitting invalidates them |

The current helper has improved numeric and neutral handling by analyzing a concatenated line
with SheenBidi. It still compresses several character levels into one representative level per
render span. Moreover, each span's `Text` makes its own internal bidi/shaping decisions. The
combined-line analysis is not the analysis consumed by those text objects.

Despite its name, `RichText::RenderParagraph` currently represents a laid-out line in these paths.
Do not treat that name as evidence that paragraph context is already preserved.

A minimal motivating example is:

```html
<p dir="rtl"><span style="color:red">אבABC</span><span>DEF</span></p>
```

The Latin sequence should remain `ABCDEF`. A correct visual decomposition, from physical left
to right, can be `[ABC, red][DEF, normal][Hebrew, red]`. Moving the first styled item as one
rectangle cannot express this. Turn this example into a regression and verify it against the
reference algorithm; it is a motivating case, not a recorded golden image.

The RTL review baseline passed 1,550 release unit tests. That is correctness context, not a
performance baseline or proof of paragraph bidi conformance. Capture a fresh baseline on resumption.

## 3. Standards and implementation references

Read the governing sections before implementing behavior:

- [Unicode UAX #9, resolving levels](https://www.unicode.org/reports/tr9/#Resolving_Embedding_Levels)
  and [line reordering](https://www.unicode.org/reports/tr9/#Reordering_Resolved_Levels).
  Resolve paragraph context before line-specific adjustments; preserve the logical stream while
  deriving each line's visual order. Use the Unicode data version supported by bundled SheenBidi
  when selecting conformance data; do not assume the newest Unicode fixtures match that version.
- [CSS Writing Modes 3, bidi application](https://www.w3.org/TR/css-writing-modes-3/#bidi-algo),
  including atomic inlines and reordering-induced box fragmentation. Ordinary styling is not a
  directional boundary. Block boundaries and bidi-type-B forced breaks define paragraph units.
  Atomic inline objects participate as neutral objects under the currently supported semantics.
  Decoration edges must follow visual fragment rules. Unsupported explicit CSS bidi scopes remain
  documented until implemented.
- [CSS Text 3, processing order](https://www.w3.org/TR/css-text-3/#order),
  [shaping across element boundaries](https://www.w3.org/TR/css-text-3/#boundary-shaping), and
  [line breaking](https://www.w3.org/TR/css-text-3/#line-breaking).
  Reuse existing whitespace/transformation semantics, retain mappings when characters change,
  and distinguish shaping boundaries from paint boundaries.
- [HarfBuzz cluster documentation](https://harfbuzz.github.io/working-with-harfbuzz-clusters.html).
  Glyphs do not map one-to-one to characters. Retain cluster mappings and shaping context, and
  establish safe break behavior rather than slicing arbitrary glyph indices.
- Bundled `src/thirdparty/SheenBidi/Headers/SheenBidi/SBParagraph.h` and `SBLine.h`:
  `SBParagraphCreateLine()` supplies paragraph-derived line runs and applies L1/L2. Audit ownership,
  offsets, error handling, and data lifetimes against the bundled implementation.

Primary invariants are source-independent: changing only color/span partitioning must not change
glyph order or character identity; resizing must preserve paragraph analysis unless text semantics
change; a logical selection must copy the same text regardless of visual fragment order.

## 4. Architecture and ownership

### 4.1 Keep three orders explicit

1. Logical source order: input text/items, selection ranges, copying, and DOM identities.
2. Shaping run order: intervals with compatible direction, script, font/fallback, and shaping style.
3. Visual line order: ordered run/cluster fragments placed on a particular line.

Do not sort the source items to obtain visual order. Do not feed an already reversed character
stream to HarfBuzz. Audit all `getLines()` consumers before changing span storage order. Prefer
logical span records plus an explicit visual traversal/index structure if that preserves existing
contracts; make the final choice in the prototype and document it.

### 4.2 Introduce paragraph-owned analysis

Use an internal graphics-layer abstraction, with candidate roles such as:

- `ParagraphText`: flattened processed UTF-32 text, paragraph boundaries, and source intervals.
- `ParagraphBidi`: resolved base level, character levels, and owned SheenBidi analysis.
- `ParagraphShape`: shared glyph/cluster storage and shaping run intervals.
- `VisualLine`: logical input range, available region, visual fragment sequence, baseline metrics.

These are conceptual names, not a requirement to create four heap allocations or public classes.
Choose compact combined storage and reuse eepp containers where their semantics fit. Keep the
analysis near `TextLayout` so it can serve RichText without making UI/DOM types part of graphics
shaping. Single-string `TextLayout::layout()` should remain a compatible convenience entry point.

Flatten each paragraph once. Map contiguous intervals to source leaves, inline ancestry, and
style IDs rather than storing heavyweight source/style objects per character. Distinguish:

- processed paragraph offsets used by SheenBidi/HarfBuzz;
- public RichText character indices;
- original text/source offsets where whitespace collapse or transformation changes length;
- virtual characters used for atomic objects or synthetic breaks.

Define half-open range conventions and offset units explicitly. Synthetic analysis characters
must never unexpectedly appear in copied text or shift public indices. Source identities are
borrowed and must not outlive the owning inline stream generation.

### 4.3 Share analysis with run shaping

Extract the reusable internals currently behind `segmentString()` and `shapeAndRun()` instead of
calling `TextLayout::layout()` on every directional fragment. The new internal shaping entry point
must consume resolved directional runs and sufficient surrounding context without doing its own
independent paragraph resolution.

Separate font/size/features/script/language decisions from paint-only color, background, and
decoration decisions. Coalesce compatible shaping intervals across paint-only boundaries. Preserve
context across boundaries that require separate shaping calls; retain fallback/font service behavior.

Investigate clusters spanning a paint boundary. Define a stable paint-ownership policy consistent
with the existing per-character color and ligature behavior; do not duplicate a ligature glyph to
paint multiple source fragments. This is an explicit prototype decision, not an incidental result
of slicing arrays. Do not globally change public `ShapedGlyph` ABI to carry RichText-only state if
parallel internal run/cluster metadata can express it.

### 4.4 Separate wrapping from paragraph resolution

Compute break opportunities and advances in logical order using shared shaped data. After choosing
a line's logical range, obtain its visual runs from the original paragraph analysis. Handle line-end
whitespace through the line operation rather than inventing a new paragraph for every wrap.

Preserve existing wrap modes and intrinsic sizing. At a break that needs contextual reshaping,
reshape the affected boundary run with the proper line context, then reconcile widths through a
bounded procedure. Never endlessly alternate shape/wrap passes or silently split a cluster.

Both float and no-float paths must use this common mechanism. Float exclusions determine the
available physical region; they do not introduce direction boundaries. RTL start alignment and
indentation apply within that region. Block/clear events keep their existing layout meaning.

### 4.5 Produce views into retained glyph storage

A visual fragment references a shaped glyph/cluster range, a logical source range, paint metadata,
and placement. A source leaf may contribute several fragments on the same line or across lines.

Avoid one new substring, shared ownership control block, and `Text` object per fragment. Extend
internal text drawing/geometry APIs to consume shaped views if necessary. Reuse glyph positioning,
selection geometry, emoji/color glyph handling, atlas access, and retained rendering instead of
copying those implementations into RichText. Materialize any unavoidable compatibility objects
once during layout and measure their cost.

Keep shaped data alive while fragment views exist. Audit generation changes and retained pointers
in `BlockLayouter` buckets. Direct accessors belong inline; computation belongs in implementation
files. Preserve class conventions and public source compatibility.

### 4.6 Interaction and decorations consume the same fragments

Hit testing must traverse visual geometry, not assume `line.spans` is sorted by x. At bidi boundaries
one logical offset can have multiple visual caret locations; specify primary affinity and preserve
existing APIs through a deterministic default. Add an internal affinity model only where needed.

Selection rectangles can be disconnected visually. Copy selection in logical order from canonical
source mappings, with correct virtual-break and atomic-object policy. Audit the current newline-gap
reconstruction in `getSelectionString()` before fragment splitting introduces more gaps.

Use the fragment model for inline backgrounds, borders, padding, margins, decorations, hit regions,
and DOM event identity. Preserve atomic objects as indivisible outer items while their contents
layout independently. Physical float sides and physical box properties keep their meanings.

## 5. Cache and invalidation contract

Distinguish analysis, shaping, line layout, and paint invalidation. Begin with correct coarse
invalidation where necessary, then narrow it using measured tests; do not build speculative global
caches before paragraph-local reuse works.

| Change | Paragraph analysis | Shaping | Lines/fragments | Paint |
| --- | --- | --- | --- | --- |
| Text or character-affecting whitespace/transformation change | Rebuild affected paragraphs | Rebuild affected runs | Rebuild | Rebuild affected data |
| Base direction or supported bidi scope change | Rebuild | Rebuild affected runs | Rebuild | Rebuild affected data |
| Font, size, features, language, or fallback-resource change | Reuse if character stream is unchanged | Rebuild affected runs | Rebuild | Rebuild affected data |
| Available width or float exclusion geometry | Reuse | Reuse except break-dependent boundary shaping | Rebuild | Reposition/rebuild affected geometry |
| Alignment/indentation/line-height/baseline change | Reuse | Reuse unless metrics require it | Recompute affected line geometry | Update |
| Color/background/selection-only change | Reuse | Reuse | Reuse bidi/order and glyph metrics; update paint fragments if needed | Update |
| Unchanged draw | Reuse | Reuse | Reuse | Submit retained data |

Keep font destruction/selective layout cache eviction safe. Inspect current shared LRU keys before
reusing them for paragraph shaping; direction, context, features, and resource identity cannot be
omitted. Avoid storing borrowed paragraph pointers in a global cache. Document owning-thread rules,
and do not introduce background shaping or async handoffs as part of this project.

## 6. Performance experiment and acceptance criteria

Performance validation is an early deliverable, not post-implementation cleanup. No percentage
improvement or regression is assumed. Compare the current implementation and prototype with the
same release configuration, fonts, backend, viewport, renderer, and hardware.

### Workload matrix

- Short unstyled ASCII labels; single-style LTR and RTL paragraphs.
- Mixed Latin/Hebrew and Latin/Arabic paragraphs with digits and punctuation.
- Same logical string partitioned into one, several, and many paint-only spans.
- Long paragraphs, many short paragraphs, and alternating-direction stress cases.
- Nested inline decorations, font fallback, emoji, and atomic inline objects.
- No-wrap, normal wrapping, preserved whitespace, and break-spaces.
- Float-constrained paragraphs and repeated viewport resizing.
- Cold layout, warm identical layout, edits near the beginning/middle/end, direction toggles,
  paint-only changes, selection changes, and repeated unchanged draws.

Measure separately:

- bidi analysis, shaping, line breaking, fragment construction, geometry generation, draw submission;
- total layout latency and distributions/tails, with repeated samples and absolute times;
- allocation count and bytes, retained capacity, peak memory, and cache hit/miss behavior;
- paragraph-resolution and shaping-call counts;
- fragment count, renderer flushes/draw submissions, and real GPU frame cost.

Prewarm glyph atlases for warm measurements; report cold atlas/font loading separately. Hidden-window
test timings are not a rendering benchmark. Confirm hardware rendering; do not use llvmpipe/Xvfb or
ASan results to claim production performance. Capture exact build options and baseline revision or
working-tree snapshot identity.

Required architectural budgets:

- No new bidi/shaping work during unchanged drawing; no new steady-state allocations attributable
  to fragment traversal after warm-up.
- Paragraph resolution count must not scale with style span count or visual line count on one
  unchanged paragraph. Width-only relayout must reuse that analysis.
- Paint-only repartitioning must not multiply shaping calls when shaping attributes are compatible.
- Additional storage scales with characters/runs/fragments, not their cross-product. Do not create
  quadratic source lookup or per-character scans of all styles/lines.
- A simple-text fast path must prove it is equivalent to the general path; ASCII alone is not a
  sufficient eligibility check if controls, atomics, or special inline semantics are present.

During Stage 0 record numeric latency/memory budgets appropriate to actual consumers. Judge changes
against measurement noise and absolute costs, not a made-up universal percentage. If the prototype
regresses common workloads materially, revise storage/cache/drawing integration before rollout.
Do not sacrifice ordering correctness to meet a benchmark; retain the documented current limitation
until an acceptable implementation is ready.

Perform an explicit allocation audit and `sizeof`/compiler layout verification for new or materially
changed types. Justify each retained buffer and ownership allocation. Select `SmallVector` capacities
from measurements and stack limits; its inline-size parameter has implementation limits. Do not
silently expand every UI node or every glyph to hold metadata only RichText needs.

## 7. Implementation stages and exit criteria

### Stage 0: baseline, contracts, and regression fixtures

- Inventory all public/internal consumers of render spans, fragments, source ranges, and glyph indices.
- Record paragraph-boundary policy for block children, literal separators, preserved newlines,
  virtual `addLineBreak()`, and HTML `<br>`. A soft wrap is never a paragraph boundary; determine
  forced-break treatment from CSS semantics instead of equating every layout break flag with B.
- Record baseline tests and workload measurements; set numeric performance budgets.
- Write reference expectations for mixed-span interleaving, wrap context, and logical selection.
- Verify available font fixtures and bundled Unicode version before choosing conformance cases.

Exit: agreed source/index/order contracts, reproducible measurements, and tests that expose the
missing representation. Do not leave intentionally failing tests in a generally usable build;
keep prototype-only expectations isolated until their implementation lands.

### Stage 1: paragraph stream and shared analysis prototype

- Build processed paragraph streams and compact source/style intervals.
- Own SheenBidi handles with RAII and define valid lifetimes for returned level/run pointers.
- Map explicit LTR/RTL base direction and specify paragraph-wide first-strong behavior for
  `Unspecified`; this intentionally replaces per-item detection in RichText and needs tests.
- Handle in-flow atomic objects and exclude floats/out-of-flow content from the character stream.
- Derive both LTR and RTL line runs through `SBParagraphCreateLine()`.
- Keep existing public output as an adapter while checking internal mappings.

Exit: character-level analysis across style boundaries and wrapping matches the reference; the
prototype resolves each paragraph once and handles empty/all-neutral inputs deterministically.

### Stage 2: reusable shaping runs and performance checkpoint

- Extract shaping with explicit paragraph-derived direction, script/font fallback, and context.
- Preserve clusters, contextual Arabic joining, ligatures, combining marks, and emoji sequences.
- Separate shaping style from paint style, and implement the chosen boundary-paint policy.
- Share glyph storage and produce cheap fragment views; audit cache keys and font lifetimes.
- Compare prototype allocation/latency/fragment/draw-submission costs against Stage 0.

Exit: the motivating mixed-span case works without independent fragment bidi decisions; simple
cases remain compatible; measured costs meet budgets or a concrete redesign is completed.

### Stage 3: common line layout, intrinsic widths, and retained rendering

- Integrate measured cluster advances into both existing wrapping paths.
- Support all current whitespace/wrap modes, tabs, indentation, line metrics, and forced breaks.
- Preserve min/max intrinsic width semantics and atomic placement.
- Derive each line's visual order from paragraph context, including whitespace adjustments.
- Route drawing through shared shaped views while retaining existing render-mode behavior.

Exit: no-float and float paths agree where exclusions are absent; resizing changes line ranges
without rerunning paragraph analysis; unchanged drawing reuses layout/geometry.

### Stage 4: fragments, HTML source mapping, and interaction

- Generate multiple visual fragments per logical inline source and nested inline box.
- Update `BlockLayouter` mapping and generation reuse without changing DOM node order or identity.
- Make caret queries, nearest-position hit testing, selection rectangles, and copying use the
  explicit logical/visual mappings, including bidi-boundary affinity and atomic endpoints.
- Implement visual start/end decoration ownership; preserve baseline and physical box semantics.
- Test dynamic style/display/direction changes and resource invalidation.

Exit: geometry, paint, hit regions, source identity, and copied text agree for fragmented items;
no stale fragment pointers survive rebuilding; existing HTML selection behavior remains intact.

### Stage 5: cleanup, compatibility, and handoff

- Remove `applyRightToLeftLineOrder()` and redundant per-fragment analysis only after replacements
  cover current consumers. Do not leave two conflicting production placement algorithms.
- Retain only justified simple-text or backend-disabled fallbacks with explicit eligibility rules.
- Run full tests, meaningful platform checks, allocation/layout audits, and the workload matrix.
- Update RichText Doxygen, `ui_rtl.md`, HTML compatibility/architecture documentation, and examples.
- Publish benchmark conditions, absolute results, memory tradeoffs, and remaining exclusions.

Exit: all acceptance criteria below are satisfied; unsupported features remain accurately documented.

## 8. Regression and conformance matrix

Use graphics-level tests for mappings/ordering, then HTML integration tests for source identity and
painting. Extend `src/tests/unit_tests/ui_rtl_tests.cpp` and relevant existing RichText, font rendering,
HTML inline, float, and selection suites, or add a dedicated paragraph-bidi test file if clearer.

Required cases:

1. One logical string with several paint-only span partitions produces equivalent glyph order,
   cluster positions, widths, and selection text, subject only to documented paint differences.
2. A mixed Hebrew/Latin source item interleaves correctly with neighboring Latin items; repeat with
   reversed base direction, Arabic, nested boxes, and a single unstyled source representation.
3. Digits split across spans keep order; test European/Arabic digits, separators, punctuation,
   bracket pairs, neutral-only spans, and leading/trailing whitespace.
4. Narrow/wide/narrow relayout preserves paragraph levels and restores geometry. Use a reference
   paragraph plus its selected line ranges as the oracle, not separately resolved line strings.
5. Whitespace, synthetic breaks, true paragraph boundaries, empty lines, and `Unspecified` direction
   preserve public character indices and documented paragraph policy.
6. Glyph clusters survive combining marks, ligatures, surrogate-free UTF-32 supplementary characters,
   ZWJ emoji, font fallback, and contextual shaping across compatible style boundaries.
7. Every valid caret position maps back through hit testing according to the chosen affinity policy;
   test physical edges, gaps, bidi boundaries, line ends, and atomic-object halves.
8. Logical selections may yield disjoint rectangles but copy source order exactly, with no duplicate
   text, injected controls, accidental newlines, or omitted characters.
9. Inline backgrounds/borders/decorations fragment correctly across visual reordering and wrapping,
   including nested inline boxes and source hit regions. Add focused render tests where numeric
   geometry assertions cannot establish correctness.
10. Physical left/right floats, clear, atomic/replaced elements, baselines, padding, and text-indent
    retain their geometry semantics in both directions.
11. Runtime changes invalidate the intended cache layers. Verify resolution/shaping call counts for
    unchanged draw, width-only relayout, paint-only changes, font eviction, and direction toggles.
12. Compile with and without `EE_TEXT_SHAPER_ENABLED`; exercise runtime shaping toggles where
    supported. Document the backend-disabled approximation instead of claiming full UBA correctness.

Use bundled-font fixtures and portable resource helpers. Avoid platform/system-font dependencies.
For Unicode conformance, select version-matched `BidiTest.txt`/`BidiCharacterTest.txt` cases; adapt
expected levels/order to line ranges and CSS atomic semantics rather than comparing screenshots.
Reference SheenBidi runs help validate integration but do not independently validate SheenBidi
itself. Keep unrelated dependency upgrades outside this project.

## 9. Build and verification workflow

Re-read `.agents/SOUL.md`, `.agents/rules/build-project.md`, `.agents/rules/unit-tests.md`,
`.agents/rules/html-layout-architecture.md`, and `.ecode/project_build.json` on resumption.
The local project configuration is authoritative if commands change.

Current Linux release correctness workflow:

```sh
premake5 --disable-static-build --with-debug-symbols --with-backend=SDL3 ninja
ninja -C make/linux -j$(nproc) ../../bin/unit_tests/eepp-unit_tests
bin/unit_tests/eepp-unit_tests
```

Format all modified C/C++ before building and regenerate after changes. Run tests directly against
the host display first; apply the documented elevation/Xvfb fallback only when required. Use the
configured full release target for performance workloads. Debug/ASan builds are additional
correctness diagnostics, not a source of performance comparisons. Follow the macOS scripts and
Windows cross-build/Wine workflow from the rules and report platforms that remain unverified.

## 10. Risks and decisions to resolve early

- Public span order: source copying and spatial hit testing currently have different assumptions;
  an indiscriminate vector sort can fix one and break the other.
- API/storage integration: decide whether internal `Text` shaped views or a graphics-level shared
  renderer can eliminate per-fragment objects without changing external contracts.
- Context-sensitive shaping: compatible paint changes must not break Arabic joining or force
  repeated shaping; metric-changing styles still require appropriate contextual handling.
- Cluster-boundary painting and caret affinity: establish explicit policies and regression coverage.
- Paragraph versus forced line boundaries: source separators and layout flags need separate meaning.
- Borrowed source/font pointers and retained fragment buckets: ownership and generation rules must
  remain valid after text rebuild, cache eviction, font destruction, and DOM mutation.
- Backend-disabled support: determine the conservative fallback and keep its limitations explicit.
- Performance: extra fragments may increase retained draw submissions even when glyph count is
  unchanged. Measure this without quietly including the separate batching optimization project.

Related work:

- `html_dynamic_non_atomic_inline_richtext_plan.md`: share the generic inline/source fragment
  model where useful; do not require the entire dynamic-display project before the bidi prototype.
- `retained_text_small_batch_optimization_plan.md`: relevant if fragmentation exposes draw-call
  costs, but independently measured and independently deliverable.
- `docs/articles/ui_rtl.md`: current documented limitation; keep it until verified implementation
  and compatibility coverage justify narrowing or removing it.

## 11. Completion criteria and effort guidance

This project is complete when paragraph context is shared by shaping and line ordering, style
partitioning does not accidentally change bidi behavior, both base directions work, existing
RichText features and interaction consume correct visual fragments, and agreed performance/memory
budgets pass with reproducible evidence. No website-specific selectors or coordinate adjustments
belong in the generic implementation.

Initial planning estimates from the review: 2–4 working days for a focused prototype and roughly
2–3 weeks total for a production implementation with wrapping, fallback, caching, and regression
coverage. Complex decorations, float integration, affinity, or API compatibility can extend that
estimate. Re-estimate after Stages 0–2; these are effort estimates, not delivery commitments or
measured performance predictions.

First action on resumption: capture the current baseline, implement the mixed-span regression and
paragraph-analysis prototype, and measure its allocations and layout cost before broad integration.
