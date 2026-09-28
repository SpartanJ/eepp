# Native UI Layout Reference

This document is the authoritative reference for eepp's **native application UI layout system**.

It describes the behavior of the layout primitives used by normal eepp applications such as ecode,
eterm, eproc, dialogs, settings panels, forms, workspaces, toolbars, and application-specific tools.

For the recommended high-level authoring model and conventions, read
[`ui_authoring.md`](ui_authoring.md) first.

This document intentionally describes **current eepp behavior**, not Android, browser, Qt, or other
framework behavior that may look similar.

HTML formatting contexts such as block, inline, Flexbox, CSS Grid, table layout, floats, and
positioned HTML are a separate system primarily used by `UIWebView`, `UIMarkdownView`, and the HTML
compatibility layer. They are not covered here except where a distinction is important.

---

## 1. Native layout model

The normal eepp layout model consists of:

```text
UIWidget
    size policy
    margins
    padding
    layout gravity
    min/max constraints
    optional layout weight
    optional relative-position relationship

UILayout
    owns or participates in positioning/sizing children

specific layouts
    UILinearLayout
    UIRelativeLayout
    UIGridLayout
    UIFlowLayout
    UISplitter
```

The most important idea is:

> **The parent layout controls the final allocation and position of its children.**

A child supplies a sizing/placement contract through properties such as:

```text
layout_width
layout_height
layout_weight
layout_gravity
margin
min-width
max-width
min-height
max-height
```

The exact meaning of those properties depends on the parent layout.

---

## 2. XML and CSS spelling

eepp accepts aliases, but normal project style distinguishes XML attributes from CSS properties.

Preferred XML spelling:

```xml
<TextInput
    layout_width="0dp"
    layout_height="wrap_content"
    layout_weight="1"
    layout_gravity="center_vertical" />
```

Preferred CSS spelling:

```css
.form-input {
    layout-width: 0dp;
    layout-height: wrap_content;
    layout-weight: 1;
    layout-gravity: center_vertical;
}
```

Common compact aliases:

```text
layout_width    -> lw
layout_height   -> lh
layout_weight   -> lw8
layout_gravity  -> lg

match_parent    -> mp
wrap_content    -> wc
```

Both full and compact forms are common in production eepp code.

---

## 3. Default widget layout state

A newly constructed `UIWidget` starts with:

```text
layout weight   = 0
layout gravity  = unset / 0
width policy    = WrapContent
height policy   = WrapContent
position policy = None
```

Specific widgets may override their own defaults.

Do not assume every widget behaves identically under `wrap_content`; intrinsic sizing is
widget-specific.

---

## 4. Size policies

The native size-policy enum is:

```cpp
enum class SizePolicy {
    Fixed,
    MatchParent,
    WrapContent
};
```

These policies are applied independently to width and height.

### 4.1 `wrap_content`

XML:

```xml
layout_width="wrap_content"
layout_height="wrap_content"
```

Compact:

```xml
lw="wc"
lh="wc"
```

A `WrapContent` axis asks the widget to derive its size from its intrinsic/content size.

The exact intrinsic size is widget-specific.

Examples:

- text widgets derive size from their text and font metrics;
- image widgets derive size from the drawable;
- buttons derive size from their internal content/theme;
- native layouts may derive their wrapped axis from their visible children.

Padding contributes to normal content sizing where the widget implementation defines it.

`wrap_content` is the default native size policy for a plain `UIWidget`.

### 4.2 `match_parent`

XML:

```xml
layout_width="match_parent"
layout_height="match_parent"
```

Compact:

```xml
lw="mp"
lh="mp"
```

`MatchParent` requests the available parent content size on that axis.

The generic match-parent calculation is:

```text
parent size
- parent content offsets
- non-auto child margins
```

Parent content offsets include padding and, where applicable, borders.

`max-width` / `max-height` can cap the resulting match-parent size.

Conceptually:

```text
child match-parent width =
    parent width
    - parent left/right content offsets
    - child left/right margins
```

The exact layout can perform additional sizing rules around this value.

Do not treat `match_parent` as a synonym for CSS `width: 100%`. They may often produce similar
geometry, but they belong to different layout systems.

