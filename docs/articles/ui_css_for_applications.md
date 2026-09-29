# CSS for Application UI

eepp uses CSS as a first-class part of its native application UI system.

This document explains how CSS should be used when styling **normal eepp application widgets**:
windows, toolbars, dialogs, editors, settings panels, lists, trees, application shells, and other
interfaces built from native eepp widgets and layouts.

It is not an exhaustive property reference. See
[`css_specification.md`](css_specification.md) for the supported selector, property, value, at-rule,
animation, and data-type reference.

For the overall UI authoring model, see [`ui_authoring.md`](ui_authoring.md). For exact native
layout behavior, see [`ui_layout_reference.md`](ui_layout_reference.md).

HTML formatting contexts are a separate concern. CSS used by `UIWebView`, `UIMarkdownView`, and the
HTML compatibility layer can use web layout concepts that should not be assumed for ordinary
application UI. See [`ui_html_compatibility.md`](ui_html_compatibility.md).

---

## 1. CSS is not only visual styling

In native eepp UI, CSS can configure both appearance and widget/layout behavior.

Typical visual properties include:

```css
color: var(--font);
background-color: var(--back);
border-color: var(--tab-line);
border-width: 1dp;
border-radius: 4dp;
font-size: 12dp;
tint: var(--icon);
opacity: 1;
```

But CSS can also set native eepp layout properties:

```css
layout-width: match_parent;
layout-height: wrap_content;
layout-weight: 1;
layout-gravity: center_vertical;
orientation: horizontal;
padding: 8dp;
margin-right: 4dp;
min-width: 120dp;
max-width: 600dp;
```

It can also configure widget-specific behavior:

```css
tab-closable: true;
row-height: 22dp;
vscroll-mode: auto;
word-wrap: true;
menu-width-mode: expand-if-needed;
```

The correct mental model is therefore:

```text
XML
    declares widget hierarchy and instance configuration

CSS
    participates in styling, sizing, layout configuration, state styling, and widget configuration

C++
    supplies behavior, dynamic content, models, commands, and application logic
```

CSS is not a separate browser-like layout system for native application widgets.

---

## 2. Use native layout properties for application layout

For ordinary application UI, the primary sizing properties are:

```css
layout-width
layout-height
layout-weight
layout-gravity
```

Example:

```css
.toolbar {
    layout-width: match_parent;
    layout-height: wrap_content;
}

.toolbar .search {
    layout-width: 0dp;
    layout-height: wrap_content;
    layout-weight: 1;
}

.toolbar .action {
    layout-width: wrap_content;
    layout-height: wrap_content;
    layout-gravity: center_vertical;
}
```

These properties participate directly in the native parent layout algorithm.

Their exact behavior depends on the parent layout and is documented in
[`ui_layout_reference.md`](ui_layout_reference.md).

Do not translate native eepp layout into browser Flexbox/Grid terminology.

For example:

```text
layout-weight
```

is a `UILinearLayout` concept. It is not `flex-grow`.

Similarly:

```text
UIFlowLayout
```

is a native wrapping layout. It is not `display: flex`.

---

## 3. Prefer canonical CSS spelling

In CSS, use hyphenated property names:

```css
layout-width: match_parent;
layout-height: wrap_content;
layout-weight: 1;
layout-gravity: center;
margin-left: 4dp;
row-valign: center;
```

eepp accepts aliases such as:

```text
layout_width
layout_height
layout_weight
layout_gravity
```

and compact aliases such as:

```text
lw
lh
lw8
lg
```

but those are primarily convenient in XML.

The documentation convention is:

```text
XML attributes    -> underscores or compact aliases
CSS properties    -> hyphens
```

Example:

```xml
<TextInput
    class="search_input"
    layout_width="0dp"
    layout_height="wrap_content"
    layout_weight="1" />
```

```css
.search_input {
    margin-left: 4dp;
    min-width: 120dp;
}
```

---

## 4. `layout-width` / `layout-height` versus `width` / `height`

These property pairs are related but should not be treated as interchangeable when authoring native
application UI.

