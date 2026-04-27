# UI Layout Separation & CSS Display/Position Support Plan (Strict Implementation Guide)

This document outlines the architectural plan for decoupling layout logic from specific widgets (like `UIRichText` and `UIHTMLTable`), introducing a robust generic layouter system, and supporting standard CSS `display` and `position` properties.

**AGENT DIRECTIVE (CRITICAL):** You are Negen. Fulfill this plan iteratively. You MUST compile and run the unit tests (`bin/unit_tests/eepp-unit_tests-debug`) after EVERY step. Do NOT proceed to the next step if there is even a 1-pixel difference in visual layout tests. Take a git stash snapshot (`git stash push -m "Phase X.Y passed" && git stash apply`) upon passing a step to keep a checkpoint while continuing to work. **If you need to restore a stash, use `git stash apply` instead of `git stash pop` so the stable snapshot is never lost.**

---

## IMPLEMENTATION HAZARDS (READ BEFORE CODING)
When migrating logic from Widgets to Layouters, you will face these traps:
1. **Pixel vs DP APIs:** Widgets have `getSize()` and `getPixelsSize()`, `getPadding()` and `getPixelsPadding()`. **Layouters MUST exclusively use the `Pixels` variants** (`getPixelsSize()`, `getPixelsPadding()`, `getLayoutPixelsMargin()`). Using the non-pixel variants will cause massive visual regressions due to DPI scaling.
2. **Infinite Recursion:** When a Layouter delegates to a widget's intrinsic width calculation, or vice versa, you must strictly manage the dirty flags (e.g., `mIntrinsicWidthsDirty = false`). Failure to clear these flags inside `computeIntrinsicWidths` will cause stack overflows.
3. **Table Hierarchy:** In HTML/eepp tables, the hierarchy is `Table` -> `TableSection` (`thead`, `tbody`) -> `TableRow` (`tr`) -> `TableCell` (`td`). The original code often checks `row->getParent()->isType(...)`. If you change sections to `UIHTMLWidget`, be extremely careful not to accidentally assign them a `BlockLayouter` that overrides the `TableLayouter`'s positioning.

---

## Phase 1: Core Infrastructure

**Step 1.1: CSS Properties & Enums**
- Add `Display`, `Position`, `Top`, `Right`, `Bottom`, `Left`, `ZIndex` to `PropertyId` enum (`propertydefinition.hpp`).
- Create `CSSDisplay` and `CSSPosition` enums (`csslayouttypes.hpp`).
- **Validation:** Compile and run all tests. Must pass. (Snapshot)

**Step 1.2: Layouter Interfaces & Manager**
- Create `UILayouter` base interface (`uilayouter.hpp`). Must hold `UIWidget* mContainer` and `bool mValid`.
- Create `UILayouterManager` (`uilayoutermanager.hpp`/`cpp`) to spawn layouters.
- Create empty skeletons for `BlockLayouter`, `InlineLayouter`, `TableLayouter`, `NoneLayouter`.
- **CRITICAL:** Run `premake4` to regenerate makefiles now that new files are added.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)

---

## Phase 2: UIHTMLWidget Base Class

**Step 2.1: Implement UIHTMLWidget**
- Create `UIHTMLWidget` inheriting from `UILayout`.
- Add CSS properties (`mDisplay`, `mPosition`, offsets, `mZIndex`).
- Implement `getLayouter()` which lazily instantiates via `UILayouterManager`.
- Override `onChildCountChange` and `onDisplayChange` to call `if (mLayouter) mLayouter->invalidate()`.
- **Validation:** Compile and run all tests. Must pass. (Snapshot)

---

## Phase 3: UIRichText & BlockLayouter (High Risk)