### 4.3 Fixed size

Any explicit dimension normally selects `Fixed` sizing on that axis:

```xml
layout_width="240dp"
layout_height="32dp"
```

The value is resolved and stored as the widget size.

The literal value:

```text
fixed
```

can also select the fixed policy without supplying a new dimension.

Use explicit sizes where the dimension itself is meaningful. Do not use arbitrary fixed dimensions
to compensate for a parent-layout mistake.

### 4.4 Zero size and weighted LinearLayout children

There is one important special case.

Inside a `UILinearLayout`, the canonical weighted-child pattern intentionally uses zero on the
weighted axis:

Horizontal parent:

```xml
<TextInput
    layout_width="0dp"
    layout_weight="1" />
```

Vertical parent:

```xml
<ListView
    layout_height="0dp"
    layout_weight="1" />
```

When eepp parses a zero layout dimension for a weighted child whose parent is a `UILinearLayout`, it
does not immediately force the stored size in the normal fixed-size path. The linear layout assigns
the weighted size during packing.

---

## 5. Minimum and maximum constraints

Widgets support:

```text
min-width
max-width
min-height
max-height
```

These can constrain native layout results.

For example, generic `match_parent` width calculation applies a `max-width` cap when one exists.

Some native layout algorithms also explicitly fit wrapped container sizes against min/max
constraints.

Do not assume min/max handling is identical to browser intrinsic-sizing rules. This document
describes the native layout system; HTML layout has additional rules.

---

## 6. Margin and padding

### Margin

Margins belong to the child and participate in parent layout allocation/positioning.

Example:

```xml
<PushButton margin-left="8dp" />
```

Margins affect:

- linear packing;
- match-parent available size;
- relative positioning;
- flow wrapping;
- grid placement;
- splitter/container alignment where relevant.

### Padding

Padding belongs to the container/widget.

Example:

```xml
<vbox padding="12dp">
    ...
</vbox>
```

Native layouts generally place children inside the padded content area.

Conceptually:

```text
container border box
    padding
        child layout area
```

---

## 7. `gravity` and `layout_gravity`

These are different properties.

### `gravity`

`gravity` controls the widget's own content or, for layouts that explicitly use it, the layout's
internal arrangement.

Examples include:

```text
left
right
center_horizontal
top
bottom
center_vertical
center
```

Widget behavior varies because different widget types own different content.

### `layout_gravity`

`layout_gravity` describes how the widget should be placed by its parent layout when that layout
supports gravity.

XML:

```xml
layout_gravity="right|center_vertical"
```

CSS:

```css
layout-gravity: right|center_vertical;
```

The distinction is:

```text
gravity
    content/layout behavior inside this widget

layout_gravity
    placement of this widget by its parent
```

This distinction is especially important in `UILinearLayout`.

---

## 8. `gravity-owner`

Layouts normally respect the positioning policy of their parent.

Some parent widgets advertise that they own child positioning with `UI_OWNS_CHILDREN_POSITION`.

A layout can set:

```text
gravity-owner: true
```

to force its own `layout_gravity` alignment against the parent even when the parent normally owns
child positions.

This is an advanced escape hatch and should not be a default solution to layout problems.

---

## 9. Layout invalidation and update behavior

Native layouts are invalidated when relevant state changes, including:

- child count changes;
- layout-affecting child attributes;
- layout size changes;
- padding changes;
- parent-size changes.

A dirty layout is normally scheduled through the owning `UISceneNode`.

During an active layout pass, some updates can occur synchronously to avoid waiting for another
frame.

Calling `UILayout::getSize()` on a dirty layout forces that layout to update before returning its
size.

Nested layouts are tracked by their parent `UILayout` and participate in layout-tree updates.

This matters mainly when implementing custom layout widgets. Ordinary application code should set
properties and allow the scene to perform layout.

---

## 10. Visibility and native layouts

Native layouts generally operate on **visible widget children**.

Examples:

- `UILinearLayout` ignores invisible children while packing;
- `UIFlowLayout` ignores invisible children while creating rows;
- `UIGridLayout` ignores invisible children;
- invisible native layouts may collapse their own layout size to zero during their update path.