### `layout-width` and `layout-height`

These express native size policies and can use:

```text
match_parent
wrap_content
fixed dimensions
```

Example:

```css
.sidebar {
    layout-width: 220dp;
    layout-height: match_parent;
}

.content {
    layout-width: 0dp;
    layout-height: match_parent;
    layout-weight: 1;
}
```

They are the preferred properties for native layout composition.

### `width` and `height`

On a normal `UIWidget`, a concrete `width` or `height` sets the corresponding native size policy to
`Fixed` and updates the widget size.

For example:

```css
.icon {
    width: 16dp;
    height: 16dp;
}
```

effectively establishes fixed native dimensions.

The value:

```css
width: auto;
height: auto;
```

maps the corresponding axis back to `WrapContent`.

Internally, applying concrete `width` / `height` also synchronizes the corresponding native
`layout-width` / `layout-height` value.

### Recommendation

For application layout, prefer:

```css
layout-width
layout-height
```

because they express the actual native layout contract.

Using `width` / `height` is reasonable for intrinsically fixed visual components:

```css
.close_icon {
    width: 12dp;
    height: 12dp;
}
```

Avoid specifying both forms for the same axis:

```css
/* Avoid */
.panel {
    width: 300dp;
    layout-width: match_parent;
}
```

That creates competing declarations with different intent.

---

## 5. XML attributes have inline specificity

For normal eepp widgets, XML attributes are loaded as style properties with inline specificity.

This means:

```xml
<TextInput
    class="search"
    layout_width="200dp" />
```

combined with:

```css
.search {
    layout-width: 300dp;
}
```

will normally retain the XML value.

The XML declaration is effectively an inline property for cascade purposes.

The same applies to ordinary XML properties such as:

```xml
font_size="12dp"
background_color="#202020"
margin_left="4dp"
visible="true"
```

when the property maps to the CSS property system.

### Authoring rule

Do not define the same configurable property in XML and CSS unless the precedence is intentional.

Prefer one owner:

```text
instance-specific structural value
    -> XML

shared/reusable styling or layout rule
    -> CSS
```

For example, this is good:

```xml
<PushButton
    id="save"
    class="dialog_action"
    text="Save" />
```

```css
.dialog_action {
    layout-height: 24dp;
    padding-left: 8dp;
    padding-right: 8dp;
}
```

because XML defines identity/content and CSS defines the reusable presentation.

### `style=""`

The XML `style` attribute also has inline specificity:

```xml
<PushButton style="background-color: red; padding: 4dp;" />
```

Use it sparingly. A class is usually easier to reuse, inspect, and maintain.

### `!important`

`!important` has higher specificity than inline properties and can therefore override them.

That is supported, but it should not be the normal way to fight XML declarations. If a stylesheet
is expected to control a property, prefer removing the competing XML value.

---

## 6. Classes are the primary reusable styling mechanism

Use classes for reusable application semantics:

```xml
<PushButton class="toolbar_button" />
<PushButton class="toolbar_button primary" />
<TextInput class="search_input error" />
```

```css
.toolbar_button {
    layout-height: 22dp;
    padding: 2dp 6dp;
}

.toolbar_button.primary {
    background-color: var(--primary);
}

.search_input.error {
    border-color: var(--theme-error);
}
```

Classes work well for both stable styling and application state.

Changing classes from C++ invalidates the widget's style and causes matching rules to be
re-evaluated.

Example:

```cpp
input->addClass( "error" );
input->removeClass( "error" );
```

Prefer semantic classes such as:

```text
error
compact
primary
warning
dragging
running
muted
```

over setting several individual style properties from C++.

Use direct C++ styling when the value is genuinely dynamic data rather than a reusable state.

---

## 7. Use IDs for unique component structure

IDs are appropriate for unique widgets:

```xml
<TextInput id="settings_filter" />
```

```css
#settings_filter {
    margin-bottom: 10dp;
}
```

Use classes when multiple widgets should share the same rule:

```css
.settings_option {
    padding: 8dp;
}
```

A useful convention is:

```text
#id
    unique component instance / structural role

.class
    reusable visual or semantic role
```

Avoid creating many unique ID rules when a class describes the actual shared concept.

---

## 8. Type and hierarchy selectors are useful for component styling

eepp supports normal selector composition such as:

```css
PushButton {
    ...
}

.settings_panel TextView {
    ...
}

.settings_panel > .header {
    ...
}

.status_bar PushButton:hover {
    ...
}
```

This makes it possible to scope component rules without inventing a class for every internal widget.

Example:

```css
.settings_panel .settings_option {
    border-bottom: 1dp solid var(--tab-line);
}

.settings_panel .settings_option > .settings_option_content {
    layout-width: match_parent;
}
```

Use the narrowest selector that expresses the component relationship clearly.

For the exhaustive selector support, including structural selectors and combinators, see
[`css_specification.md`](css_specification.md).

---

## 9. Native pseudo-classes should represent native interaction state

Use pseudo-classes for widget state managed by the UI system:

```css
.toolbar_button:hover {
    background-color: var(--button-hover);
}

.toolbar_button:pressed {
    background-color: var(--button-press);
}

.search_input:focus {
    border-color: var(--primary);
}

.action:disabled {
    opacity: 0.5;
}

.panel:focus-within {
    border-color: var(--primary);
}
```

State changes cause eepp to recompute the matching style. When a state-dependent declaration stops
matching, a lower local or stylesheet declaration takes effect again. If there is no lower
declaration, eepp restores the current inherited value or the widget's saved native value. Native
rollback requires `getPropertyString()` to serialize the property. Changes made by application code
to a property while a state rule overrides it have no defined rollback behavior.

Common native state pseudo-classes include:

```text
:hover
:focus
:focus-within
:selected
:pressed
:active
:disabled
:enabled
```

Structural pseudo-classes such as `:first-child`, `:last-child`, `:nth-child()`, and related forms
are also supported.

Use a class for application state that is not represented by a native pseudo-class:

```css
.build_status.running {
    ...
}

.input.error {
    ...
}
```

Do not emulate `:hover`, `:focus`, or `:disabled` by manually adding classes.

---

## 10. Inheritance exists for a defined property set

The current style system supports inheritance for properties explicitly registered as inheritable.

The native inherited property set currently includes:

```text
cursor

color
font-family
font-size
font-style
font-weight

text-transform
text-shadow-color
text-shadow-offset
text-decoration
text-align
text-indent
text-stroke-width
text-stroke-color
text-selection

line-spacing
line-height
tab-size

white-space
white-space-collapse

list-style-type
list-style-position
```

A child without a local value for an inheritable property can receive the nearest ancestor's value.

Example:

```css
.settings_panel {
    color: var(--font);
    font-size: 11dp;
}

.settings_panel .title {
    font-size: 18dp;
}
```

Most descendants inherit the panel color/font size, while `.title` overrides its own font size.

The explicit keyword is also supported for non-indexed inheritable properties:

```css
.special_label {
    color: inherit;
}
```

### Do not assume every property inherits

Properties such as:

```text
background-color
padding
margin
layout-width
layout-height
layout-weight
layout-gravity
```

are not inherited merely because they are set on a parent.

Inheritance is property-specific.

---

## 11. Custom properties are the preferred theming mechanism

CSS custom properties can be used with `var()`:

```css
.my_panel {
    --panel-accent: #4da3ff;
}

.my_panel .title {
    color: var(--panel-accent);
}
```

Variables are resolved through the widget style and can fall back through the parent widget tree.

This makes scoped component theming possible:

```css
.warning_panel {
    --panel-accent: var(--theme-warning);
}

.error_panel {
    --panel-accent: var(--theme-error);
}

.panel_title {
    color: var(--panel-accent);
}
```

Application styles should prefer semantic theme variables where available:

```css
color: var(--font);
background-color: var(--back);
border-color: var(--tab-line);
tint: var(--icon);
background-color: var(--primary);
```

