# Border Box Model Content Offset Plan

## Problem Statement

In browser engines, content (text, child widgets) is positioned at `border-width + padding` from the element's edge. In eepp's GUI system, borders are `BorderType::Inside` by default — a pure visual decoration drawn OVER the padding/content area. Content is only offset by padding, ignoring border width entirely. This causes HTML widgets to render text misaligned compared to real browsers.

**User requirement:** UIHTMLWidget elements must behave like browsers — the border should consume space and content should be offset by border + padding.

---

## Key Concepts

| Concept | eepp Current | Browser/CSS | Desired |
|---------|-------------|-------------|---------|
| Border space | 0 (Inside type) | border-width | border-width |
| Content offset | padding only | border + padding | border + padding |
| `box-sizing` | not implemented | `content-box` (default) | add `content-box`/`border-box` |

**BorderType behavior:**
- `Inside` (default): border drawn inside element; `getBorderBoxDiff()` returns zero rect; no space consumed
- `Outside`: border extends outward from element; adds space outside
- `Outline`: border centered on element edge; half inside, half outside

For HTML compatibility, we treat borders as **space-consuming** regardless of BorderType — they push content inward by their width. This matches the CSS `content-box` model where `width` specifies the content area.

---

## Scope: What Changes

### 1. Add helper method to UINode/UIWidget

**File:** `include/eepp/ui/uiwidget.hpp` (declaration), `src/eepp/ui/uiwidget.cpp` (implementation)

Add `getPixelsContentOffset()` that returns a `Rectf` containing `padding + border` for all 4 sides. This becomes the single source of truth for content positioning in HTML widgets.

```cpp
// Returns the content area origin offset = padding + border
Rectf getPixelsContentOffset() const;
```

Implementation:
```cpp
Rectf UIWidget::getPixelsContentOffset() const {
    Rectf offset = getPixelsPadding();
    if (hasBorder()) {
        const auto& b = getBorder()->getBorders();
        offset.Left   += b.left.width;
        offset.Right  += b.right.width;
        offset.Top    += b.top.width;
        offset.Bottom += b.bottom.width;
    }
    return offset;
}
```

**Complexity: LOW** — one new method, ~15 lines.

---

### 2. BlockLayouter — Content Area Calculations

**File:** `src/eepp/ui/blocklayouter.cpp`

All locations that use `mContainer->getPixelsPadding()` for child positioning must switch to `mContainer->getPixelsContentOffset()`.

**Affected lines (~8 sites):**

| Line(s) | Current | Change |
|---------|---------|--------|
| 34-43 `computeIntrinsicWidths` | `getPixelsPadding().Left + .Right` | add border widths to intrinsic size |
| 74-77 `updateLayout` totW | `getPixelsPadding().Left + .Right` | `getPixelsContentOffset().Left + .Right` |
| 88-92 `updateLayout` totH | `getPixelsPadding().Top + .Bottom` | `getPixelsContentOffset().Top + .Bottom` |
| 161-167 `positionRichTextChildren` hitbox | `getPixelsPadding().Left/Top` | `getPixelsContentOffset().Left/Top` |
| 214-216 BR element width | `getPixelsPadding().Left + .Right` | `getPixelsContentOffset().Left + .Right` |
| 227-228 custom widget position | `getPixelsPadding().Left/Top` | `getPixelsContentOffset().Left/Top` |

**Complexity: MEDIUM** — mechanical replacement, ~8 call sites.

---

### 3. UIRichText — Text Rendering & Intrinsic Widths

**File:** `src/eepp/ui/uirichtext.cpp`

| Line(s) | Current | Change |
|---------|---------|--------|
| 180-186 `draw()` | `mScreenPos + mPaddingPx.Left/Top` | add border width to offset |
| 589-590 `rebuildRichText` maxWidth | `- getPixelsPadding().Left - .Right` | `- getPixelsContentOffset().Left - .Right` |
| 638-641 block child width | `- getPixelsPadding().Left - .Right` | `- getPixelsContentOffset().Left - .Right` |
| 665-668 child size computation | same pattern | same |
| 725-728 `getMinIntrinsicWidth` | `+ mPaddingPx.Left + .Right` | `+ getPixelsContentOffset().Left + .Right` |
| 753-756 `getMaxIntrinsicWidth` | same | same |

**Complexity: MEDIUM** — ~6 call sites, same mechanical pattern.

---

### 4. UIHTMLWidget — Out-of-Flow Children

**File:** `src/eepp/ui/uihtmlwidget.cpp`

| Line | Current | Change |
|------|---------|--------|
| 202 `positionOutOfFlowChildren` | `getPixelsPadding().Left, .Top` | `getPixelsContentOffset().Left, .Top` |

Container block origin for absolutely positioned children must include border.

**Complexity: LOW** — single line change.