Do not use invisible children as spacers.

---

## 11. `UILinearLayout`

`UILinearLayout` is the primary sequential application layout.

XML aliases:

```xml
<vbox>
<hbox>
```

C++:

```cpp
UILinearLayout::NewVertical();
UILinearLayout::NewHorizontal();
```

Default orientation:

```text
vertical
```

### 11.1 Vertical linear layout

A vertical layout packs visible children from top to bottom.

Each child's vertical margins contribute to the consumed main-axis space.

The child's `layout_gravity` controls **horizontal** placement inside the layout:

```text
left
center_horizontal
right
```

Example:

```xml
<vbox
    layout_width="match_parent"
    layout_height="match_parent"
    padding="12dp">

    <TextView
        text="Left"
        layout_gravity="left" />

    <PushButton
        text="Centered"
        layout_gravity="center_horizontal" />

    <PushButton
        text="Right"
        layout_gravity="right" />
</vbox>
```

### 11.2 Horizontal linear layout

A horizontal layout packs visible children from left to right.

Each child's horizontal margins contribute to consumed main-axis space.

The child's `layout_gravity` controls **vertical** placement:

```text
top
center_vertical
bottom
```

Example:

```xml
<hbox
    layout_width="match_parent"
    layout_height="40dp">

    <TextView
        text="Name"
        layout_gravity="center_vertical" />

    <TextInput
        layout_width="0dp"
        layout_weight="1"
        layout_gravity="center_vertical" />
</hbox>
```

---

## 12. LinearLayout size-policy behavior

Before packing, the layout applies child policies on the cross axis.

### Vertical layout

For child width:

- `WrapContent`: child auto-sizing is enabled;
- `MatchParent`: width becomes the layout's content width minus child horizontal margins;
- `Fixed`: existing width remains.

A child with `layout_height="match_parent"` and zero weight can also be resized to the parent's
available height.

### Horizontal layout

For child height:

- `WrapContent`: child auto-sizing is enabled;
- `MatchParent`: height becomes the layout's content height minus child vertical margins;
- `Fixed`: existing height remains.

A child with `layout_width="match_parent"` and zero weight can also be resized to the parent's
available width.

---

## 13. LinearLayout weights

`layout_weight` distributes remaining space along the layout orientation.

Canonical horizontal use:

```xml
<hbox lw="mp" lh="wc">
    <TextView text="Project" min-width="90dp" lg="center_vertical" />
    <TextInput lw="0dp" lw8="1" />
</hbox>
```

Canonical vertical use:

```xml
<vbox lw="mp" lh="mp">
    <TextView text="Header" />
    <ListView lw="mp" lh="0dp" lw8="1" />
    <hbox lw="mp" lh="wc">
        <!-- footer -->
    </hbox>
</vbox>
```

### 13.1 Weight calculation

The implementation treats positive weights as **relative weights**.

For all visible children:

```text
totalWeight = sum(max(childWeight, 0))
```

The layout first calculates the main-axis space already consumed by:

- visible non-weighted fixed/wrapped children;
- visible child margins.

Then each positively weighted child gets:

```text
remainingSpace * childWeight / totalWeight
```

Therefore these are equivalent ratios:

```text
1, 1
0.5, 0.5
50, 50
```

and:

```text
1, 3
25, 75
```

produce equivalent proportions.

> Current implementation does **not** require weights to be normalized to `0..1` or sum to `1`.

Older documentation that describes weights as normalized should be considered overly restrictive.

### 13.2 Example calculation

Container:

```text
horizontal content width = 600dp
```

Children:

```text
A = fixed 100dp
B = weight 1
C = weight 3
```

Ignoring margins:

```text
remaining = 600 - 100 = 500
totalWeight = 4

B = 125
C = 375
```

---

## 14. LinearLayout wrap-content behavior

A vertical `UILinearLayout` with `layout_height="wrap_content"` grows to the visible child heights,
vertical margins, and padding.

A horizontal `UILinearLayout` with `layout_width="wrap_content"` similarly grows from visible child
widths, margins, and padding.

The cross-axis wrapped size is based on the maximum visible non-match-parent child extent plus
container padding, with min/max constraints considered.

