# eepp Superluminal GUI/HTML Optimization Plan - 2026-07-09

## Goal

Use the Superluminal capture from `2026-07-09_19-52-19_eepp-unit_tests` to identify cheap, low-risk optimization work in eepp's GUI, text, and HTML compatibility layers. This plan intentionally ignores unit-test-specific hotspots such as `EE::Graphics::Image::diff()` and focuses on code paths likely to matter in real applications and `ecode`.

Session analyzed:

```text
/home/downloads/2026-07-09_19-52-19_eepp-unit_tests.slp
```

Current baseline capture after CSS selector phases 1-8:

```text
/tmp/eepp-unit-tests-current-final-2026-07-10.linux
```

Accessor-inlining comparison capture:

```text
/tmp/eepp-unit-tests-inline-accessors-2026-07-10.linux
```

Both current captures ran all 737 release tests at 500 Hz. The post-change run passed 736 tests
with one skipped test.

MCP could not open the packaged `.slp` directly, so it was extracted to `/tmp/superluminal-eepp-unit-tests` and the contained `.session` was queried.

## Capture Notes

- Capture duration: ~29.7 s.
- Process: `eepp-unit_tests`.
- Hot thread: `Job Scheduler 1`, ~20.69 s executing.
- Dominant ignored costs:
  - `EE::Graphics::Image::diff`: ~1.93 s exclusive / ~10.74 s inclusive.
  - libm `pow`, `cbrt`, `cos`, `atan2`, `sin`: mostly driven by image comparison and rendering tests.
  - repeated application/window teardown costs are visible but not the main target here.

Existing related plan:

```text
.agent/plans/eepp_css_selector_optimization_plan.md
```

CSS selector matching still appears in this capture, but that file already covers the larger selector-indexing and correctness work. Items below only include selector-adjacent tasks when they are smaller or complementary.

## Top Relevant Hotspots

Approximate aggregate timings from the capture:

| Area | Evidence |
| --- | --- |
| Dirty layout invalidation | `UISceneNode::invalidateLayout`: ~120 ms exclusive / ~159 ms inclusive, 1031 calls |
| HTML rich text rebuild | `UIRichText::rebuildRichText`: ~1.24 s inclusive; internal lambda ~37.8 ms exclusive / ~1.19 s inclusive |
| Block layout | `BlockLayouter::updateLayout`: ~1.57 s inclusive, dominated by `UIRichText::rebuildRichText` and `RichText::updateLayout` |
| RichText layout/wrap | `RichText::updateLayout`: ~609 ms inclusive; `RichTextInlineLayouter::layoutNoFloats`: ~507 ms inclusive; `LineWrap::computeLineBreaksInternal<LineWrapInfoEx>`: ~299 ms inclusive |
| White-space inheritance lookup | `getEffectiveWhiteSpaceCollapse`: ~406 ms inclusive, 3235 calls |
| Text width measurement | `Text::updateWidthCache`: ~142 ms inclusive, 916 calls |
| Font glyph/kerning lookup | `FontTrueType::getGlyphByIndex`: ~338 ms inclusive, 2189 calls; `getKerning`: ~72 ms inclusive, 525 calls |
| CSS length parsing | `StyleSheetLength::fromString`: ~44.7 ms exclusive / ~106.9 ms inclusive, 872 calls |
| CSS style selection | `StyleSheet::getElementStyles`: ~154 ms inclusive; `StyleSheetSelector::select`: ~116 ms inclusive; `StyleSheetSelectorRule::matches`: ~55 ms inclusive |
| HTML load/style reload | `UIHTML_KittyHomeSmallDoesNotHang` branch: `loadLayoutFromString` ~199 ms, recursive `reloadStyle` ~101 ms, `updateDirtyLayouts` ~185 ms |

## Ranked Plan

### 1. Reduce `UISceneNode::invalidateLayout()` dirty-list scanning

Files:

```text
src/eepp/ui/uiscenenode.cpp
include/eepp/ui/uiscenenode.hpp
include/eepp/ui/uilayout.hpp
```

Current hot section: `src/eepp/ui/uiscenenode.cpp:1173`.

The expensive part is the second pass over `mDirtyLayouts`, where every new dirty layout may walk ancestors of every already-dirty layout to remove descendants. In this capture, most exclusive time is in `layout->getParent()`, `it->isLayout()`, and climbing ancestors.

Proposed cheap path:

- Add a fast path for the common case where the dirty set is empty or has one element.
- Track a per-layout `dirtyAncestor` or contiguous-layout-root/depth helper so descendant checks can avoid repeated full ancestor walks.
- Consider storing dirty layouts in depth order and stopping descendant checks early when the existing layout is not deeper than the new node.
- Keep current behavior as the correctness fallback until measured.

Validation:

- Existing layout invalidation tests.
- Add a focused test with nested layouts separated by non-layout widgets to preserve the current "only contiguous layout path coalesces" invariant.

### 2. Cache white-space collapse and text-transform during rich-text rebuild

Files:

```text
src/eepp/ui/uirichtext.cpp
include/eepp/ui/uirichtext.hpp
```

Current hot sections:

```text
src/eepp/ui/uirichtext.cpp:1490
src/eepp/ui/uirichtext.cpp:1846
src/eepp/ui/uirichtext.cpp:1969
src/eepp/ui/uirichtext.cpp:2003
```

`getEffectiveWhiteSpaceCollapse()` walks ancestors and calls `isType()` / `hasLocalProperty()` repeatedly. It was called 3235 times and cost ~406 ms inclusive. `getEffectiveTextTransform` performs a similar ancestor walk inside `rebuildRichText()`.

Proposed cheap path:

- Carry inherited `WhiteSpaceCollapse` and `TextTransform` as traversal state in `UIRichText::rebuildRichText()` instead of recomputing from each text node.
- Reuse the computed `collapse` for the inline-span post-processing at `uirichtext.cpp:2003` instead of calling `getEffectiveWhiteSpaceCollapse()` again.
- For correctness, update traversal state only when entering `UIRichText` / `UITextSpan` nodes with local relevant properties.

Validation:

- Existing `UIHTML.*white*`, `UIRichText.*`, and text-transform tests.
- Add a nested span test with inherited `white-space`, local `white-space-collapse`, and nested text-transform override.

### 3. Fast-parse common CSS lengths without allocations

**Status: Implemented and measured**

`StyleSheetLength::fromString()` now trims once, scans scalar numeric prefixes directly from a
`std::string_view`, and converts the numeric subview through `String::fromString()`. The core string
API exposes `std::string_view` numeric overloads while retaining exact `std::string` forwarding
overloads for source and ABI compatibility with the implicitly constructible `EE::String` type.
The CSS parser no longer builds separate number/unit strings or retries parsing by removing
characters. Function expressions retain the existing parser, and position keywords map directly
to percentage lengths without recursive string construction.

Validation includes signed values, leading-dot decimals, scientific notation, surrounding
whitespace, position keywords, unitless and unknown units, `pxAsDp`, all existing CSS function
tests, and a dedicated release benchmark.

- Final focused benchmark median for 320,000 mixed values: 13.67 ms to 10.10 ms, a 26.1%
  reduction.
- Before numeric conversion was centralized in `String`, the full-suite capture measured 105.4 ms
  inclusive / 44.6 ms exclusive to 46.5 ms / 12.4 ms. A future full capture should refresh those
  aggregate numbers for the final implementation.
- Post-change full release suite: 737 passed, one skipped.

Comparison capture:

```text
/tmp/eepp-unit-tests-length-fast-path-2026-07-10.linux
```

Files:

```text
src/eepp/ui/css/stylesheetlength.cpp
include/eepp/ui/css/stylesheetlength.hpp
```