**Step 3.1: Inheritance and Access Control**
- Change `UIRichText` and `UITextSpan` to inherit from `UIHTMLWidget`.
- Add `BlockLayouter` and `InlineLayouter` as `friend class` to `UIRichText`, `UILayout`, and `UIWidget`.
- **CRITICAL:** In `UIHTMLWidget::getLayouter()`, temporarily return `nullptr`. We want `UIRichText` to still use its monolithic logic while we set up inheritance.
- **Validation:** Compile and run `UIRichText.*` tests. Must pass exactly. (Snapshot)

**Step 3.2: Implement BlockLayouter**
- Copy `UIRichText::updateLayout()`, `getMinIntrinsicWidth()`, and `getMaxIntrinsicWidth()` into `BlockLayouter`.
- **INVARIANTS TO MAINTAIN:**
  - You MUST use `getPixelsPadding()` everywhere `mPaddingPx` was used.
  - You MUST use `getPixelsSize()` everywhere `mSize` was used.
  - The `mResizedCount` loop must be preserved: if `richText->mResizedCount > 0` after `setInternalPixelsWidth/Height`, `positionRichTextChildren` must run again.
  - Do NOT skip `MatchParent` children in the `positionRichTextChildren` while loop.
- Enable `BlockLayouter` for `UIRichText` (but explicitly disable it for `UI_TYPE_HTML_TABLE_CELL` for now to isolate bugs).
- **Validation:** Compile and run `UIRichText.*` tests. **Zero pixel difference allowed.** (Snapshot)

**Step 3.3: Implement InlineLayouter**
- Replicate inline logic for `UITextSpan` into `InlineLayouter`.
- **Validation:** Compile and run all tests. (Snapshot)

---

## Phase 4: UIHTMLTable Refactoring (High Risk)

**Step 4.1: Table Base Classes & Friends**
- Change `UIHTMLTable`, `UIHTMLTableRow`, `UIHTMLTableHead`, `UIHTMLTableBody`, `UIHTMLTableFooter` to inherit from `UIHTMLWidget`.
- Make `TableLayouter` a friend of `UIHTMLTable`.
- **CRITICAL:** `UILayouterManager` MUST return `nullptr` for Table Sections and Table Rows. Only the `Table` itself gets `TableLayouter`.
- **Validation:** Compile and run `UIHTMLTable.*` tests. (Snapshot)

**Step 4.2: Implement TableLayouter**
- Move `UIHTMLTable::updateLayout()` and `computeIntrinsicWidths()` into `TableLayouter`.
- **INVARIANTS TO MAINTAIN:**
  - `computeIntrinsicWidths` MUST clear `table->mIntrinsicWidthsDirty = false;` at all exit points to prevent recursion.
  - `currentY` for rows MUST be accumulated exactly as: `Float currentY = padding.Top + mCellspacing - headHeight;` then incremented by `rowHeight + mCellspacing`. Do not attempt to "fix" this math; it offsets based on section anchors.
  - When determining `headHeight`, use `row->getParent()->isType(UI_TYPE_HTML_TABLE_HEAD)`. Do not check the cell's parent.
- Enable `TableLayouter` in `getLayouter()`.
- **Validation:** Compile and run `UIHTMLTable.*` tests. **Zero pixel difference allowed.** (Snapshot)

**Step 4.3: Unify TableCell with BlockLayouter**
- Now that TableLayouter is proven, allow `UILayouterManager` to return `BlockLayouter` for `CSSDisplay::TableCell`.
- Ensure `BlockLayouter` does NOT override fixed widths if the container is a `TableCell` (the table layouter owns the cell width).
- **Validation:** Run all tests. (Snapshot)

---

## Phase 5: CSS Position (Out-of-Flow)

**Step 5.1: Position Implementation**
- Implement `getContainingBlock()` in `UIHTMLWidget`.
- Update layouters to skip children with `position: absolute|fixed`.
- Add `positionOutOfFlowChildren()` to `UIHTMLWidget::updateLayout()` after the layouter finishes.
- **Validation:** Compile and run all tests. Existing UI should remain unaffected. (Snapshot)