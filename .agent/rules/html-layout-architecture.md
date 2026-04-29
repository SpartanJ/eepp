# HTML Layout Architecture

This document describes the decoupled HTML/CSS layout engine architecture implemented in `eepp` for `UIHTMLWidget` and related classes.

## Core Concepts

### 1. UIHTMLWidget
`UIHTMLWidget` is the base class for all HTML-like elements. It holds parsed CSS properties (Display, Position, Float, Clear, etc.). Instead of implementing complex layout math directly, it queries a `UILayouterManager` to instantiate the appropriate `UILayouter` based on its `CSSDisplay` property.

### 2. Layouters
Layout math has been extracted from widgets into stateless (or locally stateful) "Layouters":
- **BlockLayouter:** Handles `CSSDisplay::Block`. It positions block-level children vertically. For rich text, it delegates text shaping to the `RichText` engine and simply maps physical coordinates for custom inline widgets.
- **TableLayouter:** Handles `CSSDisplay::Table`. Encapsulates HTML table column width distribution and row positioning.
- **InlineLayouter:** Handles `CSSDisplay::Inline`. *This layouter is empty by design.* Inline formatting (like `<span>` or `<a>`) is completely managed by the nearest Block container (via the `RichText` engine). It acts as a no-op so standard linear layout logic doesn't override text flows.
- **NoneLayouter:** Handles `CSSDisplay::None`. Skips all layout and rendering.

### 3. The UIRichText Engine Integration
`UIRichText` acts as the primary block container for mixed text and widget content. 
- It uses `rebuildRichText()` to recursively traverse its children.
- Pure text nodes (`UITextSpan`, `<br>`) are appended to the core `RichText` engine via `RichText::addSpan()`.
- Arbitrary inline widgets (e.g., `<input>`, `<button>`, or images) are passed to the engine via `RichText::addCustomSize()`.
- After `RichText` performs line-wrapping, `BlockLayouter` iterates over the resulting `CustomBlock`s and calls `setPixelsPosition()` on those child widgets to match where the engine placed them.

### 4. Pixel (dp) Math strictly enforced
All layouters **MUST** use Pixel (`Px`) variants of size and padding APIs.
- Use `getPixelsSize()`, `getPixelsPadding()`, and `getLayoutPixelsMargin()`.
- Never use `getSize()` or `getPadding()`, as these return density-independent pixels (dp) and will cause severe calculation bugs on HiDPI displays if mixed with pixel calculations.

### 5. CSS Position (Out-Of-Flow)
Elements with `position: absolute` or `position: fixed`:
- Are ignored by standard Layouters and `UIRichText::rebuildRichText()`.
- Are positioned at the end of the parent's `updateLayout()` using `positionOutOfFlowChildren()`.
- Absolute elements are positioned relative to their `getContainingBlock()` (the nearest positioned ancestor). Fixed elements map to the `UISceneNode` root.
