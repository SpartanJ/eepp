# Dynamic Non-Atomic Inline `UIRichText` Plan

Status: proposed follow-up to the HTML replaced-element and auto-margin refactor, 2026-08-03.

## Goal

Support HTML elements represented by `UIRichText` when their computed `display` changes to
`inline`, without treating them as atomic inline widgets and without relying on `UITextSpan` as the
only representation of a non-atomic inline box.

The end state should provide:

- correct non-atomic inline participation for any eligible rich-text HTML box whose computed outer
  display is `inline`;
- runtime block-to-inline and inline-to-block transitions without replacing the DOM widget;
- one type-safe rich-text bridge for inline fragment generation, independent of the concrete
  `UITextSpan` class;
- preserved text styling, whitespace processing, backgrounds, borders, hit boxes, and event source
  identity across display changes;
- continued atomic handling for replaced elements, inline-block, inline-flex, inline-grid, floats,
  and out-of-flow boxes.

This work must remain separate from replaced-image mechanics. `UIHTMLImage` is an inline-level
replaced element and must continue to enter the atomic-box path.

## Governing CSS Concept

The generic concept is the distinction between an element's outer display role and whether its
principal box is atomic or non-atomic in an inline formatting context.

Relevant standards:

- CSS Display Level 3, inner and outer display types and box generation;
- CSS 2.2 section 9.2.2, inline-level elements and inline boxes;
- CSS 2.2 section 9.4.2, inline formatting contexts;
- CSS 2.2 section 8.6, inline element boxes and fragmented padding, borders, and margins;
- CSS Text Level 3, white-space processing across inline element boundaries;
- CSS 2.2 sections 9.7 and 10.3.9, blockification and atomic inline-level boxes.

Fixture-independent invariant: changing a normal rich-text HTML element from `display:block` to
`display:inline` makes its text and inline descendants participate in the containing line stream,
while preserving its own inline fragment decorations and DOM identity. It must not become a single
`RichText::CustomBlock`.

## Current State and Problem

`UIRichText::rebuildRichText()` currently recognizes a non-atomic inline container only when the
widget is a `UITextSpan`:

```cpp
widget->isType( UI_TYPE_TEXTSPAN ) && widget->asType<UITextSpan>()->isInline()
```

That branch then calls `UITextSpan`-specific APIs for:

- text and font-style access;
- layout character counts;
- inline background, border, decoration, and baseline metadata;
- whitespace and text-transform processing;
- child traversal and inline-box push/pop;
- inline-block follow-up behavior.

The type restriction is currently necessary. A broader `UI_TYPE_HTML_WIDGET` test would admit
inline replaced elements such as `UIHTMLImage` and then perform an invalid `UITextSpan` cast.

However, an element instantiated as `UIRichText` (for example a `div` or paragraph) remains a
`UIRichText` when CSS changes its display. `UIHTMLWidget::setDisplay()` exchanges its layouter and
size policy but not its concrete widget type. When such a box becomes `display:inline`,
`UILayouterManager` currently gives it a `BlockLayouter`, and the parent rich-text rebuild treats it
as an atomic custom block. This does not model a normal non-atomic inline box.

## Design Direction

### Classify inline participation explicitly

Introduce a centralized inline participation classification derived from computed CSS state and
box capabilities, for example:

```cpp
enum class CSSInlineParticipation {
    None,
    NonAtomicContainer,
    AtomicBox
};
```

The exact API is an implementation decision. It should live near `UIHTMLWidget` / formatting-role
classification and consider:

- computed outer display;
- float and out-of-flow state;
- flex/grid blockification;
- replaced-element identity;
- whether the widget can expose rich-text inline-container content.

Do not infer non-atomic behavior from `SizePolicy`, tag name, or `UI_TYPE_TEXTSPAN` alone.

### Extract a type-safe inline-content interface

Move the data needed by `UIRichText::rebuildRichText()` behind a narrow interface or virtual API
implemented by rich-text-backed HTML widgets. Possible shapes include an
`UIInlineContentProvider` interface or protected/public virtual methods on `UIRichText`.

