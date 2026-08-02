# HTML Replaced Elements and CSS Auto-Margin Plan

Status: proposed follow-up after the Asahi Linux async inline-image regression fix, 2026-08-02.

## Goal

Replace formatting-role inference with an explicit CSS used-value model for margins and make
`UIHTMLImage` a real `UIHTMLWidget` replaced element rather than a native `UIImage` with an HTML
flag.

The end state should provide:

- CSS `display`, positioning, float, intrinsic sizing, and used-margin behavior from
  `UIHTMLWidget` for `<img>`;
- reusable image loading, drawable ownership, intrinsic dimensions, aspect ratio, and painting
  shared with native `UIImage` through composition;
- formatting-context-specific auto-margin resolution based on the generated CSS box, not widget
  type or `SizePolicy` heuristics;
- deterministic behavior across synchronous and asynchronous stylesheet/image loading;
- no image-specific exceptions in `BlockLayouter`, `UIRichText`, or `FlexLayouter`.

## Current State and Problem

`UIHTMLImage` currently derives from `UIImage`. It adds HTML identity, `alt`, and fallback text,
but it does not inherit the CSS layout state owned by `UIHTMLWidget`:

- `CSSDisplay` and blockification;
- `CSSPosition`, float, and clear;
- box sizing and CSS used-size helpers;
- layouter selection and formatting-context participation;
- normal-flow and out-of-flow classification;
- baseline behavior shared by other HTML boxes.

As a result, HTML layout infers an image's formatting role from combinations of:

- `UI_HTML_ELEMENT`;
- `SizePolicy`;
- concrete widget type;
- parent layout context.

That inference caused the Asahi regression. An inline `<img>` with `margin-left:auto` and
`margin-right:auto` was passed to the generic native block auto-margin calculator. During async
SVG loading, stale parent width was converted into large image margins, the inline anchor expanded
to include them, and repeated layout fed the expanded width back into the next pass.

The immediate fix centralizes formatting-context used margins, but still classifies some boxes
indirectly. This plan replaces that compatibility layer with explicit CSS box semantics.

## Standards Baseline

Implement against CSS used values, keeping these cases distinct:

1. Inline non-replaced and inline replaced boxes: horizontal `auto` margins use `0`.
2. Normal-flow inline-block boxes, replaced or non-replaced: horizontal `auto` margins use `0`.
3. Floats, replaced or non-replaced: horizontal `auto` margins use `0`.
4. Normal-flow block boxes: solve horizontal margins and width using the block constraint
   equation; two auto margins center a definite-width box.
5. Absolutely/fixed positioned boxes: solve margins together with `left`, `right`, and `width`;
   auto margins are not unconditionally zero.
6. Flex and grid items: use their module-specific auto-margin algorithms, not block formatting
   rules.
7. Vertical auto margins: preserve the rules of the applicable formatting model. Do not fold them
   into a horizontal-only shortcut.

Primary references:

- CSS 2.2 section 10.3, Calculating widths and margins.
- CSS 2.2 sections 10.3.1 through 10.3.10 for normal flow, replaced boxes, floats,
  inline-blocks, and positioned boxes.
- CSS 2.2 section 10.6 for heights and vertical margins.
- CSS Display Level 3 for inner/outer display roles and blockification.
- CSS Flexbox section 8.1 and CSS Grid alignment rules for auto margins on flex/grid items.
- HTML rendering rules and replaced-element behavior for `img`.

## Design Direction

### Separate CSS box semantics from image mechanics

Change the inheritance direction to:

```text
UIWidget
├── UIImage                  native eepp image widget
└── UIHTMLWidget
    └── UIHTMLImage          HTML replaced element
```

Do not copy `UIImage` wholesale into `UIHTMLImage`. Extract reusable image mechanics into a
non-widget component or small set of helpers, tentatively named `UIImageContent`:

```text
UIImageContent
├── DrawablePtr and resource-change connection
├── local/remote/deferred source loading
├── intrinsic pixel size and aspect ratio
├── scale mode, tint, destination-size calculation
├── sprite scheduling support
└── drawable painting
```

