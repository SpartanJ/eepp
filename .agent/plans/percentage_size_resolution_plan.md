# Percentage Size Resolution Plan

## Goal

CSS-spec-compliant resolution of percentage `width` and `height` values, spanning both the HTML layouter pipeline and the non-HTML GUI layouters.

## CSS Spec Reference

From CSS 2.1 §10.5:

> If the height of the containing block is not specified explicitly (i.e., it depends on content height), and this element is not absolutely positioned, the value computes to `auto`.

The same rule applies to width §10.2. Symmetric treatment needed.

## Architecture Landscape

### Two Laying Hierarchies

| Hierarchy | Base | Purpose |
|---|---|---|
| `UILayout` → `UIWidget` | Non-HTML GUI containers | `UILinearLayout`, `UIGridLayout`, `UIStackLayout`, `UIRelativeLayout`, `UISplitter`, `UIHTMLWidget` |
| `UILayouter` (standalone) | HTML/CSS display-mode engines | `BlockLayouter`, `TableLayouter`, `InlineLayouter`, `NoneLayouter` |

`UIHTMLWidget` inherits from `UILayout` and delegates CSS layout to a `UILayouter*`.

### How Percentage Width/Height Becomes `SizePolicy::Fixed`

CSS `width: 85%` / `height: 100%` arrives at `UIWidget::applyProperty()` as `PropertyId::Width` / `PropertyId::Height`. The handler (non-auto branch) does:

```cpp
setLayoutWidthPolicy( SizePolicy::Fixed );
setSize( eefloor( lengthFromValueAsDp( "85%" ) ), ... );
```

`lengthFromValueAsDp` resolves the percentage against the parent's current pixel size, which may be 0 if the parent hasn't been laid out yet. The resolution is **not** re-evaluated later when the parent size becomes known—**unless** the layouter has explicit re-resolution logic.

### Where Re-resolution Exists Today (Width Only)

Both `BlockLayouter` and `TableLayouter` re-resolve `Fixed` width at layout time:

**`blocklayouter.cpp:64-69`:**
```cpp
const StyleSheetProperty* prop = nullptr;
if ( mContainer->getLayoutWidthPolicy() == SizePolicy::Fixed && 
     mContainer->getUIStyle() &&
     ( prop = mContainer->getUIStyle()->getProperty( PropertyId::Width ) ) ) {
    mContainer->setInternalPixelsSize(
        { mContainer->lengthFromValue( *prop ), mContainer->getPixelsSize().getHeight() } );
}
```

**`tablelayouter.cpp:294-298`:**
```cpp
if ( widget->getLayoutWidthPolicy() == SizePolicy::Fixed && 
     widget->getUIStyle() &&
     ( prop = widget->getUIStyle()->getProperty( PropertyId::Width ) ) ) {
    widget->asType<UINode>()->setInternalPixelsSize(
        { widget->lengthFromValue( *prop ), widget->getPixelsSize().getHeight() } );
}
```

There is **no equivalent height re-resolution** in either layouter. The height path only handles `WrapContent` (content-based) and ignores `Fixed` height:

**`blocklayouter.cpp:89-100`:**
```cpp
Float totH = mContainer->getPixelsSize().getHeight();
if ( mContainer->getLayoutHeightPolicy() == SizePolicy::WrapContent ) {
    totH = rt->getSize().getHeight() + ...;
}
// Fixed height passes through unchanged (stale 0px from early resolution)
```

## The Problem

Percentage height on a `WrapContent` parent produces `SizePolicy::Fixed` with 0px (parent un-laid-out). Without height re-resolution in the layouters, the 0px sticks. This causes the cascade of zero-height elements seen in `contact.html` (`.stylish-form` ul with `height: 100%` inside a WrapContent `form`).

## Non-HTML Layouter Impact Assessment

### `UILinearLayout` (`uilinearlayout.cpp`)

