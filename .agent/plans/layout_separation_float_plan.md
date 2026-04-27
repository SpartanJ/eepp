# UI Layout Phase 6: CSS Float & Clear Plan

This document outlines the architectural plan for implementing CSS `float` and `clear` support within the decoupled layout system, leveraging the `Graphics::RichText` engine for mixed content formatting.

**AGENT DIRECTIVE (CRITICAL):** You MUST compile and run the unit tests (`bin/unit_tests/eepp-unit_tests-debug`) after EVERY step. Do NOT proceed to the next step if there is even a 1-pixel difference in visual layout tests. Take a git stash snapshot (`git stash push -m "Phase 6.X passed" && git stash apply`) upon passing a step to keep a checkpoint while continuing to work. **If you need to restore a stash, use `git stash apply` instead of `git stash pop` so the stable snapshot is never lost.**

---

## IMPLEMENTATION HAZARDS (READ BEFORE CODING)
1. **Keyword Collision:** `Float` is a C++ type (`typedef float Float`). When defining the CSS enum, you MUST name it `CSSFloat` to avoid compiler collisions.
2. **Y-Coordinate Interleaving:** `RichText::updateLayout` currently breaks lines independently of their Y position, and only computes Y coordinates *after* all lines are formed. Because floating elements alter the available horizontal width at specific Y coordinate ranges, you will have to calculate `curY` *during* the block iteration, keeping track of active floats to restrict `curX` and `maxWidth`.
3. **Out-Of-Flow Precedence:** Floating elements are *not* out-of-flow in the same way `position: absolute` elements are. `absolute` elements are ignored by layouters, whereas `float` elements strictly participate in and influence the block formatting context (they take up space and push text around). Do not mark them as `isOutOfFlow() = true` in `UIRichText::rebuildRichText`.

---

## Phase 6: Float and Clear implementation

**Step 6.1: CSS Enums and Properties**
- In `csslayouttypes.hpp`, define:
  ```cpp
  enum class CSSFloat { None, Left, Right };
  enum class CSSClear { None, Left, Right, Both };
  ```
  And their helper parsing functions (`CSSFloatHelper::fromString`, etc.).
- In `propertydefinition.hpp`, ensure `PropertyId::Float` and `PropertyId::Clear` exist (if not, add them, avoiding conflicts).
- In `UIHTMLWidget`, add `mFloat` and `mClear` members (defaulting to `None`).
- In `UIHTMLWidget::applyProperty`, parse the `Float` and `Clear` properties. Call `notifyLayoutAttrChange()` when they change.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)

**Step 6.2: Extend RichText API**
- In `include/eepp/graphics/richtext.hpp`, update `RichText::addCustomSize`:
  ```cpp
  void addCustomSize( const Sizef& size, bool isBlock, CSSFloat floatType = CSSFloat::None, CSSClear clearType = CSSClear::None );
  ```
- Update `CustomBlock` struct to store `floatType` and `clearType`.
- In `UIRichText::rebuildRichText`, extract `getCSSFloat()` and `getCSSClear()` from the child widget (defaulting to `None` if the child isn't an HTML widget). Pass these to `richText.addCustomSize`.
- **Validation:** Compile and run all tests. (Snapshot)

**Step 6.3: Core RichText Layout Algorithm (The Tricky Part)**
- In `RichText::updateLayout()`, introduce Y-coordinate awareness during the main loop:
  - Create tracking lists: `std::vector<Rectf> leftFloats; std::vector<Rectf> rightFloats;`
  - Introduce `Float curY = 0;`
  - Before placing *any* block (text or custom), process `clear`: if the block has `clear: left`, advance `curY` past the `bottom` of all `leftFloats`. (Same for `right` and `both`). Reset `curX` and push a new `RenderParagraph` if `curY` changed.
  - Compute `availableLeft(curY)` and `availableRight(curY, mMaxWidth)`. Your `curX` must never be less than `availableLeft`.
  - **If the block is a float:**
    - Place it immediately at `availableLeft` (if left) or `availableRight - width` (if right).
    - Add its bounding box `{x, curY, width, height}` to the respective float list.
    - Do *not* advance `curX` for the normal inline flow.
    - Do *not* trigger a new line for normal flow text (floats are pulled out of the inline line box).
  - **If the block is normal text/inline:**
    - Adjust `LineWrap::computeLineBreaksEx` to respect the narrowed `mMaxWidth` computed from `availableRight - availableLeft`. *(Note: You may need to handle the case where text wraps below a float and reclaims full width. This can be done by processing text in line-height chunks if constrained by a float).*
- Make sure `curY` is updated when normal lines wrap.
- **Validation:** This is the most complex step. Ensure all existing tests pass exactly (0 pixels difference) before writing float-specific tests. (Snapshot)

**Step 6.4: Float/Clear Layout Tests**
- In `src/tests/unit_tests/uihtml_position_tests.cpp` (or a new `uihtml_float_tests.cpp`), write robust tests for:
  - Text wrapping around a `float: left` block.
  - Two consecutive `float: left` blocks stacking horizontally.
  - A block with `clear: both` jumping below all floats.
  - `BlockLayouter` correctly locating the `CustomBlock` widgets where `RichText` positioned the floats.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)