A wrapped linear layout can repack after its cross-axis size changes because that size may affect
child alignment or nested layout.

---

## 15. LinearLayout properties

Layout-specific:

```text
orientation
gravity-owner
```

`orientation` values:

```text
vertical
horizontal
```

Default:

```text
vertical
```

Child properties commonly used with a linear layout:

```text
layout_width
layout_height
layout_weight
layout_gravity
margin
min-width
max-width
min-height
max-height
```

---

## 16. `UIRelativeLayout`

`UIRelativeLayout` is a native layout for:

- alignment against the parent;
- positioning one child relative to a sibling.

It owns child positioning.

Use it when sibling relationships are more natural than a nested `vbox`/`hbox` hierarchy.

Do not choose it merely because it permits more arbitrary positioning.

---

## 17. RelativeLayout child sizing

For each widget child:

Width:

```text
WrapContent  -> enable auto-sizing
MatchParent  -> layout width - child horizontal margins - layout horizontal padding
Fixed        -> keep existing width
```

Height:

```text
WrapContent  -> enable auto-sizing
MatchParent  -> layout height - child vertical margins - layout vertical padding
Fixed        -> keep existing height
```

---

## 18. RelativeLayout parent alignment

When a child has no sibling-relative position policy, `layout_gravity` positions it against the
`UIRelativeLayout`.

Horizontal:

```text
left
center_horizontal
right
```

Vertical:

```text
top
center_vertical
bottom
```

Example:

```xml
<RelativeLayout
    layout_width="match_parent"
    layout_height="match_parent"
    padding="8dp">

    <PushButton
        text="Bottom right"
        layout_gravity="right|bottom" />
</RelativeLayout>
```

---

## 19. RelativeLayout sibling relationships

Supported relationships are:

```text
layout_to_left_of
layout_to_right_of
layout_to_top_of
layout_to_bottom_of
```

The value is the target sibling ID.

Example:

```xml
<RelativeLayout
    layout_width="match_parent"
    layout_height="match_parent">

    <Widget
        id="sidebar"
        layout_width="220dp"
        layout_height="match_parent" />

    <Widget
        id="content"
        layout_width="match_parent"
        layout_height="match_parent"
        layout_to_right_of="sidebar" />
</RelativeLayout>
```

The relationship is resolved by finding the referenced node and storing the target widget.

It is used only when target and positioned widget share the same parent.

Only one stored `PositionPolicy` is active at a time.

This is not a general constraint solver.

---

## 20. RelativeLayout wrap-content caveat

`UIRelativeLayout::updateLayout()` applies its own size policy and lays out children, but it does
**not** compute a new bounding rectangle from all positioned children.

For application containers, prefer a defined or `match_parent` size when using `UIRelativeLayout`.

Do not assume `wrap_content` behaves like a general constraint-layout "measure descendants and fit"
pass.

---

## 21. `UIGridLayout`

`UIGridLayout` is eepp's **native repeated-cell grid**.

It is not CSS Grid.

Use it for normal application UI when children should occupy repeated cells with a common row/column
size policy.

Do not infer CSS track sizing behavior from this class.

---

## 22. GridLayout sizing model

Grid cells use one column mode and one row mode.

```text
column-mode:
    size
    weight

row-mode:
    size
    weight
```

Defaults:

```text
column-mode   = weight
row-mode      = weight
column-weight = 0.25
row-weight    = 0.25
column-width  = 0
row-height    = 0
column-margin = 0
row-margin    = 0
```

When `column-mode=size`, cell width is `column-width`.

When `column-mode=weight`, cell width is approximately:

```text
available grid width * column-weight
```

When `row-mode=size`, cell height is `row-height`.

When `row-mode=weight`, cell height is approximately:

```text
available grid height * row-weight
```

---

## 23. GridLayout placement

Visible widget children are processed sequentially.

For each child:

1. determine target cell size;
2. set child size policy to `Fixed, Fixed`;
3. resize child to target size;
4. place child at current grid position;
5. advance by cell width plus column margin;
6. wrap to the next row when the next cell no longer fits.

This is a repeated-cell wrapping grid, not a named-track CSS grid.