- **Children's Fixed policy**: Handled as no-op in `applyWidthPolicyOnChildren` (line 102-104: `case SizePolicy::Fixed: default: {}`) and `applyHeightPolicyOnChildren` (line 151-153). No re-resolution of percentage values.
- **Own Fixed size**: Not re-resolved from CSS property. Uses whatever was set.
- **Impact of fix**: `layout-height: 100%` on a child inside a `WrapContent` parent would correctly switch to `WrapContent` at `applyProperty` time. Since `UILinearLayout` doesn't do CSS-style percentage re-resolution, the `applyProperty`-level fix is sufficient.
- **Risk**: None. Non-HTML widgets don't use CSS `width`/`height` properties (they use `layout-width`/`layout-height`). The `PropertyId::Width`/`PropertyId::Height` path in `applyProperty` is only triggered by CSS parsing into HTML elements.

### `UIGridLayout` (`uigridlayout.cpp`)

- Forces all children to `SizePolicy::Fixed` (line 163). No percentage re-resolution.
- **Impact**: None. Grid layout overrides child size policies anyway.

### `UIStackLayout` (`uistacklayout.cpp`)

- `applySizePolicyOnChildren()`: `Fixed` is no-op (lines 55, 73). No re-resolution.
- **Impact**: Same as `UILinearLayout` — `applyProperty` fix is sufficient.

### `UIRelativeLayout` (`uirelativelayout.cpp`)

- `fixChildSize()`: `Fixed` is no-op (lines 148, 165). No re-resolution.
- **Impact**: Same as above.

### `UISplitter` (`uisplitter.cpp`)

- Forces children to `Fixed` (line 186). Uses `StyleSheetLength("50%")` for split partition.
- **Impact**: None.

### Summary: Non-HTML Layouters

None of the non-HTML layouters re-resolve percentage CSS properties. They don't need to because they don't use CSS `width`/`height` — they use eepp's `layout-width`/`layout-height` (or direct `setSize`). The `applyProperty` fix only fires for `PropertyId::Width`/`PropertyId::Height` (CSS properties from parsed stylesheets), not for `PropertyId::LayoutWidth`/`PropertyId::LayoutHeight` (eepp-specific properties) **unless** the latter are set to percentage values by user code.

For the `PropertyId::LayoutWidth`/`PropertyId::LayoutHeight` cases, the `applyProperty` fix also applies the percentage-on-WrapContent-parent check. This is correct — if a user writes `layout-height="100%"` on a widget inside a WrapContent parent, the engine should treat it as auto.

## Implementation Steps

### Step 1: Restore Width Fix in `applyProperty` (`uiwidget.cpp`)

Re-apply the percentage-on-WrapContent-parent check to `PropertyId::Width` (currently reverted). Keep the existing `PropertyId::Height` fix.

**Rationale**: Symmetric CSS spec treatment. The width re-resolution in the layouters (BlockLayouter line 64-69, TableLayouter line 294-298) already handles the case where the parent HAS explicit dimensions — they re-compute from the CSS property at layout time. The `applyProperty` fix only fires when the parent has `WrapContent` (no explicit dimension), which is exactly when CSS spec says the percentage should compute to `auto`.

**Files**: `src/eepp/ui/uiwidget.cpp`
- `PropertyId::Width` (line ~1932): Add percentage-on-WrapContent-parent check (restore what was reverted)
- `PropertyId::LayoutWidth` (line ~2169): Same check (restore what was reverted)
- `PropertyId::Height` and `PropertyId::LayoutHeight`: Already have the fix

### Step 2: Add Height Re-resolution in BlockLayouter (`blocklayouter.cpp`)

Mirror the existing width re-resolution pattern for height. Insert before the RichText rebuild:

```cpp
// After setMatchParentIfNeededVerticalGrowth(), before rebuildRichText:
if ( mContainer->getLayoutHeightPolicy() == SizePolicy::Fixed && 
     mContainer->getUIStyle() &&
     ( prop = mContainer->getUIStyle()->getProperty( PropertyId::Height ) ) ) {
    mContainer->setInternalPixelsSize(
        { mContainer->getPixelsSize().getWidth(), mContainer->lengthFromValue( *prop ) } );
}
```

This handles the case where an element has `height: 100%` on a parent that DOES have an explicit height (MatchParent or Fixed), ensuring the percentage resolves at layout time when parent dimensions are known.

**Rationale**: The `applyProperty` fix handles the `WrapContent` parent case (switches to WrapContent). The layouter re-resolution handles the `MatchParent`/`Fixed` parent case (re-computes the percentage against the now-laid-out parent). Together they cover all cases.

**Files**: `src/eepp/ui/blocklayouter.cpp`

### Step 3: Add Height Re-resolution in TableLayouter (`tablelayouter.cpp`)

Same pattern. Insert after `setMatchParentIfNeededVerticalGrowth()` (line 290):

```cpp
if ( widget->getLayoutHeightPolicy() == SizePolicy::Fixed && 
     widget->getUIStyle() &&
     ( prop = widget->getUIStyle()->getProperty( PropertyId::Height ) ) ) {
    widget->setInternalPixelsSize(
        { widget->getPixelsSize().getWidth(), widget->lengthFromValue( *prop ) } );
}
```

**Files**: `src/eepp/ui/tablelayouter.cpp`

### Step 4: Write Tests

- **ContactFormLayout** (already done in `uihtml_tests.cpp`): Verifies height resolution for `height: 100%` on WrapContent parent.
- **New test: PercentageHeightOnFixedParent**: Creates an element with `height: 100%` inside a parent with explicit `height: 300px`. Verifies the child gets 300px at layout time (tests the `BlockLayouter` height re-resolution).
- **New test: PercentageWidthOnWrapContentParent**: Creates an element with `width: 100%` inside a `display: inline-block` parent (which has WrapContent width). Verifies the child gets content-based width (tests the width fix in `applyProperty`).
- **New test: PercentageWidthTableOnFixedParent**: Creates a table with `width: 85%` inside a parent with `width: 500px`. Verifies the table gets 425px (tests `TableLayouter` width re-resolution with the fix in place).

## Risk Assessment

| Risk | Mitigation |
|---|---|
| Percentage width fix breaks table layouts (like Hacker News) | Table's parent (block element) has `MatchParent` width, NOT `WrapContent`. The fix only fires for `WrapContent` parents. The existing width re-resolution in `TableLayouter` continues to work for `Fixed`-policy tables. |
| Height re-resolution conflicts with `setMatchParentIfNeededVerticalGrowth` | `setMatchParentIfNeededVerticalGrowth` only fires for `MatchParent` policy. The re-resolution fires for `Fixed` policy. They handle mutually exclusive cases. |
| Non-HTML layouters affected by `LayoutHeight` fix | EE-internal `layout-height` with percentage values is extremely rare. Even if it occurs, the correct CSS-spec behavior is to treat it as auto on a WrapContent parent. This is a correctness improvement. |
| Existing image comparison tests | The 3 `complexLayout` tests must be re-verified after changes. No image diffs expected (see risk #1). |

## Files Modified

| File | Change |
|---|---|
| `src/eepp/ui/uiwidget.cpp` | Restore percentage-on-WrapContent check for `PropertyId::Width` and `PropertyId::LayoutWidth` |
| `src/eepp/ui/blocklayouter.cpp` | Add height re-resolution (mirror existing width pattern) |
| `src/eepp/ui/tablelayouter.cpp` | Add height re-resolution (mirror existing width pattern) |
| `src/tests/unit_tests/uihtml_tests.cpp` | Add new tests for various percentage scenarios |
| `.agent/plans/percentage_size_resolution_plan.md` | This plan |