Current hot section: `src/eepp/ui/css/stylesheetlength.cpp:391`.

`StyleSheetLength::fromString()` allocates `num`, allocates `unit` via `substr`, lower-hashes the whole string before knowing whether it is a keyword, and may repeatedly call `String::fromString()` while popping characters.

Proposed cheap path:

- First scan a trimmed `std::string_view`.
- If the first non-sign character is numeric or `.`, parse numeric prefix directly and pass the unit suffix as `std::string_view`.
- Add `unitFromString(std::string_view)` and avoid constructing `unit`.
- Only call `String::hashToLower()` for non-numeric keyword candidates.
- Keep the existing function-parser path for `calc()`, `min()`, `max()`, and `clamp()`.

Validation:

- CSS length parser tests for signed values, decimals, percentages, unitless `0`, `pxAsDp`, keywords, and function strings.

### 4. Avoid duplicate content-offset and size work in `UIRichText::rebuildRichText()`

Files:

```text
src/eepp/ui/uirichtext.cpp
```

Current hot section: `src/eepp/ui/uirichtext.cpp:2038`.

The custom-block branch repeatedly calls `container->getPixelsContentOffset()` and `container->getPixelsSize()` while computing available width, then repeats equivalent work for `customSize`. It also calls `setPixelsSize()` in paths where the computed size may be unchanged.

Proposed cheap path:

- Compute container content offset, current pixel size, inner width, and layout policies once before traversing children.
- In the fill-parent branch, compare against the current child size before calling `setPixelsSize()`.
- Reuse the computed available width for both `setPixelsSize()` and custom block width.

Validation:

- Existing inline-block, block, float, and wrap-content layout tests.
- Add a focused test that verifies no layout change when a fill-parent child already has the computed size.

### 5. Eliminate redundant `RichText::updateLayout()` calls for inline-block baselines

Files:

```text
src/eepp/ui/uirichtext.cpp
src/eepp/ui/blocklayouter.cpp
include/eepp/graphics/richtext.hpp
```

Current hot sections:

```text
src/eepp/ui/uirichtext.cpp:1531
src/eepp/ui/uirichtext.cpp:2144
src/eepp/ui/blocklayouter.cpp:125
src/eepp/ui/blocklayouter.cpp:127
```

`getAtomicInlineBoxBaseline()` calls `rt->updateLayout()` to read lines, and later the inline-block path may call `getRichTextPtr()->updateLayout()` again. `BlockLayouter::updateLayout()` also does a full rebuild/update sequence.

Proposed cheap path:

- Add an `ensureLayoutUpdated()` or dirty-generation check on `RichText` so repeated calls in the same rebuild are no-ops.
- When `getAtomicInlineBoxBaseline()` only needs the last in-flow line baseline, expose a cached baseline from the child widget/layouter after its own layout.
- Avoid calling `updateLayout()` at `uirichtext.cpp:2144` if baseline lookup already forced the same layout generation.

Validation:

- Inline-block baseline tests, nested `details/summary`, and rich-text baseline tests.

### 6. Share glyph advance/kerning work between text width and line wrap

Files:

```text
src/eepp/graphics/text.cpp
src/eepp/graphics/linewrap.cpp
src/eepp/graphics/fonttruetype.cpp
include/eepp/graphics/font.hpp
include/eepp/graphics/fonttruetype.hpp
```

Current hot sections:

```text
src/eepp/graphics/text.cpp:1543
src/eepp/graphics/linewrap.cpp:70
src/eepp/graphics/fonttruetype.cpp:736
src/eepp/graphics/fonttruetype.cpp:947
```

Both `Text::updateWidthCache()` and `LineWrap::computeLineBreaksInternal()` scan strings and repeatedly call `getGlyph()` and `getKerning()`. The line-wrap source annotation shows most time inside glyph advance lookup; width cache shows the same pattern.