---

## 24. GridLayout child weight override

`UIGridLayout` has a special behavior:

If a child has non-zero `layout_weight`, that child's target **width** becomes:

```text
child layout weight
* grid content width
```

This is not `UILinearLayout`'s remaining-space distribution algorithm.

Do not assume `layout_weight` means the same algorithm in every layout type.

For straightforward grids, prefer the layout-level row/column sizing properties unless this
per-child width override is intentionally needed.

---

## 25. GridLayout alignment

The grid's own horizontal `gravity` affects row placement:

```text
left
center
right
```

This is container `gravity`, not child `layout_gravity`.

---

## 26. GridLayout wrap-content behavior

With:

```text
layout_height="wrap_content"
```

the layout derives its height from generated rows plus vertical padding.

A match-parent grid width/height is resolved from the parent before cell placement.

Children are converted to fixed cell sizes during grid layout.

---

## 27. GridLayout properties

```text
column-mode
column-weight
column-width
column-margin

row-mode
row-weight
row-height
row-margin
```

Container `gravity` also affects horizontal grid alignment.

---

## 28. `UIFlowLayout`

`UIFlowLayout` is eepp's native horizontal wrapping flow layout.

Conceptually:

```text
A B C D
E F G
H I
```

Children are consumed in order from left to right.

When the next visible child no longer fits the current row, a new row is created.

---

## 29. FlowLayout width behavior

A notable design choice:

When the flow layout itself uses:

```text
layout_width="wrap_content"
```

it initially uses the available match-parent width as its wrapping width.

After layout, if all visible children effectively fit on one row and use less than the available
width, the layout can shrink its wrapped width to the used width.

So `wrap_content` for `UIFlowLayout` is not simply "sum all child widths without a constraint."
Available parent width participates in wrapping first.

---

## 30. FlowLayout row creation

For each visible child:

1. apply the child's size policy;
2. test whether the child would exceed the current row;
3. if necessary, create a new row;
4. place the child;
5. include child margins in row consumption;
6. track the row's maximum height.

The implementation can also create a new row after placement when the resulting cursor exceeds the
layout width.

---

## 31. FlowLayout child size policies

Before rows are built:

```text
WrapContent width/height
    enable child auto-sizing

MatchParent width
    available match-parent width

MatchParent height
    available match-parent height

Fixed
    retain explicit size
```

After row heights are known, a child with:

```text
layout_height="match_parent"
```

is resized to that row's maximum height.

---

## 32. FlowLayout alignment

The flow layout's own `gravity` controls row-group placement.

Horizontal gravity:

```text
left
center
right
```

controls each row's horizontal displacement.

Vertical gravity:

```text
top
center_vertical
bottom
```

can shift the complete row group inside a container taller than the total flow content.

Within each row, `row-valign` controls child vertical alignment.

---

## 33. `row-valign`

Values:

```text
top
center
bottom
```

Default:

```text
bottom
```

Meaning:

```text
top
    child aligns near row top, respecting top margin

center
    child is centered in row maximum height

bottom
    child aligns near row bottom, respecting bottom margin
```

This is separate from the container's vertical `gravity`.

---

## 34. FlowLayout wrap-content height

With:

```text
layout_height="wrap_content"
```

height becomes:

```text
top padding
+ sum(row maximum heights)
+ bottom padding
```

with child vertical margins contributing to row heights.

---

## 35. FlowLayout properties

Layout-specific:

```text
row-valign
gravity-owner
```

Important general properties:

```text
gravity
layout_width
layout_height
padding
margin
```

`UIFlowLayout` does not use child `layout_weight` as a remaining-space distribution mechanism.

---

## 36. `UISplitter`

`UISplitter` is a two-pane resizable layout.

Default orientation:

```text
horizontal
```

Horizontal:

```text
[first pane] | [second pane]
```

Vertical:

```text
[first pane]
-------------
[second pane]
```

The separator is an internal draggable widget.

---

## 37. Splitter child model

`UISplitter` supports at most two external widget children.

Internally it also owns its separator widget.

The first external child becomes the first pane; the second becomes the last pane.

Additional external children are rejected/closed.

Children placed into the splitter are forced to:

