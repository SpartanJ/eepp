# HTML Layout Architecture

This document describes the eepp GUI HTML compatibility layer: the subset of HTML/CSS layout implemented through `UIHTMLWidget`, `UIRichText`, `RichText`, and the `UILayouter` family.

The goal is browser-compatible behavior where implemented, not a parallel eepp-specific layout language. When adding HTML/CSS features, start from the relevant specification and then map that behavior onto the existing architecture.

## Spec-First Requirement

HTML/CSS compatibility work is **specification implementation**, even when the request begins with a
visual difference in one website fixture. A screenshot identifies a symptom; it does not define the
layout rule. Agents must not patch toward fixture pixels until they can name the generic HTML/CSS
mechanism that produces those pixels.

### Mandatory Decision Process

Before editing layout or rendering code, write down in the working notes or plan:

1. The generic concept involved, such as inline baseline calculation, margin collapsing, intrinsic
   sizing, replaced elements, float exclusion, or border painting.
2. The specification section or browser-defined user-agent behavior that governs it.
3. The engine abstraction that owns the behavior. Prefer parsers, computed style, formatting
   contexts, line layout, or generic painting over element classes and fixture code.
4. A fixture-independent invariant that a focused test can assert.

If those four points cannot be identified, continue investigating before implementing a fix.

### Prohibited Fix Patterns

Do not put website names, fixture IDs/classes, DOM ancestry from one page, or special coordinates
from a screenshot into generic layout/rendering code. Do not change an HTML element default when the
problem belongs to a CSS formatting rule. Do not introduce unexplained pixel offsets, font-size
multipliers, or thresholds chosen only because they improve a golden image.

Element-specific behavior is allowed only when HTML or a user-agent stylesheet actually defines
behavior for that element, such as replaced form controls or `summary { display: list-item; }`. In
that case, document the HTML/UA rule being implemented; the triggering website is still not the
justification.

### Required Evidence Before Completion

- Add a focused regression test that does not depend on the original website and expresses the
  generic invariant. Keep the real fixture or golden test as integration coverage, not as the only
  proof.
- Explain every new layout constant from a specification, font metric, existing engine convention,
  or documented user-agent choice. If it has no principled source, do not add it.
- Search the generic implementation diff for fixture names, selectors, and unexplained constants.
- In the final handoff, state the governing concept, the abstraction where it was fixed, and why the
  solution applies beyond the original fixture.

A visually improved fixture is not sufficient evidence of correctness. If the focused generic test
and the fixture disagree, investigate the model or document an intentional unsupported subset rather
than adding a second compensating adjustment.

Primary references:

- HTML Living Standard: https://html.spec.whatwg.org/
- CSS specifications index: https://www.w3.org/Style/CSS/specs.en.html
- CSS Display: https://www.w3.org/TR/css-display-3/
- CSS Visual Formatting Model, including inline formatting and positioning: https://www.w3.org/TR/CSS22/visuren.html
- CSS line height and inline-block baseline rules: https://www.w3.org/TR/CSS22/visudet.html#line-height
- CSS Lists and Counters: https://www.w3.org/TR/css-lists-3/
- CSS Flexbox Module Level 1: https://www.w3.org/TR/css-flexbox-1/

Required workflow for new HTML/CSS behavior:

- Identify the spec section that defines the behavior.
- Implement the behavior at the correct abstraction level, usually display/layout/formatting/list/positioning, not by tag name.
- Preserve HTML element defaults from the spec, such as `summary { display: list-item; }`.
- Add focused tests that encode the generic behavior and, when useful, a real HTML fixture that triggered the issue.
- If eepp intentionally supports only a subset of a spec, document the limitation close to the implementation or in this file.

## Core Concepts

### UIWebView Document Scenes

Each `UIWebView` owns a nested `UISceneNode` for its loaded HTML document. The document scene is
attached below the web view's clipped scroll container so it renders, receives input, and scrolls as
part of the embedding application tree, but it is not added to `SceneManager` as a top-level scene.

The application scene and document scene are separate style and resource boundaries:

- Application stylesheets must not match web-view document nodes.
- Document stylesheets must not match application widgets or sibling web views.
- URI, referer, relative-resource resolution, navigation interception, cookies, dirty style/layout
  queues, actions, keyframes, and author `@font-face` aliases are document-scoped state.
- Application code that intentionally injects document CSS must use
  `UIWebView::getDocumentSceneNode()->combineStyleSheet(...)`.

Nested document scenes inherit only host services needed to operate inside the embedding UI:

- window and DPI,
- shared `UIEventDispatcher`,
- thread pool,
- color-scheme and contrast preferences,
- default font, default font size, and default theme pointer for native controls.

Do not copy the host stylesheet, URI, referer, navigation callback, cookies, dirty queues, roots,
actions, or icon-theme ownership into a document scene.