Proposed cheap path:

- Introduce a small text-run metrics helper used by both width-cache and line-wrap code.
- Cache space advance once per run.
- For TTF fonts, resolve glyph index once per codepoint and use glyph-index APIs for both glyph lookup and kerning.
- Return or reference just the metrics needed by these loops instead of copying full `Glyph` objects when only `advance`, `lsbDelta`, and `rsbDelta` are needed.

Validation:

- Existing font rendering tests.
- Focused width and wrap tests for tabs, newlines, CR, kerning-enabled proportional fonts, and monospace fast path.

### 7. Add a cheaper kerning path from already-known glyph indices

Files:

```text
src/eepp/graphics/fonttruetype.cpp
include/eepp/graphics/fonttruetype.hpp
```

Current hot section: `src/eepp/graphics/fonttruetype.cpp:947`.

`FontTrueType::getKerning()` computes a cache key from codepoints, then calls `getGlyph()` for both codepoints, then calls `getGlyphIndex()` again. There is already a `getKerningFromGlyphIndex()` entry point nearby.

Proposed cheap path:

- Let the text-run helper call `getKerningFromGlyphIndex()` when it already resolved indices.
- Confirm whether `getKerningFromGlyphIndex()` preserves fallback-font correctness. If not, add an index-plus-font-id key variant.
- Keep the public codepoint path unchanged for external callers.

Validation:

- Kerning tests across normal TTF, fallback glyphs, outline thickness, bold, and italic.

### 8. Reserve and reuse rich-text inline layout buffers

Files:

```text
src/eepp/graphics/richtext.cpp
include/eepp/graphics/richtext.hpp
```

Relevant profiler entries:

```text
RichTextInlineLayouter::rebuildFragments: ~75.5 ms inclusive
RichTextInlineLayouter::appendLayoutRuns: ~25.2 ms inclusive
std::vector<RichText::InlineItem>::emplace_back: ~15 ms inclusive
RichText::clear: ~21.9 ms inclusive
```

Proposed cheap path:

- Preserve vector capacity across `RichText::clear()` for inline items, render paragraphs, spans, and layout runs.
- Reserve based on previous rebuild sizes or current child/text-node count before `UIRichText::rebuildRichText()` starts pushing items.
- Keep memory bounded by shrinking only after unusually large transient layouts.

Validation:

- RichText layout tests and ASAN run.
- Add a stress test that rebuilds a long inline-heavy text repeatedly and verifies stable output.

### 9. Batch style reload side effects during HTML/layout loading

Files:

```text
src/eepp/ui/uiscenenode.cpp
src/eepp/ui/uiwidget.cpp
src/eepp/ui/uistyle.cpp
src/eepp/ui/uihtmlwidget.cpp
```

Capture branch:

```text
UIHTML_KittyHomeSmallDoesNotHang
  UISceneNode::loadLayoutFromString: ~199 ms
  UIWidget::reloadStyle recursion: ~101 ms
  UIStyle::onStateChange / applyStyleSheetProperty visible below it
```

Proposed cheap path:

- During `loadLayoutNodes()`, defer inherited property propagation and layout invalidation until a subtree is attached and initial style application is complete.
- Reuse the existing attributes transaction mechanism where possible.
- Add a scene-level "loading style batch" guard that queues dirty style/layout once per node instead of repeatedly during recursive construction.

Validation:

- Existing layout loading tests.
- HTML fixture tests involving inherited color/font/white-space and immediate layout after load.

### 10. Broaden hot accessor/type-check inlining beyond selector code

**Status: Core hierarchy implemented and measured**

The trivial `getType()` / `isType()` implementations for `Node`, `UINode`, `UIWidget`,
`UILayout`, `UIHTMLWidget`, and `UIRichText` are now inline, along with `Node::isLayout()`.

In the comparable full-suite captures:

- `UISceneNode::invalidateLayout()` decreased from 86.1 ms inclusive / 41.5 ms exclusive to
  26.3 ms total.
- Base type accessor symbols largely disappeared from the hot function list.
- `getEffectiveWhiteSpaceCollapse()` decreased from 344.0 ms to 162.9 ms inclusive, partly
  because its repeated type checks became cheaper.
- Unit-test process CPU time decreased from 20.12 s to 19.88 s, approximately 1.2%. Treat this
  whole-process result as directional because the full suite contains rendering and image-diff
  noise.

Further subclass inlining remains possible, especially table widgets, but the next independent
high-value target is CSS length parsing.

Files:

```text
include/eepp/scene/node.hpp
include/eepp/ui/uiwidget.hpp
include/eepp/ui/uinode.hpp
include/eepp/ui/uilayout.hpp
```

The selector optimization plan already lists some inlining work. This capture shows the same issue in rich-text and layout code:

```text
Node::getParent: ~37.7 ms exclusive
Node::getType: ~36.1 ms exclusive
UIWidget::isType: ~37.6 ms exclusive
UINode::isType: ~26.9 ms exclusive
UIHTMLWidget::isType / getType: ~32.0 ms combined
```

Proposed cheap path:

- Execute the accessor-inline phase from the CSS selector plan, but validate impact on rich-text/layout too.
- Include `getType()`/`isType()` candidates for `UINode`, `UIWidget`, `UILayout`, `UIHTMLWidget`, `UIRichText`, and table element subclasses where definitions are trivial.
- Keep ABI/ODR constraints in mind; remove matching out-of-line definitions where required.

Validation:

- Full build and unit test smoke.
- `git diff --check` and one focused HTML/rich-text filter.

### 11. Keep CSS selector optimization in the existing dedicated plan

Files:

```text
.agent/plans/eepp_css_selector_optimization_plan.md
src/eepp/ui/css/stylesheet.cpp
src/eepp/ui/css/stylesheetselector.cpp
src/eepp/ui/css/stylesheetselectorrule.cpp
```

Capture evidence still supports that plan:

```text
StyleSheet::getElementStyles: ~154 ms inclusive, 1084 calls
StyleSheetSelector::select: ~116 ms inclusive
StyleSheetSelectorRule::matches: ~55 ms inclusive
```

Do not duplicate that work here. When executing this plan, treat selector indexing, sibling combinator correctness, and class/hash matching as owned by `eepp_css_selector_optimization_plan.md`.

Small complementary task:

- If a style reload batching change is implemented first, rerun the same capture or filtered HTML tests before starting selector indexing. Batching may reduce selector call volume and change the measured priority.

### 12. Add a repeatable profiling harness for these focused cases

Files:

```text
projects/scripts/
.agent/plans/
src/tests/unit_tests/
```

The full unit-test capture is useful but heavily distorted by screenshot/image diff work. Add a small documented workflow for targeted captures:

- Full HTML fixture subset.
- RichText-only subset.
- Text/font width/wrap subset.
- CSS parser/style subset.

Suggested filters:

```text
UIHTML.*
UIRichText.*
RichText.*
FontRendering.*Text*
```

The goal is not new product code, but a repeatable before/after measurement path before implementing larger changes.

## Suggested Execution Order

1. Fast local wins: CSS length parsing, white-space collapse reuse, duplicate content-offset/size work.
2. Dirty layout invalidation coalescing.
3. RichText update-layout de-duplication and buffer reuse.
4. Text/glyph/kerning metrics helper.
5. Style reload batching.
6. Existing CSS selector optimization plan.

## Non-Goals For This Pass

- Optimizing `Image::diff()` or image comparison tests.
- Rewriting the whole HTML layout architecture.
- Large selector-index rewrites already described in `eepp_css_selector_optimization_plan.md`.
- Changing browser compatibility semantics for whitespace, inline-block baselines, floats, or flex/grid blockification.