```text
Fixed, Fixed
```

because the splitter owns their exact geometry.

---

## 38. Splitter with one child

If only one pane exists:

- the separator is hidden and disabled;
- the first child fills the splitter's padded content area.

---

## 39. Split partition

Property:

```text
splitter-partition
```

Default:

```text
50%
```

Meaning:

> Desired space occupied by the first pane along the splitter axis.

The separator's own size is removed from available split space when visible.

Conceptually:

```text
totalSpace =
    splitter content axis size
    - visible separator size

first = resolve(splitter-partition, totalSpace)
second = totalSpace - first
```

---

## 40. Splitter minimum sizes

The first pane's minimum size constrains the initial partition.

During dragging, both pane minimum sizes constrain separator movement.

A requested partition may therefore not be achievable when it violates minimum sizes.

Do not assume `0%` always produces a physically zero-sized first pane if its minimum size is non-zero.

---

## 41. Splitter visibility properties

```text
splitter-always-show
```

Default:

```text
true
```

When true, the divider remains visible whenever two panes exist.

```text
splitter-hide-on-edge
```

Default:

```text
false
```

When enabled and `splitter-always-show` is false, percentage partitions exactly at `0%` or `100%`
can hide the divider so one pane consumes the full splitter.

---

## 42. Splitter dragging

Horizontal orientation:

```text
separator drags along x
cursor = horizontal resize
```

Vertical orientation:

```text
separator drags along y
cursor = vertical resize
```

Dragging immediately resizes both children.

After a drag, the stored split partition is recalculated as a percentage of available split space.

---

## 43. Splitter properties

```text
orientation
splitter-partition
splitter-always-show
splitter-hide-on-edge
```

Orientation values:

```text
horizontal
vertical
```

---

## 44. Choosing the correct native layout

| Need | Native layout |
| --- | --- |
| Vertical sequence | `UILinearLayout` / `<vbox>` |
| Horizontal sequence | `UILinearLayout` / `<hbox>` |
| Flexible remaining space in a row/column | `UILinearLayout` + `layout_weight` |
| Child positioned relative to a sibling | `UIRelativeLayout` |
| Repeated equal/weighted cells | `UIGridLayout` |
| Horizontal items that wrap into rows | `UIFlowLayout` |
| Two user-resizable panes | `UISplitter` |
| Tabs | `UITabWidget` |
| Split/tab editor workspace | `UITabWidgetSplitter` |
| Web/document formatting | HTML compatibility layout |

`UITabWidget` and `UITabWidgetSplitter` are higher-level containers rather than low-level generic
layout algorithms and belong in the widget/application references.

---

## 45. Production patterns

### Flexible form row

```xml
<hbox
    layout_width="match_parent"
    layout_height="wrap_content"
    margin-bottom="8dp">

    <TextView
        text="Project"
        min-width="90dp"
        layout_gravity="center_vertical" />

    <TextInput
        id="project"
        layout_width="0dp"
        layout_weight="1" />
</hbox>
```

### Header + flexible content + footer

```xml
<vbox
    layout_width="match_parent"
    layout_height="match_parent">

    <hbox
        layout_width="match_parent"
        layout_height="wrap_content">
        <!-- toolbar -->
    </hbox>

    <SomeContent
        layout_width="match_parent"
        layout_height="0dp"
        layout_weight="1" />

    <hbox
        layout_width="match_parent"
        layout_height="wrap_content">
        <!-- footer -->
    </hbox>
</vbox>
```

### Wrapping option group

```xml
<FlowLayout
    layout_width="match_parent"
    layout_height="wrap_content"
    row-valign="center">

    <CheckBox text="Linux" margin-right="8dp" />
    <CheckBox text="macOS" margin-right="8dp" />
    <CheckBox text="Windows" margin-right="8dp" />
    <CheckBox text="FreeBSD" margin-right="8dp" />
</FlowLayout>
```

### Two-pane workspace

```xml
<Splitter
    layout_width="match_parent"
    layout_height="match_parent"
    orientation="horizontal"
    splitter-partition="30%">

    <TreeView />
    <CodeEditor />
</Splitter>
```

---