rather than duplicating literal colors throughout application CSS.

---

## 12. `light-dark()` and color-scheme styling

eepp resolves `light-dark()` according to the scene color-scheme preference.

Example:

```css
.icon {
    tint: light-dark(#303030, #e8e8e8);
}
```

It can be combined with variables:

```css
.button:hover {
    background-color:
        light-dark(var(--scrollbar-hback-hover), var(--primary));
}
```

`@media (prefers-color-scheme: ...)` is also supported:

```css
.description {
    color: var(--disabled-color);
}

@media (prefers-color-scheme: dark) {
    .description {
        color: var(--font-hint);
    }
}
```

Use theme variables first when the theme already exposes the semantic color you need.

Use `light-dark()` or `prefers-color-scheme` when the component genuinely requires a
scheme-dependent distinction.

---

## 13. Use `dp` for application geometry

`dp` is the normal unit for application UI dimensions:

```css
padding: 8dp;
margin-right: 4dp;
font-size: 12dp;
border-width: 1dp;
layout-height: 24dp;
```

This lets the UI scale with eepp's pixel density.

Other supported length units include standard CSS-style units such as:

```text
em
rem
pt
pc
in
cm
mm
vw
vh
vmin
vmax
```

and eepp-specific rounded dp variants:

```text
dpr
dprd
dpru
```

For ordinary application controls, start with `dp`.

Use `em` / `rem` when a dimension should intentionally scale with typography.

Use viewport units only when the desired behavior is genuinely tied to the viewport.

Do not use physical pixels as the default UI design unit.

---

## 14. Prefer native flexible layout over percentage sizing

Although several length properties can resolve percentages, percentages are not a replacement for
native layout policies.

For normal application layout, prefer:

```text
match_parent
wrap_content
layout_weight
Splitter partitions
native GridLayout sizing
FlowLayout wrapping
```

over browser-style percentage composition.

Example:

```css
.content {
    layout-width: 0dp;
    layout-weight: 1;
}
```

usually expresses application intent better than:

```css
.content {
    width: 75%;
}
```

Percentages are appropriate when the specific property or widget is documented to use them, such as
a percentage `splitter-partition`.

---

## 15. `gravity` and `layout-gravity` are different

CSS can configure both:

```css
.button {
    gravity: center;
    layout-gravity: center_vertical;
}
```

The distinction is:

```text
gravity
    alignment/content behavior owned by the widget itself

layout-gravity
    placement of the widget by its parent native layout
```

Do not use `text-align`, `gravity`, and `layout-gravity` interchangeably.

For exact layout behavior, see [`ui_layout_reference.md`](ui_layout_reference.md).

---

## 16. Visibility is not browser `visibility`

Native application UI has:

```css
visible: false;
```

and also accepts:

```css
visibility: hidden;
```

For ordinary `UIWidget`, both ultimately make the widget not visible.

Native layouts generally ignore invisible children while packing.

Therefore:

```css
visibility: hidden;
```

does **not** have the browser meaning of "do not paint this element but preserve its layout box."

It can change surrounding native layout geometry.

This is useful for optional application controls:

```css
.advanced_controls {
    visible: false;
}

.advanced_controls.enabled {
    visible: true;
}
```

but it is important to understand that showing/hiding the widget can cause its parent layout to
repack.

If an invisible-looking element must still reserve space, design that state explicitly rather than
assuming browser `visibility` semantics.

---

## 17. `display` is not the native application visibility mechanism

Properties such as:

```css
display
position
float
clear
flex-*
grid-*
```

belong to the HTML compatibility/layout implementation.

They are not the layout vocabulary for ordinary native application widgets.

For native application UI:

```text
show/hide widget
    -> visible

sequential placement
    -> vbox / hbox

sibling/overlay placement
    -> RelativeLayout

resizable panes
    -> Splitter

wrapping controls
    -> FlowLayout

repeated native cells
    -> UIGridLayout
```

Do not write:

```css
.application_toolbar {
    display: flex;
}
```

