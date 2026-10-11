# UI direction and right-to-left layouts

eepp shapes and renders bidirectional text with HarfBuzz and SheenBidi. On top of that, every
widget has a **UI direction** that decides how *logical* UI concepts resolve to physical sides.
A layout authored with logical properties works unchanged in left-to-right (LTR) and
right-to-left (RTL) applications.

The central rule:

> Direction changes the meaning of logical UI concepts; it never changes the meaning of
> explicitly physical concepts.

| Logical (direction-sensitive) | Physical (never mirrored) |
| --- | --- |
| `start`, `end` (gravity, layout-gravity, text-align) | `left`, `right`, `center` |
| `margin-inline-start`, `margin-inline-end`, `margin-inline` | `margin-left`, `margin-right`, `margin` |
| `padding-inline-start`, `padding-inline-end`, `padding-inline` | `padding-left`, `padding-right`, `padding` |
| `layout-to-start-of`, `layout-to-end-of` | `layout-to-left-of`, `layout-to-right-of` |
| `UIHorizontalAlignment::Start/End`, `PositionPolicy::StartOf/EndOf` | `UI_HALIGN_LEFT/RIGHT`, `UI_ANCHOR_LEFT/RIGHT`, `PositionPolicy::LeftOf/RightOf` |

In LTR, `start` is left and `end` is right. In RTL, `start` is right and `end` is left.

## Setting the direction

```cpp
// Whole application (every widget that does not override its direction inherits it).
app.getUI()->setDirection( UIDirection::RightToLeft );

// A subtree.
panel->setDirection( UIDirection::LeftToRight );
panel->clearDirectionOverride(); // inherit again
```

```xml
<vbox direction="rtl">
	<TextView text="..." />
	<CodeEditor direction="ltr" />
</vbox>
```

```css
.sidebar { direction: rtl; }
```

```html
<body dir="rtl">
	<p>...</p>
	<div dir="ltr">...</div>
</body>
```

* `UINode::getDirection()`, `isRTL()` and `isLTR()` return the cached effective direction. They
  are cheap to call from layout and draw code; no parent walk happens.
* Owned menus inherit direction from their trigger, including after reparenting to the window
  container. Explicit menu overrides still take precedence.
* The effective direction is the node's own override or the direction of its nearest UI ancestor.
  Scene roots default to LTR. Directions do not cross scene boundaries, so a `UIWebView` document
  starts as LTR, like a browser document inside its embedder.
* Changing a direction at runtime re-resolves logical alignments, logical margins/padding and
  logical relations, and invalidates the affected layouts. Styles are not reloaded and widgets are
  not rebuilt. Reparenting a widget into a differently directed parent updates its subtree.
* Widgets override `onDirectionChanged()` (calling the base implementation) to refresh
  direction-sensitive internals.

`UIDirectionHelper::fromLocale()` (also available as `directionForLocale()`) returns the
conventional direction of a locale name (`ar`, `he_IL`, `fa-IR`, `ur.UTF-8`, ...). eepp never
changes the direction on its own when the locale changes: applications decide their policy and
call `setDirection()` explicitly, which also allows a manual override.

## Logical alignment

Widgets default to the logical `start` horizontal alignment and layout gravity. In LTR this is
exactly the historical `left` default, so existing applications are unchanged; in RTL, labels,
text inputs, list items and layout children align to the right.

```xml
<TextView text="..." text-align="start" />
<PushButton layout_gravity="end|center_vertical" />
<TextView gravity="end" />
```

C++: `setHorizontalAlignment()`, `setLayoutHorizontalAlignment()` and `setTextAlignment()` accept
`UIHorizontalAlignment::Start`/`End`. The physical setters (`setHorizontalAlign()`,
`setLayoutGravity()` including its zero-valued left bits, `setTextAlign()`, `resetFlags()`)
keep their physical meaning and replace a logical alignment. `setFlags()` does so only for
`UI_HALIGN_CENTER` and `UI_HALIGN_RIGHT`: `UI_HALIGN_LEFT` is zero, so `setFlags( UI_HALIGN_LEFT )`
keeps a logical alignment. Use `setHorizontalAlign( UI_HALIGN_LEFT )` to force physical left.