The WebView document topology keeps three document metrics distinct:

- **CSS viewport:** the visible web-view viewport, used for media queries, viewport units,
  HTML/body minimum height, fixed positioning, and sticky positioning.
- **Layout viewport / initial containing block:** the viewport-sized root layout reference used for
  normal root/body layout and percentage descendants.
- **Scrollable overflow extent:** the measured document overflow size. This belongs to the
  `UIWebView` document layout scroll target and the nested document scene extent, not to the root
  containing block.

Document scenes are owner-updated by `UIWebView::scheduledUpdate()`. Do not register them with
`SceneManager`, and do not add a second update subscription for the same document scene.

Author `@font-face` family names resolve through the owning `UISceneNode` before the global
`FontManager`. Internally registered font resources use scene-unique names so two documents can
declare the same CSS family without replacing each other. Generic, system, and explicitly shared
application fonts remain global fallbacks.

Async document work must be generation-guarded. A newer navigation invalidates older document
responses and document-scene resource loads, and destroying the web view or document scene invalidates
pending callbacks before they mutate styles, cookies, fonts, or DOM.

Inspector and debugging tools should target either the application scene or a document scene
explicitly. Application-root searches intentionally stop at nested scene boundaries; use
`UIWebView::getDocumentSceneNode()` when inspecting or querying document nodes.

### UIHTMLWidget

`UIHTMLWidget` is the base class for HTML-like elements. It stores parsed CSS layout state such as `display`, `position`, `float`, `clear`, list style, and data attributes. It does not own all layout math directly. Instead, it uses `UILayouterManager` to instantiate the appropriate `UILayouter` for its `CSSDisplay`.

Important responsibilities:

- CSS property application and invalidation.
- Out-of-flow positioning support through containing blocks.
- Exposing a `RichText` object for elements that render mixed inline content.

### Layouters

Layout math is separated from widgets into `UILayouter` implementations:

- `BlockLayouter`: Handles block-like containers, including `display: block`, `inline-block`, `list-item`, and `table-cell`. It delegates inline formatting to `RichText`, then maps the resulting physical spans back to child widgets. Also used for blockified flex item children.
- `FlexLayouter`: Full CSS Flexbox Level 1 implementation for `display: flex` and `display: inline-flex`. Handles all flex container/item properties, the flex layout algorithm (item collection, flex basis resolution, flexible length distribution, main/cross-axis alignment, wrapping, gaps), auto margins, `order`-based paint sorting, `visibility: collapse`, min-width:auto, baseline alignment, and anonymous flex item text wrapping.
- `TableLayouter`: Handles `display: table` and encapsulates HTML table column width distribution, rows, sections, and cell positioning.
- `InlineLayouter`: A no-op layouter for true inline text-span elements. Inline formatting is owned by the nearest rich-text/block formatting context so normal widget layout does not override text flow.
- `NoneLayouter`: Handles `display: none` by skipping layout/rendering participation.

`UILayouterManager::create()` is the dispatch point. Key routing rules:
- `Block`/`InlineBlock`/`ListItem`/`TableCell` → `BlockLayouter`
- `Flex`/`InlineFlex` → `FlexLayouter`
- `Inline` → `InlineLayouter` (for text spans) or `BlockLayouter`
- `Table` → `TableLayouter`
- `None` → `NoneLayouter`

**Blockification per CSS Flexbox §4:** Children of flex containers are automatically blockified — they receive `BlockLayouter` regardless of their own `display` value (unless they are themselves a flex container, which keeps `FlexLayouter`). This is enforced in `UILayouterManager::create()` before the display-based dispatch.

## RichText Integration

`UIRichText` is the primary container for mixed text and widget content.

`UIRichText::rebuildRichText()` recursively walks normal-flow children and builds a `Graphics::RichText` stream:

- Text nodes and inline text spans become `RichText::SpanBlock` entries via `addSpan()`.
- `<br>` contributes a line break.
- Inline-blocks, list items, replaced controls, images, and block-like widgets become `RichText::CustomBlock` entries via `addCustomSize()`.
- Out-of-flow children are skipped here and positioned later.

`RichText::updateLayout()` performs line wrapping and inline formatting. `BlockLayouter::positionRichTextChildren()` then consumes `RichText::RenderSpan`s and assigns pixel positions/sizes back to DOM widgets.

### Inline Paint Ownership

Inline text-span CSS is split across three layers:

- `UITextSpan::applyProperty()` owns CSS-to-style mapping for inline text properties. For `background-color` it sets the span font background color because inline spans are normally painted by the parent rich-text stream, not by `UITextSpan::draw()`.
- `UIRichText::rebuildRichText()` is the DOM-to-rich-text bridge. It converts inline `UITextSpan` widgets into `RichText::InlineItem::Box` entries, carrying margin, padding, border, text decoration, background color, optional background image/layers, and optional rounded `UIBackgroundDrawable`.
- `Graphics::RichText::draw()` owns the actual inline fragment painting. It paints box background color first, then background image/layers, then borders, and finally text/atomic content. When a span has both `background-color` and `background-image`, the color fill must still render behind the image layer. When the span has `border-radius`, use the span `UIBackgroundDrawable` for the color fill so rounded corners are preserved.

Atomic inline widgets (`RichText::RenderSpan::Type::AtomicBox`) do not call `UITextSpan::draw()`. Any visual style that belongs to an atomic inline-level box must therefore be carried through the `UIRichText::rebuildRichText()` metadata and painted by `RichText`, or it will be lost.

### UITextNode

`UITextNode` is a lightweight non-rendering node for raw parsed text (`node_pcdata`). Its text is extracted during `rebuildRichText()` and rendered by the parent `UIRichText`. After wrapping, `BlockLayouter` assigns it position and size for debugging and hit-box accounting.

**Flex item special case:** When a `UITextNode` becomes an anonymous flex item (bare text child of a flex container), it uses a cached `Text* mFlexText` object for multi-line word wrapping. `FlexLayouter::resolveCrossSizes()` configures `mFlexText` with the target font, text, and `setMaxWrapWidth(targetMainSize)`, then measures `getTextHeight()` for cross sizing. `UITextNode::draw()` renders `mFlexText` at the flex-assigned position instead of being a no-op.

### Custom Blocks And Baselines

`RichText::CustomBlock` represents atomic inline-level or block-like widgets inside the rich-text stream. It carries:

- physical size,
- float/clear state,
- a baseline offset,
- virtual line-break state.

The default custom-block baseline is the bottom edge to preserve old behavior for generic drawables and spacers. HTML widgets that expose internal `RichText` should provide a CSS-compatible baseline derived from their last in-flow internal line box:

```cpp
margin.Top + contentOffset.Top + lastLine.y + lastLine.maxAscent
```

This is required for `display: inline-block` and for nested rich-text widgets such as `<details><summary>...</summary></details>`. A taller inline-block caused by inherited `line-height` should keep its real height but align by baseline in the parent inline formatting context.

Do not fix baseline problems by special-casing individual elements, zeroing `line-height`, or changing element display defaults. The correct layer is generic inline formatting and custom-block baseline propagation.

**Flex container baseline:** `FlexLayouter` stores the container's baseline after layout in `mContainerBaseline`. For row-direction flex containers, the baseline comes from the first flex line's maximum baseline offset. For column-direction, it comes from the last flex line. `UIHTMLWidget::getBaseline()` delegates to `FlexLayouter::getBaseline()` when the widget is a flex container, allowing outer flex containers to baseline-align nested flex containers correctly.

## Display And Flow Rules

### Inline Content

True inline content is formatted by the nearest `UIRichText`/`BlockLayouter` context. Inline widgets should not be independently positioned by ordinary widget layout.

### Inline-Block

`display: inline-block` creates an atomic inline-level box in the parent inline formatting context and an internal formatting context for its children. It should:

- participate in the parent line as a custom block,
- preserve its own internal line-height and content sizing,
- expose its internal baseline when it has in-flow line boxes,
- fallback to its bottom edge only when it has no in-flow line boxes.

### List Items And Markers

`display: list-item` is block-like for layout but has marker behavior. Shared marker rendering belongs in `UIHTMLListStyle` and related list-item/summary code, not duplicated per element.

Requirements:

- `list-style-type` must be parsed and rendered according to the supported CSS list values.
- `list-style-type: none` disables marker rendering and marker spacing.
- `<summary>` defaults to `display: list-item` and uses disclosure marker defaults as defined by HTML rendering rules.
- `disclosure-open` and `disclosure-closed` should use eepp's primitive marker drawing facilities, not textual `v` or `>` approximations.

### Flex Layout

`display: flex` and `display: inline-flex` use `FlexLayouter` implementing the full CSS Flexbox Level 1 layout algorithm. Key behaviors:

- **Item collection:** In-flow children are collected, sorted by CSS `order` property. Out-of-flow (`absolute`/`fixed`) and `display: none` children are skipped. Bare `UITextNode` children become anonymous flex items with text-based sizing.
- **Flex basis resolution:** `flex-basis` resolves against the container's inner main size (percentage) or uses the item's explicit main size / content size (`auto`). `flex-basis: content` bypasses the explicit main-size check.
- **Flexible length resolution:** Iterative §9.7 algorithm: distribute free space, clamp by min/max, freeze items that hit constraints, redistribute remaining space to unfrozen items.
- **Main-axis alignment:** `justify-content` with all values (flex-start, flex-end, center, space-between, space-around, space-evenly). Auto margins on main axis absorb free space before justify-content applies (§8.1).
- **Cross-axis alignment:** `align-items`/`align-self` with flex-start, flex-end, center, baseline, stretch. Auto margins on cross axis absorb free space before alignment. Baseline alignment matches text baselines across items on the same flex line.
- **Multi-line:** `flex-wrap: wrap`/`wrap-reverse` with per-line cross sizing and `align-content` distribution (all values including space-evenly). When container main size is indefinite, all items stay on a single line.
- **Gaps:** `row-gap`/`column-gap` with `gap` shorthand. `gap: normal` resolves to 0px in flexbox.
- **`min-width: auto` / `min-height: auto`:** Content-based minimum prevents flex items from collapsing below their min-content size. The `overflow` property suppresses this minimum when set to non-visible.
- **`visibility: collapse`:** Collapsed items are invisible and zero-sized on the main axis, but their cross size still contributes to the flex line cross size (preserving layout stability).
- **`order`-modified paint order:** `UIHTMLWidget::drawChildren()` stable-sorts children by CSS
  `order` when flex items have differing values. Per CSS Flexbox §4.3, painting uses
  order-modified document order; `row-reverse`/`column-reverse` changes layout direction but does
  not reverse paint order.
- **Container baseline:** After layout, the flex container's baseline is available via `FlexLayouter::getBaseline()` for use by outer formatting contexts.

### Out-Of-Flow Positioning

Elements with `position: absolute` or `position: fixed`:

- are ignored by standard layouters and `UIRichText::rebuildRichText()`,
- do not contribute to normal-flow auto size,
- are positioned at the end of the parent's `updateLayout()` using `positionOutOfFlowChildren()`,
- use `getContainingBlock()` for absolute positioning and the `UISceneNode` root for fixed positioning.

Relative positioning should preserve normal-flow space and then offset painting/positioning according to CSS semantics.

Floats are represented in `RichText::CustomBlock` with `CSSFloat`/`CSSClear` metadata and handled by the float-aware RichText path. Float placement is edge-aligned and should not be altered by inline baseline alignment.

### Paint Order And Supported Stacking Contexts

`UIHTMLWidget` owns HTML box paint ordering and matching reverse-order hit testing. Its lazy paint
cache implements the supported CSS 2 Appendix E phases: negative positioned stacking groups,
in-flow content, non-positioned floats, positioned descendants with `z-index: auto` or `0`, and
positive positioned stacking groups. Equal-category items retain CSS tree order; flex and grid
containers use `order`-modified document order as their input tree order.

Positioned descendants and child stacking groups can participate in the nearest ancestor stacking
scope even when they are nested below ordinary blocks or floats. Such promoted nodes remain in their
DOM hierarchy for layout and ownership. Painting replays the skipped ancestors' transforms and exact
border/content/padding clip type, and hit testing applies the same ancestor clips. A real stacking
group is atomic: its descendant z-indices cannot escape and interleave with sibling groups.

The current stacking-context trigger subset consists of positioned boxes with an applicable
non-`auto` `z-index`, fixed and sticky positioned boxes (including `z-index: auto`), and flex/grid
items with non-`auto` `z-index`. Relative and absolute boxes with `z-index: auto` participate in the
positioned-auto phase without becoming atomic. Opacity, transforms, filters, isolation, containment,
and other modern stacking-context triggers are not yet modeled as CSS stacking groups. Inline
fragment background/text sub-phases remain owned by `RichText`; implementing their complete Appendix
E interleaving requires fragment-level paint records rather than widget-level sorting.

## Pixel Math

All layouters must use pixel (`Px`) APIs for layout calculations:

- Use `getPixelsSize()`, `getPixelsPadding()`, `getPixelsContentOffset()`, and `getLayoutPixelsMargin()`.
- Do not mix these with `getSize()` or `getPadding()` in layout math. Those return density-independent pixels (`dp`) and will break HiDPI calculations.

Convert only at clear API boundaries where the surrounding code expects dp.

## Testing Expectations

For HTML/CSS layout work, prefer narrow tests plus one realistic fixture when the bug came from real content:

- Unit-level tests for parser/style/layout primitives.
- RichText tests for inline formatting, wrapping, baselines, custom blocks, floats, and line-height.
- Flex algorithm tests for all flex container/item properties, direction/wrap modes, alignment, gaps, auto margins, baseline, collapsed items, and edge cases (71 unit tests in `uihtml_flex_test.cpp`).
- UIHTML fixture tests for browser-like element interactions such as flexbox layouts, details/summary, tables, forms, lists, images, and positioned descendants.
- Regression assertions should verify layout invariants, not screenshot pixels only.

When possible, compare against browser behavior manually or with a reference capture, then encode the spec behavior in assertions.