for a normal application toolbar.

Use a native `<hbox>` / `UILinearLayout` and style it with CSS.

---

## 18. `position: absolute` is not a native layout escape hatch

Browser positioning properties are part of the HTML compatibility layer.

For normal application widgets, the parent native layout owns child positioning.

When a native application needs overlapping or edge-aligned content, use `UIRelativeLayout`:

```xml
<RelativeLayout
    layout_width="match_parent"
    layout_height="match_parent">

    <CodeEditor
        layout_width="match_parent"
        layout_height="match_parent" />

    <PushButton
        layout_width="wrap_content"
        layout_height="wrap_content"
        layout_gravity="bottom|right" />
</RelativeLayout>
```

Do not convert this into:

```css
position: absolute;
right: 0;
bottom: 0;
```

unless the widget is intentionally inside the HTML formatting system.

Likewise, direct `x` / `y` positioning should not be used to fight a parent native layout.

---

## 19. `overflow` does not create a native scroll container

On an ordinary `UIWidget`, values such as:

```css
overflow: hidden;
overflow: auto;
overflow: scroll;
```

configure clipping behavior.

They do not turn an arbitrary native widget into a browser-style scrolling box.

If the application needs scrolling, use a scrolling widget such as:

```xml
<ScrollView
    layout_width="match_parent"
    layout_height="match_parent">

    <vbox
        layout_width="match_parent"
        layout_height="wrap_content">
        ...
    </vbox>
</ScrollView>
```

This distinction is important:

```text
overflow
    clipping behavior on a normal widget

ScrollView / scrollable widget
    actual native scrolling interaction
```

HTML compatibility layout has its own overflow semantics.

---

## 20. Margins and padding are native layout inputs

CSS margins and padding directly affect native geometry.

Example:

```css
.settings_option {
    padding: 9dp 4dp 11dp 4dp;
    margin-bottom: 0dp;
}

.settings_option_control {
    margin-left: 20dp;
}
```

A child's margin participates in its parent's native layout calculation.

A container's padding defines its internal content region.

This means CSS changes to margin/padding can trigger layout changes; they are not merely paint
properties.

Use margin for separation between sibling widgets and padding for space inside a widget/container.

---

## 21. Prefer `layout-gravity` over `margin: auto` for native alignment

Native widgets support auto margins, but normal application layout is usually clearer when
alignment is expressed through the parent layout and `layout-gravity`.

Prefer:

```css
.dialog_buttons {
    gravity: right;
}

.centered_control {
    layout-gravity: center_horizontal;
}
```

over importing browser layout tricks based on combinations of auto margins.

Use auto margins only when their native behavior is specifically what the component requires.

---

## 22. Background and foreground layers are widget styling tools

Native widgets support background and foreground drawing independent of HTML layout.

Typical properties include:

```css
background-color
background-image
background-tint
background-position-x
background-position-y
background-size

foreground-color
foreground-image
foreground-tint
foreground-position-x
foreground-position-y
foreground-size
```

This is useful for application controls and icons:

```css
.close_button {
    background-color: var(--icon-back-hover);
    foreground-image:
        poly(line, var(--icon-line-hover), "0dp 0dp, 6dp 6dp"),
        poly(line, var(--icon-line-hover), "6dp 0dp, 0dp 6dp");
    foreground-position: 3dp 3dp, 3dp 3dp;
}
```

These are ordinary widget rendering properties. They do not imply browser pseudo-elements or a DOM
paint model.

For exact syntax and resource forms, see [`css_specification.md`](css_specification.md).

---

## 23. Use transitions for visual state changes

CSS transitions work naturally with pseudo-classes and classes:

```css
.icon_button {
    tint: var(--icon);
    background-color: transparent;
    transition: all 0.15s;
}

.icon_button:hover {
    tint: var(--primary);
    background-color: var(--icon-back-hover);
}
```

Changing state causes eepp to resolve the new style, and supported property types can transition to
the new value.

Prefer CSS transitions for presentation changes driven by style state.