Both `UIImage` and `UIHTMLImage` own an `UIImageContent` instance and provide narrow host callbacks
for scene/resource access, size changes, invalidation, and main-thread delivery.

The component must not own layout policy, CSS display, margins, padding, borders, position, or
parent geometry. Those remain widget responsibilities.

### Represent formatting role explicitly

Introduce a small formatting-role/used-value input derived from computed CSS state after
blockification, for example:

```cpp
enum class CSSFormattingRole {
    Inline,
    InlineBlock,
    NormalFlowBlock,
    Float,
    Absolute,
    Fixed,
    FlexItem,
    GridItem,
    Table
};
```

The exact enum is an implementation decision; avoid exposing redundant public API if existing
computed display/position state can produce the same result cheaply. The important invariant is
that used-margin resolution receives an explicit role and never guesses from `SizePolicy` or
concrete widget class.

Keep role derivation centralized near `UILayouterManager` / `UIHTMLWidget`, including CSS
blockification. Do not derive it separately in RichText, block, flex, grid, and positioned layout.

### Separate computed, resolved, and used margins

Preserve three concepts:

- specified/computed margin value, including the `auto` bit;
- resolved length for non-auto values, including percentages;
- used margin for the current formatting context and containing block.

Do not mutate the stored resolved margin merely to obtain a used value for one layout pass. Return
a stack-local used-margin structure instead. This prevents stale block auto margins from being
reused when display, position, float, or parent formatting context changes asynchronously.

Suggested API shape:

```cpp
struct CSSUsedMargins {
    Rectf value;
    Uint8 autoSides;
};

CSSUsedMargins resolveUsedMargins(
    const UIHTMLWidget& box,
    CSSFormattingRole role,
    const CSSContainingBlockMetrics& containingBlock );
```

The final API can be smaller, but it must not call the generic native `UIWidget::calculateAutoMargin()`
for HTML formatting.

## Implementation Stages

### Stage 1 - Lock Down Current Behavior

- Keep the real Asahi WebView fixture test with a thread pool and weighted native host layout.
- Assert the logo image is 130px, its inline anchor matches that width, and document overflow stays
  within a reasonable viewport-derived bound.
- Add focused tests for:
  - inline `<img style="margin: auto">`;
  - inline-block non-replaced element with horizontal auto margins;
  - fixed-width normal-flow block with horizontal auto margins;
  - block-level replaced image with horizontal auto margins;
  - floated image with auto margins;
  - absolutely positioned replaced and non-replaced boxes for the relevant inset combinations;
  - flex and grid items with auto margins;
  - display changes before and after deferred CSS/image completion.
- Where practical, compare numeric invariants against a browser reference.

### Stage 2 - Introduce Formatting-Role-Aware Used Margins

- Add one centralized role derivation function based on computed display, position, float, and
  parent flex/grid state.
- Introduce a non-mutating used-margin resolver.
- Route `BlockLayouter`, `UIRichText`, positioned layout, flex, and grid through the appropriate
  role-specific solver.
- Keep flex/grid distribution in their existing layouters; the shared resolver should identify and
  defer those roles rather than reimplementing their algorithms.
- Remove `UIHTMLWidget::getFormattingContextLayoutPixelsMargin()` once all callers use the explicit
  role API.
- Audit `UIWidget::calculateAutoMargin()` callers and retain it only for native eepp layout.

### Stage 3 - Extract Reusable Image Content

- Inventory every `UIImage` responsibility and split it into:
  - image resource/content behavior suitable for reuse;
  - native widget layout and alignment behavior that remains in `UIImage`.
- Create `UIImageContent` or equivalent without adding a second scene/widget hierarchy.
- Move drawable loading, async lifetime guarding, resource-change subscription, destination-size
  calculation, tint/scale settings, and drawing helpers behind the component.
- Preserve the current shared `DrawablePtr` ownership and scene-scoped resource resolution.
- Keep async callbacks generation/lifetime guarded and main-thread UI mutation explicit.
- Add unit tests for the component through both native and HTML hosts; avoid exposing internals
  solely for testing.