## 46. Important differences from browser layout

Do not import these as semantic equivalences:

```text
match_parent == width:100%
layout_weight == flex-grow
UIGridLayout == CSS Grid
UIFlowLayout == display:flex + flex-wrap
layout_gravity == justify-content / align-items
```

They can be useful analogies, but native eepp layouts are separate algorithms.

---

## 47. Important differences from Android intuition

eepp deliberately borrows several Android-style concepts, especially:

```text
LinearLayout
layout_width
layout_height
match_parent
wrap_content
layout_weight
layout_gravity
RelativeLayout-style sibling relations
dp
```

Android documentation is not authoritative for eepp.

Notable example:

> Current eepp `UILinearLayout` weights are arbitrary relative positive values; they do not need to
> be normalized or sum to `1`.

Always prefer eepp behavior over external analogy.

---

## 48. Inspecting native layout at runtime

Use the **Runtime UI Inspector & Automation Protocol** rather than guessing geometry.

Typical process:

```text
ui.contexts
    ↓
ui.query target
    ↓
ui.inspect target geometry/layout properties
    ↓
inspect parent
    ↓
inspect relevant siblings
    ↓
screenshot if useful
```

Stable debugging questions:

```text
What parent layout owns this child?
What are its width/height policies?
What are its actual bounds?
Does it have weight?
What are its margins?
What is the parent's padding?
What gravity is active?
What min/max constraints exist?
```

For weighted linear layouts, inspect all siblings participating in the same axis.

---

## 49. Layout debugging checklist

When size is wrong:

```text
1. identify parent layout type
2. inspect layout_width / layout_height
3. determine Fixed / MatchParent / WrapContent
4. check layout_weight
5. check margins
6. check parent padding/content offsets
7. check min/max constraints
8. check sibling sizes/weights
9. check visibility
10. only then consider an explicit fixed size
```

When position is wrong:

```text
1. identify parent layout
2. check layout_gravity
3. distinguish layout_gravity from gravity
4. check margins
5. check RelativeLayout position relationship
6. check parent gravity where Grid/Flow uses it
7. check gravity-owner only if parent positioning ownership matters
```

---

## 50. Implementation-oriented notes

These details matter mainly when implementing custom layouts.

### Layout dirtiness

`UILayout` maintains dirty state and participates in the scene's layout invalidation queue.

### Reentrancy guard

Layouts use `mPacking` to prevent recursive packing.

### Nested layouts

A `UILayout` tracks direct child layouts and updates the layout tree recursively.

### Auto-size children

A layout can request `onAutoSize()` for ordinary children or temporarily update a child layout as
wrapping content.

### Parent notifications

When a layout's wrapped size changes, implementations notify the parent so enclosing layouts can
repack.

Custom layout implementations should preserve these lifecycle expectations.

---

## 51. Scope boundaries

This document intentionally does not specify:

- HTML Flexbox;
- HTML CSS Grid;
- block/inline formatting;
- HTML table layout;
- floats;
- sticky/fixed/absolute HTML positioning;
- HTML intrinsic min-content/max-content behavior.

See [`ui_html_compatibility.md`](ui_html_compatibility.md) for HTML/web layout semantics.

For related topics:

- [`ui_introduction.md`](ui_introduction.md) — introduction to eepp's UI system.
- [`ui_authoring.md`](ui_authoring.md) — recommended application UI mental model and authoring conventions.
- [`ui_css_for_applications.md`](ui_css_for_applications.md) — CSS behavior and conventions for normal application widgets.
- [`ui_inspector.md`](ui_inspector.md) — runtime UI inspection and automation protocol.
- [`ui_databinding.md`](ui_databinding.md) — UI data binding.
- [`ui_charts.md`](ui_charts.md) — charting widgets and chart-specific behavior.

---

## 52. Source of truth

When uncertain about native layout behavior, use this order:

1. current eepp implementation
2. current eepp unit/integration tests
3. production eepp/ecode/eterm/eproc usage
4. this reference
5. [`css_specification.md`](css_specification.md) and other general docs
6. Android/browser/other-framework analogy


If a documented rule and the current implementation disagree, treat the implementation as
authoritative and update the documentation.