Use C++ actions/timers when the animation is application behavior rather than a style transition.

Full transition/animation syntax is documented in
[`css_specification.md`](css_specification.md).

---

## 24. Use media queries for real environment differences

Application CSS can use media queries.

Common useful cases include:

```css
@media (prefers-color-scheme: dark) {
    ...
}
```

and size/density-dependent component adjustments.

eepp also exposes a non-standard `pixel-density` media feature.

Do not use media queries as a substitute for native layout responsiveness.

For example, a toolbar that naturally needs to wrap should normally use `UIFlowLayout`, not a
series of width breakpoints.

Media queries are most useful when the **style itself** should change because of the environment.

See [`css_specification.md`](css_specification.md) for the supported media features.

---

## 25. Component-local `<style>` blocks are valid

A layout can include a style block alongside its XML:

```xml
<style>
<![CDATA[
.my_dialog {
    background-color: var(--back);
}

.my_dialog .title {
    font-size: 16dp;
    font-weight: bold;
}
]]>
</style>

<vbox class="my_dialog">
    <TextView class="title" />
    ...
</vbox>
```

This is useful for reusable/self-contained components whose CSS belongs closely to their structure.

Production eepp code uses this extensively.

For application-wide rules or shared design-system styles, use the scene/application stylesheet
instead of copying the same `<style>` block into multiple components.

A practical division is:

```text
application/theme CSS
    shared controls
    application-wide classes
    common colors and component rules

component-local <style>
    styles private to one reusable layout/component

XML attributes
    instance-specific structural/configuration values
```

---

## 26. Custom properties can make components locally themeable

A component can define semantic variables and let callers override them.

Example:

```css
.notification {
    --notification-accent: var(--primary);
    border-left: 3dp solid var(--notification-accent);
}

.notification.warning {
    --notification-accent: var(--theme-warning);
}

.notification.error {
    --notification-accent: var(--theme-error);
}
```

Internal descendants can reuse the semantic variable:

```css
.notification .icon {
    tint: var(--notification-accent);
}
```

This is preferable to repeating selector-specific colors throughout the component.

---

## 27. Runtime class changes are preferable to runtime property bundles

Suppose a widget has two application states.

Prefer:

```css
.connection {
    color: var(--font-hint);
}

.connection.connected {
    color: var(--theme-success);
}
```

```cpp
connection->setClass( connected ? "connection connected" : "connection" );
```

or:

```cpp
connection->toggleClass( "connected", connected );
```

when available in the calling context.

Avoid code that manually synchronizes several visual properties:

```cpp
// Avoid for reusable visual state
widget->setFontColor( ... );
widget->setBackgroundColor( ... );
widget->setBorderColor( ... );
widget->setAlpha( ... );
```

unless those values are genuinely data-driven and do not represent a reusable style state.

Classes keep the state definition in one place and allow theme changes to continue working.

---

## 28. Stylesheet changes and class changes are reactive

The style system tracks widget state and matching selectors.

Operations such as:

```text
adding/removing/changing classes
changing pseudo-state
updating the scene stylesheet
changing inherited style
changing variables
changing color-scheme-dependent values
```

can cause affected properties to be re-resolved and applied.

Application code should therefore change the semantic input:

```text
class
state
variable
stylesheet
```

rather than assuming CSS is only evaluated once when XML is loaded.

---

## 29. Keep application structure readable in XML

CSS can set layout properties, but that does not mean every structural fact belongs in CSS.

For example, this is often easier to understand:

```xml
<vbox
    id="content"
    layout_width="match_parent"
    layout_height="0dp"
    layout_weight="1">
```

than hiding the entire structural contract in an unrelated stylesheet.

Conversely, repeated visual/layout policy should not be duplicated across many XML nodes:

```xml
<PushButton class="toolbar_button" />
<PushButton class="toolbar_button" />
<PushButton class="toolbar_button" />
```

```css
.toolbar_button {
    layout-height: 22dp;
    padding: 2dp 6dp;
    margin-right: 4dp;
}
```