### Stage 4 - Rebase `UIHTMLImage` on `UIHTMLWidget`

- Change `UIHTMLImage` to inherit from `UIHTMLWidget` and own shared image content.
- Implement it as a replaced element with:
  - intrinsic width, height, and ratio from the decoded drawable/SVG;
  - CSS width/height/min/max/box-sizing integration;
  - default inline outer display and atomic inline participation;
  - block, inline-block, float, flex/grid item, and positioned behavior from computed CSS;
  - baseline fallback at the replaced element's bottom margin edge as required by the supported
    inline formatting model;
  - `alt` fallback sizing, painting, and accessibility/hit-box behavior;
  - async intrinsic-size invalidation that dirties the correct ancestors exactly once.
- Ensure CSS `display` changes exchange layouters correctly and do not retain stale geometry.
- Keep the public `UIHTMLImage` API source-compatible where reasonable (`getDrawable`, `setAlt`,
  `getAlt`, source properties), but do not preserve inheritance-based `UIImage*` conversion.

### Stage 5 - Migrate Callers and Remove Compatibility Inference

- Update callers that assume `UIHTMLImage` is a `UIImage`.
- Replace casts and shared behavior with explicit `UIHTMLImage` or image-content APIs.
- Audit type queries, inspector property reporting, serialization, widget creation, and tests.
- Remove `UI_HTML_ELEMENT` branches in `UIImage` that only exist to emulate HTML pixel/intrinsic
  behavior.
- Remove image-specific checks and `SizePolicy` heuristics from RichText and layouters.
- Validate that `UISvg` remains a native image widget unless/until inline `<svg>` receives its own
  explicit HTML/SVG replaced-element wrapper; do not accidentally fold that larger migration into
  this work.

### Stage 6 - Conformance and Performance Validation

- Run focused HTML layout suites after each stage, then the full unit-test suite.
- Exercise synchronous and asynchronous image loading, repeated navigation, viewport resize,
  device-pixel-ratio changes, and deferred stylesheets.
- Add a stress test that repeatedly loads/destroys image-heavy WebViews under ASan.
- Verify no layout oscillation: count dirty-layout iterations and assert convergence for the Asahi
  fixture.
- Compare release-build layout/rebuild counters before and after the refactor.
- Audit every introduced allocation, string copy, shared ownership handoff, callback capture, and
  per-frame branch. Image loading may allocate; steady-state layout and drawing must not add new
  heap work.

## Compatibility and Migration Risks

- `UIHTMLImage*` will no longer convert to `UIImage*`. Search the complete repository and document
  any external API compatibility impact before landing the inheritance change.
- Native `UIImage` alignment semantics are not CSS `object-position` or inline formatting
  semantics. Keep those concepts separate during extraction.
- Replaced sizing must distinguish CSS pixels from physical raster pixels, especially for SVG and
  HiDPI rendering.
- Async drawable completion must not apply geometry from an obsolete navigation or destroyed
  document scene.
- `alt` text is not merely native image fallback drawing; its intrinsic sizing and line
  participation need an explicit supported behavior and tests.
- Absolute-position auto margins require inset-aware equation solving. Treating all out-of-flow
  auto margins as zero would be another compatibility shortcut.
- Tables, flex items, and grid items have formatting-model-specific used-size and auto-margin rules;
  do not force them through the normal block equation.

## Exit Criteria

This follow-up is complete when:

- `UIHTMLImage` derives from `UIHTMLWidget`, not `UIImage`;
- native and HTML images share image mechanics without duplicated resource/drawing code;
- HTML used margins are resolved from explicit formatting roles without mutating stored margins;
- inline, block, float, positioned, flex, and grid auto-margin tests match supported CSS behavior;
- the Asahi async fixture converges with a 130px logo/anchor and bounded document overflow;
- deferred style/image completion and viewport resizing cannot feed stale margins or positions back
  into intrinsic sizing;
- no `UI_HTML_ELEMENT` or `SizePolicy` heuristic remains as the source of an image's CSS display
  role;
- focused and full test suites pass under the required wrapper, and the allocation/performance
  audit finds no new steady-state heap work.