## Flow-relative margins and padding

```css
.toolbar-button { margin-inline-start: 8dp; }
.item { padding-inline: 12dp 4dp; }
```

Logical and physical declarations that resolve to the same edge cascade normally: the higher
specificity wins and, with the same specificity, the later declaration wins. Physical properties
are never converted at parse time, so the result follows direction changes. Layout code that needs
a declared margin or padding value uses `UIWidget::getBoxEdgeProperty()`.

## Layouts

* **Horizontal `LinearLayout`** is an inline row: in RTL its first child is placed at the right.
  Node order does not change, and physical margins/padding stay on their physical sides.
  Vertical layouts keep their top-to-bottom order and resolve `start`/`end` gravity.
* **`FlowLayout`** rows are mirrored the same way. **`GridLayout`** cells start from the right
  through its default `start` alignment.
* **`RelativeLayout`** resolves `layout-to-start-of`/`layout-to-end-of` with the child direction.
* **Flexbox** (`row`, `row-reverse`, and the cross axis of columns) uses the inline-start edge as
  main-start, including auto margins and `justify-content`.
* **CSS Grid** numbers columns from the inline-start edge; `justify-self: start/end` follow the
  direction; margins stay physical.
* **HTML inline formatting**: a right-to-left paragraph fills its lines from the right. Inline items
  are reordered per line using the UAX #9 reordering rule at inline-item granularity, so runs of
  left-to-right items keep their order; text inside each item is shaped with the paragraph
  direction. `float: left/right` stays physical. List markers are drawn on the inline-start side.

## Text

An explicitly directed subtree uses its direction as the base paragraph direction of
`UITextView`, `UITextInput` (including hints), tooltips, check boxes, buttons and `UIRichText`. Widgets that
never received a direction keep first-strong-character detection, the historical behavior of LTR
applications. `UICodeEditor`/`UITextEdit` keep their own `setTextDirection()` setting.

Caret placement, hit testing and selection use the shaped layout with the same base direction, so
visual Left/Right movement and logical movement stay distinct. Tab and Shift+Tab focus traversal
always follow document order.

## Standard controls

| Control | RTL behavior |
| --- | --- |
| CheckBox, RadioButton | Indicator at the right, text before it |
| PushButton | `InnerWidgetOrientation` is a logical sequence: the icon is at the right of the text |
| Menu items | Shortcut at the inline-end (left); icon gutter at the right |
| Submenus | Arrow at the left, mirrored; the submenu prefers opening to the left and still flips when it does not fit |
| MenuBar | First menu at the right; menus hang from the button's right edge; Left/Right keys move to the visually adjacent menu |
| TabWidget | Tab 0 at the right; indexes, next/previous and drag reordering stay logical |
| TreeView | Indentation and expander at the right; expanders mirrored; cells default to `start` |
| TableView, ListView | Cells default to `start`; column order is not reversed |
| Window chrome | Title buttons mirrored to the left (custom eepp decorations only) |

The default themes use logical properties where spacing means "before" or "after". Application
themes should do the same for directional spacing.

## Directional icons

Mirroring is opt-in: arbitrary images, media controls and code never mirror. Framework-owned
affordances (submenu arrows, tree expanders) mirror automatically. Applications can opt in for
back/forward style icons with `setMirrorOnRTL( true )` or the `mirror-on-rtl: true` CSS property.

## Current limitations

* `dir="auto"` and `unicode-bidi` are not modeled; `auto` behaves as `inherit`.
* Vertical writing modes and logical border/inset properties are not implemented.
* Scrollbars keep their physical side and scroll offsets stay physical.
* Table column order and native window decorations are not mirrored.
* Inline box decorations (padding/borders of inline elements split across reordered RTL items) use
  their logical start/end only approximately.
* Inline bidi placement operates at item granularity. Full paragraph bidi across mixed-direction
  styling boundaries and wrapped-line context is not yet modeled.

See `src/examples/ui_rtl/ui_rtl.cpp` for a runnable example that toggles the direction at runtime.