The interface should expose only semantics required by the parent inline formatting context:

- font style and optional owned text run;
- layout character-count reset/update when applicable;
- inline fragment background, border, padding, margin, decoration, and baseline alignment;
- effective white-space and text-transform inputs;
- logical children for recursive stream construction;
- source widget identity for painting, hit testing, and events.

Prefer moving generally valid style accessors from `UITextSpan` into `UIRichText` over duplicating
state or conditionally casting. Keep span-only hit-box conveniences in `UITextSpan` if they are not
required by all inline containers.

### Keep atomic and non-atomic paths separate

The parent rich-text stream must continue to distinguish:

- non-atomic `display:inline` rich-text containers: push an inline fragment box, recursively append
  text/children, then pop the box;
- atomic inline-level boxes: append one `RichText::CustomBlock` / atomic box;
- block-level boxes: append the supported block representation and line breaks;
- floats and positioned boxes: retain their formatting-context-specific paths.

`inline-block`, `inline-flex`, and `inline-grid` are atomic even when backed by `UIRichText`.
Replaced elements such as `UIHTMLImage` and form controls remain atomic at `display:inline`.

### Preserve DOM objects during display mutation

Do not replace a `UIRichText` instance with a `UITextSpan` when `display` changes. Replacing the
widget would risk losing references, listeners, focus, animation state, inspector identity, child
ownership, and author-script-visible state. The same widget should change formatting
participation in place.

## Implementation Stages

### Stage 1 - Lock Down the Unsupported Behavior

Add focused tests before generalizing the implementation:

- a block-created `div` styled `display:inline` between text siblings shares their line;
- nested inline children remain in document order and wrap as one inline stream;
- runtime `block -> inline -> block` mutation updates layout and paint ownership;
- inline padding, border, background color, and background image fragment across wrapped lines;
- whitespace collapse and preservation across the newly inline container's boundaries;
- an inline `UIHTMLImage` in the same content remains an atomic box;
- `inline-block`, floated, absolute, flex-item, and grid-item rich-text boxes remain atomic or
  blockified as required;
- hit testing/event source identity continues to reference the original DOM widget.

Use numeric layout invariants and render-span types where available. Avoid website-specific
fixtures as the only proof.

### Stage 2 - Introduce Inline Participation Classification

- Add the centralized inline participation query.
- Define replaced/atomic capability explicitly rather than through concrete image type checks.
- Incorporate parent flex/grid blockification and float/out-of-flow state before ordinary inline
  display classification.
- Use the query in `UIRichText::rebuildRichText()` and
  `BlockLayouter::positionRichTextChildren()` so stream generation and fragment-to-widget mapping
  cannot disagree.
- Document which unsupported CSS display combinations intentionally fall back to atomic behavior.

### Stage 3 - Generalize Rich-Text Inline Container Data

- Inventory every `UITextSpan` method/state read by the current non-atomic inline branch.
- Move generally applicable font/style/text-fragment access to `UIRichText`, or expose it through a
  narrow inline-content provider.
- Split optional owned text from descendant text nodes. A `UIRichText` with no direct text must
  still generate its fragment box and recursively contribute its children.
- Keep layout character-count bookkeeping conditional where it is meaningful; do not add dummy
  span state to all rich-text boxes solely to satisfy the old implementation.
- Replace repeated casts in the branch with one validated provider/reference.
- Generalize inline background/border helper signatures from `UITextSpan*` to the narrowest valid
  rich-text/HTML type.

### Stage 4 - Route Dynamic `UIRichText` Through Inline Formatting

- Update `UIRichText::rebuildRichText()` to push/pop inline boxes for every classified non-atomic
  inline container.
- Ensure recursive traversal skips out-of-flow descendants exactly as before.
- Preserve whitespace state across element boundaries and nested inline boxes.
- Update `UILayouterManager::create()` so non-atomic inline rich-text containers receive the
  no-op/inline-owned layout behavior instead of an independent `BlockLayouter`.