The useful distinction is not:

```text
XML = layout
CSS = appearance
```

because CSS can configure native layout.

The useful distinction is:

```text
XML
    hierarchy + clear instance-specific structure

CSS
    reusable rules + visual language + state styling + shared layout policy
```

---

## 30. Do not build an HTML mental model around native CSS

The fact that eepp understands CSS syntax does not mean ordinary application widgets form a browser
DOM with browser formatting rules.

For native application UI, avoid assumptions such as:

```text
display:block controls normal widget flow
display:flex is the standard row layout
CSS Grid should replace UIGridLayout
visibility:hidden preserves layout
overflow:auto creates scrolling
position:absolute is the normal overlay mechanism
width:100% is equivalent to match_parent
```

Instead use the native concepts:

```text
UILinearLayout / vbox / hbox
UIRelativeLayout
UIGridLayout
UIFlowLayout
UISplitter
UITabWidget
UITabWidgetSplitter
ScrollView
layout-width / layout-height
layout-weight
layout-gravity
```

CSS configures these native widgets; it does not replace their layout model.

---

## 31. Recommended application CSS style

A typical native component style should look like this:

```css
.search_panel {
    layout-width: match_parent;
    layout-height: wrap_content;
    padding: 4dp;
    background-color: var(--back);
    border-bottom: 1dp solid var(--tab-line);
}

.search_panel .search_input {
    layout-width: 0dp;
    layout-height: 20dp;
    layout-weight: 1;
    margin-right: 4dp;
}

.search_panel .action {
    layout-width: wrap_content;
    layout-height: 20dp;
    layout-gravity: center_vertical;
    padding: 2dp 6dp;
    transition: background-color 0.1s;
}

.search_panel .action:hover {
    background-color: var(--button-hover);
}

.search_panel .action:disabled {
    opacity: 0.5;
}

.search_panel.error .search_input {
    border-color: var(--theme-error);
}
```

The corresponding XML remains structurally obvious:

```xml
<hbox class="search_panel">
    <TextInput
        id="search"
        class="search_input" />

    <PushButton
        id="previous"
        class="action"
        text="Previous" />

    <PushButton
        id="next"
        class="action"
        text="Next" />
</hbox>
```

---

## 32. Quick decision guide

| Question | Prefer |
| --- | --- |
| Native widget should fill parent | `layout-width: match_parent` / `layout-height: match_parent` |
| Native widget should fit content | `wrap_content` |
| Child should receive remaining LinearLayout space | zero weighted axis + `layout-weight` |
| Reusable visual/layout rule | CSS class |
| Unique structural target | ID |
| Hover/focus/disabled styling | pseudo-class |
| Application-specific state | semantic class |
| Theme color | `var(--...)` |
| Light/dark-specific value | theme variable, then `light-dark()` / media query |
| Normal UI geometry | `dp` |
| Hide native widget and repack layout | `visible: false` |
| Scroll native content | `ScrollView` / scrollable widget |
| Overlay native controls | `RelativeLayout` |
| Responsive wrapping controls | `UIFlowLayout` |
| Browser Flex/Grid/positioning | HTML compatibility layer only |

---

## 33. Related documentation

Use these documents together:

- [`ui_introduction.md`](ui_introduction.md) — introduction to the eepp UI system.
- [`ui_authoring.md`](ui_authoring.md) — recommended application UI mental model and conventions.
- [`ui_layout_reference.md`](ui_layout_reference.md) — exact native layout behavior.
- [`css_specification.md`](css_specification.md) — exhaustive CSS syntax/property reference.
- [`ui_html_compatibility.md`](ui_html_compatibility.md) — HTML/web formatting semantics.
- [`ui_inspector.md`](ui_inspector.md) — runtime UI inspection and automation protocol.
- [`ui_databinding.md`](ui_databinding.md) — data binding.
- [`ui_charts.md`](ui_charts.md) — chart-specific API and behavior.

When this document and a generic browser CSS assumption disagree for ordinary application widgets,
follow eepp's native application UI semantics.