---

### 5. TableLayouter — Table Layout

**File:** `src/eepp/ui/tablelayouter.cpp`

| Line(s) | Current | Change |
|---------|---------|--------|
| 274-277 `computeIntrinsicWidths` | `getPixelsPadding().Left + .Right` | `getPixelsContentOffset().Left + .Right` |
| 309 available width | `getPixelsPadding().Left + .Right` | `getPixelsContentOffset().Left + .Right` |
| 513, 516 row positioning | `getPixelsPadding().Left` | `getPixelsContentOffset().Left` |
| 527-529 wrap-content height | `getPixelsPadding().Top + .Bottom` | `getPixelsContentOffset().Top + .Bottom` |

**Complexity: LOW** — ~4 call sites.

---

### 6. UIWidget::getMatchParentWidth/Height

**File:** `src/eepp/ui/uiwidget.cpp` (lines ~2577-2617)

These methods calculate how much space a `match_parent` child can use. Currently subtract parent padding; must also subtract parent border.

```cpp
// Before:
Float width = getParent()->getPixelsSize().getWidth() - marginLeft - marginRight -
              padding.Left - padding.Right;
// After:
Rectf parentOffset = getParent()->asType<UIWidget>()->getPixelsContentOffset();
Float width = getParent()->getPixelsSize().getWidth() - marginLeft - marginRight -
              parentOffset.Left - parentOffset.Right;
```

**Complexity: LOW** — 2 methods, ~4 subtraction lines each.

---

### 7. UIWidget::calculateAutoMargin

**File:** `src/eepp/ui/uiwidget.cpp` (lines ~590-659)

`margin: auto` calculation uses parent padding to determine available space. Must include parent border.

```cpp
// Before:
Float availableWidth = parentSize.getWidth() - parentPadding.Left - parentPadding.Right -
                       getPixelsSize().getWidth();
// After:
Rectf parentContentOffset = ...getPixelsContentOffset();
Float availableWidth = parentSize.getWidth() - parentContentOffset.Left -
                       parentContentOffset.Right - getPixelsSize().getWidth();
```

**Complexity: LOW** — ~4 call sites.

---

### 8. (Optional) CSS `box-sizing` Property

**Scope:** Can be deferred. Adding it now would make the fix more complete but doubles the complexity.

If implemented:
- Add `BoxSizing` to `PropertyId` enum (`propertydefinition.hpp`)
- Register property: `registerProperty("box-sizing", "content-box")`
- Under `content-box`: width/height set on content area; border+padding added outside (current plan)
- Under `border-box`: width/height include border+padding; content = width - padding - border (would need reverse calculation)

**Complexity: HIGH** — new property, two calculation modes, affects all width/height resolution. Recommended as follow-up.

---

## Non-Scope / NOT Changing