- Ensure `UIHTMLWidget::onDisplayChange()` invalidates both the old and new formatting owners and
  clears stale block geometry, fragment metadata, and hit boxes.
- Update `BlockLayouter::positionRichTextChildren()` to map all non-atomic inline fragment sources
  back to widgets without `UITextSpan`-only assumptions.

### Stage 5 - Resolve Paint and Hit-Test Ownership

- Generalize the rule that non-atomic inline containers are painted by the ancestor `RichText`
  stream, not by their own `UIRichText::draw()` call.
- Prevent duplicate text, background, and border painting during and after display mutation.
- Rebuild per-fragment hit boxes for generalized inline containers, or explicitly separate widget
  aggregate bounds from span-specific text hit boxes.
- Verify hover, click, selection/source lookup, inspector bounds, and anchor behavior across wrapped
  fragments.
- Clear inline fragment state when a box becomes block, atomic, floated, or out-of-flow.

### Stage 6 - Audit Special Formatting Contexts

- Verify flex/grid children are blockified before non-atomic inline classification.
- Verify `display:inline-flex` and `display:inline-grid` stay atomic in the parent line while owning
  their internal formatting contexts.
- Verify table-internal roles are not accidentally flattened into text.
- Verify list-item markers remain associated with the correct principal box after display changes.
- Verify inline formatting inside table cells, details/summary, anchors, labels, and form wrappers.
- Confirm `UIHTMLImage`, native `UIImage`, and `UISvg` behavior is unchanged.

### Stage 7 - Validation and Performance Audit

- Run focused rich-text, inline-block, flex, grid, table, details, image, and WebView tests.
- Run the complete unit-test suite through `projects/scripts/xvfb-run-eepp`.
- Exercise repeated runtime display toggles and stylesheet replacement under ASan.
- Assert layout convergence after each mutation; no stale block size may feed the next inline pass.
- Audit new abstractions for allocations. Classification and steady-state rebuild traversal should
  add no heap allocation beyond fragment data already required by `RichText`.
- Search the final layout diff for tag names, fixture selectors, unexplained constants, concrete
  image checks, and unsafe downcasts.

## Compatibility and Migration Risks

- `UIRichText::draw()` currently assumes most non-`UITextSpan` instances own their paint. Changing
  that ownership without synchronized parent-stream metadata can cause duplicate or missing text.
- `UITextSpan` carries direct text and font-style APIs not currently exposed uniformly by
  `UIRichText`; moving them can affect serialization and native callers.
- Inline boxes fragment across lines, so a single widget rectangle is insufficient for precise
  backgrounds, borders, and hit testing.
- Display changes can occur after deferred stylesheets load. Old layouters and fragment metadata
  must not survive the transition.
- Flex/grid blockification takes precedence over the specified inline outer display.
- A generic `display:inline` test must not flatten replaced elements or atomic inline formatting
  contexts.
- Anonymous text and whitespace processing depend on logical sibling order, not widget layout
  order; recursive generalization must preserve that ordering.

## Exit Criteria

This follow-up is complete when:

- an eligible `UIRichText` with computed `display:inline` participates as a non-atomic inline
  container regardless of its construction-time widget class;
- runtime block/inline transitions preserve the DOM widget and converge without stale geometry;
- `UIRichText::rebuildRichText()` no longer uses `UI_TYPE_TEXTSPAN` as the definition of a
  non-atomic inline box;
- the inline branch contains no unsafe or conditionally invalid `UITextSpan` casts;
- stream construction, positioning, painting, and hit testing use the same centralized inline
  participation classification;
- replaced elements, inline-block/flex/grid, floats, positioned boxes, and flex/grid items retain
  their atomic or blockified behavior;
- focused mutation/fragment/whitespace tests and the full unit-test suite pass under the required
  wrapper;
- the performance audit finds no new steady-state heap work and the final diff contains no
  fixture-specific layout rules.
