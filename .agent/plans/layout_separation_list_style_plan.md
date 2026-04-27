# UI Layout Phase 7: CSS List Style Type Plan

This document outlines the architectural plan for implementing the CSS `list-style-type` property within the decoupled layout system. This replaces the current CSS background-image hacks with proper text-based list markers.

**AGENT DIRECTIVE (CRITICAL):** You MUST compile and run the unit tests (`bin/unit_tests/eepp-unit_tests-debug`) after EVERY step. Do NOT proceed to the next step if there is even a 1-pixel difference in visual layout tests. Take a git stash snapshot (`git stash push -m "Phase 7.X passed" && git stash apply`) upon passing a step to keep a checkpoint while continuing to work. **If you need to restore a stash, use `git stash apply` instead of `git stash pop` so the stable snapshot is never lost.**

---

## IMPLEMENTATION HAZARDS (READ BEFORE CODING)
1. **Property Inheritance:** `list-style-type` must be an inherited property so that setting it on `<ul>` or `<ol>` applies it to the `<li>` children.
2. **Clipping:** `UIRichText` clips its content to the content box (`mSize - mPadding`). In CSS, when `list-style-position` is `outside` (the default), the marker is drawn in the padding/margin area. The marker MUST be drawn outside of the `clipSmartEnable` block in `UIRichText::draw()` to remain visible.
3. **Sibling Indexing:** Ordered lists (`decimal`, etc.) require knowing the element's index. Only count previous siblings that are actual `<li>` elements (check `getTag() == "li"` or similar).

---

## Phase 7: List Style Type implementation

**Step 7.1: CSS Enums and Properties**
- In `csslayouttypes.hpp`, define:
  ```cpp
  enum class CSSListStyleType { None, Disc, Circle, Square, Decimal, LowerAlpha, UpperAlpha, LowerRoman, UpperRoman };
  ```
  And its helper functions (`CSSListStyleTypeHelper::fromString`, etc.).
- In `propertydefinition.hpp`, add `PropertyId::ListStyleType`.
- In `stylesheetspecification.cpp`, register the property as inherited:
  ```cpp
  registerProperty( "list-style-type", "none", true );
  ```
- In `UIHTMLWidget`, add `mListStyleType` defaulting to `None`. Update `applyProperty` to parse it.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)

**Step 7.2: Marker String Generation & Text Cache**
- In `include/eepp/ui/uirichtext.hpp`, add a `Text mListMarkerText;` member to cache the rendered marker.
- Add a helper function `String getListMarkerString() const;` that:
  - Returns `""` for `None`.
  - Returns appropriate unicode characters for `Disc` ("•"), `Circle` ("○"), `Square` ("■").
  - For ordered types like `Decimal`, counts preceding `<li>` siblings and formats the string (e.g., `1. `, `2. `).
- In `UIRichText::updateLayout()`, after styling is resolved, update `mListMarkerText`:
  - Set its string using `getListMarkerString()`.
  - Copy the font, size, and color from `mRichText.getFontStyleConfig()`.
- **Validation:** Compile and run all tests. (Snapshot)

**Step 7.3: Rendering the Marker**
- In `UIRichText::draw()`, add logic to render `mListMarkerText` if its string is not empty.
- **Positioning:**
  - `X`: `mScreenPos.x + mPaddingPx.Left - mListMarkerText.getTextWidth() - offset`. (You may use a small hardcoded offset like `0.25em` derived from the font size to give it breathing room from the text).
  - `Y`: `mScreenPos.y + mPaddingPx.Top`. This aligns it with the start of the first line of text.
- Ensure the drawing logic is placed *outside* the `if (isClipped()) { clipSmartEnable... }` block so it is not clipped away by the padding.
- **Validation:** Compile and run all tests. (Snapshot)

**Step 7.4: Remove CSS Hacks & Update Tests**
- Open `bin/assets/ui/breeze.css` and remove the `background-image`, `background-tint`, `background-position`, and `background-size` hacks for `ol > li` and `ul > li`.
- Change them to properly use `list-style-type`:
  - `ul > li { list-style-type: disc; padding-left: 2em; }`
  - `ol > li { list-style-type: decimal; padding-left: 2em; }`
- Create a specific unit test in `src/tests/unit_tests/uihtml_tests.cpp` (or a dedicated layout test) to verify `list-style-type: decimal` correctly increments numbers and `list-style-type: disc` draws a bullet.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)