- **Non-HTML widgets** (UIPushButton, UITextInput, etc.) — they continue using `getPixelsPadding()` directly and border remains decorative.
- **UINode::nodeDraw()` clip regions** — the clipping pipeline already uses `getBorderBoxDiff()` for BorderBox clip; no change needed.
- **UIBorderDrawable rendering** — border geometry generation is unchanged.
- **BorderType behavior** — Inside/Outside/Outline remain as-is; we only USE the border width value for content offset, regardless of type.
- **Background rendering** — backgrounds already render within the padded area; we're only moving content inward.

---

## Test Impact & Validation Protocol

### Expected Test Failures

This change alters the content area origin for all HTML widgets — text, child widgets, intrinsic widths, and match-parent sizing all shift. This means:

**Guaranteed to fail:**
- `UIBorder.renderingVariations` — text inside bordered boxes will shift inward by the border width, changing pixel positions. **This failure IS the expected correct behavior** (the test proves the fix works).
- `UIRichText.anchorMargins` — content offset changes affect the rendered layout.
- `UIRichText.spanPadding` — spans with padding inside bordered containers shift.
- `UIHTMLTable.complexLayout` (1,2,3) — any elements with borders will have their text/content shifted.

**Expected to pass unchanged:**
- Non-HTML widget tests (UILayout, FontRendering, etc.) — these widgets don't use the HTML border model.
- Tests where no element has a border — no content offset change occurs.

**Unknown (may or may not differ):**
- Margin-dependent tests (e.g., `UILayout.marginAuto`) — if parent has a border, `getMatchParentWidth/Height` result changes.
- Layout tests with nested containers — cascading size changes from border inclusion could alter layouts.

### What "Re-generate and Verify" Means

The `compareImages` helper in the unit tests works as follows:

1. **Golden image check:** On each test run, the rendered frame is captured via `win->getFrontBufferImage()` and pixel-compared against a stored `.webp` image at `bin/unit_tests/assets/<folder>/<imageName>.webp`.

2. **Auto-generation on first run:** If the golden image file does not exist, the captured frame is saved AS the new golden image and the test passes. This is how `eepp-ui-border-rendering.webp` was created.

3. **Re-generation for updated rendering:** To update a golden image after an intentional rendering change:
   ```bash
   # Delete the old golden image, re-run the test to auto-create a new one
   rm bin/unit_tests/assets/html/eepp-ui-table-complex.webp
   ASAN_OPTIONS=detect_leaks=0 xvfb-run -a -s "-screen 0 1280x1024x24" \
       bin/unit_tests/eepp-unit_tests-debug --filter="UIHTMLTable.complexLayout"
   ```

4. **Human validation is REQUIRED after re-generation.** The test will pass automatically once the golden image is regenerated, but this proves nothing — it only proves the rendering is consistent with itself. A human must visually inspect the new rendering (against the old golden image, or against a reference browser rendering) to confirm the change is correct and not a regression. The agent can assist by:
   - Describing expected visual differences (e.g., "all text should be shifted right by 4px in bordered elements")
   - Comparing pixel dimensions between old and new golden images
   - Rendering the same HTML in a reference browser for side-by-side comparison (if the agent has image analysis capabilities)

### Agent Protocol for Failing Tests

When tests fail due to expected rendering changes, the agent MUST:

1. **Report** which tests failed and whether the failure is expected (border-related shift) or unexpected (regression).
2. **Do NOT auto-regenerate** golden images without first describing the expected visual differences to the user.
3. **Request human validation** by explaining what changed and asking the user to confirm the new rendering looks correct. Example: *"The UIBorder.renderingVariations test failed because text inside bordered boxes shifted right by border-left-width and down by border-top-width. I'll regenerate the golden image now — please visually verify the result matches expectations."*
4. **Regenerate golden images only after approval** — delete the old `.webp`, re-run the test, and confirm it passes.
5. **Verify with a reference browser** if the agent has image analysis capabilities — render the same HTML in a browser and compare.

---

## Risk Assessment

| Risk | Severity | Mitigation |
|------|----------|------------|
| Breaking non-HTML widgets | HIGH | Helper method on UIWidget, but only HTML layouters (BlockLayouter, UIRichText, TableLayouter) call it. Non-HTML widgets keep using `getPixelsPadding()` directly. |
| Intrinsic width changes breaking layout | MEDIUM | Run existing HTML layout image tests after each change. Verify pixel-identical rendering with re-generated golden images. |
| Match-parent calculations | MEDIUM | `getMatchParentWidth/Height` is called by ALL widgets, not just HTML. Must gate the border addition on whether parent has a border. |
| Circle dependency on border resolution | LOW | `updateBorders()` is lazy — widths are empty strings until first draw. We must call `mOwner->lengthFromValue(...)` to resolve before reading. In the `getPixelsContentOffset()` method, `getBorder()->getBorders()` accesses already-resolved values — `updateBorders()` is called in `UIBorderDrawable::update()` before draw. |

---

## Implementation Order

1. **Add `getPixelsContentOffset()` method** to UIWidget (declaration + implementation)
2. **Update BlockLayouter** — switch all `getPixelsPadding()` to `getPixelsContentOffset()`
3. **Update UIRichText** — text rendering offset and intrinsic widths
4. **Update UIHTMLWidget** — out-of-flow children offset
5. **Update TableLayouter** — table cell padding offset
6. **Update `getMatchParentWidth/Height`** — gate on parent having border, subtract parent border
7. **Update `calculateAutoMargin`** — gate on parent having border, subtract parent border
8. **Run all tests** — identify which fail and classify as expected vs unexpected
9. **Request human validation** — for all tests with expected failures, describe the visual change and ask for confirmation
10. **Regenerate golden images after approval** — delete old `.webp` files, re-run tests to capture new baseline
11. **Verify non-HTML widgets unaffected** — ensure non-HTML tests still pass with existing golden images

---

## Verification

After implementation, the agent must:

1. **Run the full test suite** and compile a failure report categorizing each as:
   - **Expected failure (border shift):** tests where content moved due to border offset — these visually differ from old golden images
   - **Unexpected failure:** tests where the change caused a regression — these must be investigated and fixed
   - **Passing unchanged:** tests that continue to match their existing golden images

2. **For each expected failure**, describe to the user exactly what changed (e.g., "text in `UIRichText.anchorMargins` shifted right by the container's border-left-width of 4px"). See [Test Impact & Validation Protocol](#test-impact--validation-protocol).

3. **Await human approval** before regenerating any golden images.

4. **After approval**, regenerate golden images for the affected tests and confirm they pass.

5. **Verify** the `UIBorder.renderingVariations` test now produces the correct browser-like rendering (text inside bordered boxes is properly offset by border + padding).
