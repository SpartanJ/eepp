# UI Application Authoring Guide

eepp provides a declarative UI system built around XML widget hierarchies, CSS styling, native layout containers, and C++ behavior.

The framework borrows ideas from several UI systems, most notably Android-style layout concepts and CSS, while also having its own widget, sizing, scene, and layout semantics. This gives eepp a flexible UI model, but it can also be misleading if concepts from Android or the web are assumed to behave identically.

This guide documents the **recommended way to build normal eepp application interfaces**. It is intended both for developers and for AI coding agents working on eepp applications.

The focus is practical application UI such as:

- application shells;
- toolbars and status bars;
- settings panels;
- dialogs and forms;
- editor and workspace layouts;
- lists, trees, and tables;
- sidebars and split views;
- application-specific tools and panels.

The goal is not to duplicate the API reference or CSS property reference. Instead, this guide establishes the mental model, conventions, and preferred patterns that should be used when authoring eepp UI.

When examples or analogies from Android, HTML, CSS, Qt, or other frameworks conflict with eepp behavior, **eepp's own layout rules and implementation take precedence**.

## 1. The most important distinction

eepp exposes **two different layout worlds**.

They coexist in the same UI framework, but they are intended for different jobs.

## Normal eepp application UI

For application UI such as ecode, eterm, eproc, dialogs, settings panels, toolbars, sidebars, forms, editors, tables, and controls, use eepp's **native layout system**:

- `UILinearLayout`
  - `<vbox>`
  - `<hbox>`
- `UIRelativeLayout`
- `UIGridLayout`
- `UIFlowLayout`
- `UISplitter`
- `UITabWidget`
- other specialized eepp containers

Use native eepp layout properties such as:

```text
layout_width
layout_height
layout_weight
layout_gravity
margin
padding
layout-to-left-of
layout-to-right-of
layout-to-top-of
layout-to-bottom-of
```

CSS is still used extensively for styling these widgets and may also set these eepp-native layout properties.

This is the normal and preferred way to build an eepp application UI.

---

## HTML compatibility layout

eepp also implements a large portion of web layout for:

- `UIWebView`
- `UIMarkdownView`
- HTML rendering
- HTML-compatible rich document layout

That includes concepts such as:

```text
display: block
display: inline
display: inline-block
display: flex
display: grid
table layout
float
absolute/fixed/sticky positioning
web min/max sizing behavior
```

These capabilities are primarily part of the **HTML compatibility layer**.

They can technically be used outside HTML content, but ordinary eepp application UI generally does not need them.

> **Do not default to web layout just because eepp supports CSS.**

For a normal eepp settings window, toolbar, sidebar, dialog, form, or editor shell, prefer the native eepp layout containers.

---

# 2. Mental model

Think of ordinary eepp UI as:

```text
XML
    defines widget/layout hierarchy

native eepp layouts
    decide where children go and how available space is distributed

CSS
    styles widgets
    and can set eepp-native layout properties

C++
    provides behavior, data, commands, models, bindings, and dynamic changes
```

The hierarchy is DOM-like, but ordinary application layout is **not browser layout**.

Example:

```xml
<vbox layout_width="match_parent"
      layout_height="match_parent"
      padding="12dp">

    <TextView
        text="Project"
        layout_width="match_parent"
        layout_height="wrap_content" />

    <TextInput
        id="project"
        layout_width="match_parent"
        layout_height="wrap_content" />

    <hbox
        layout_width="match_parent"
        layout_height="wrap_content">

        <Widget layout_width="0dp"
                layout_height="1dp"
                layout_weight="1" />

        <PushButton text="Cancel" />
        <PushButton text="Save"
                    margin-left="8dp" />
    </hbox>
</vbox>
```

The layout model here is eepp-native:

```text
vbox
    vertical packing

hbox
    horizontal packing

layout-width / layout-height
    child's sizing contract with the parent layout

layout-weight
    distribution of remaining space

layout-gravity
    child's placement within layout-controlled space
```

---

# 3. XML and CSS are not separate layout systems

A common mistake is assuming:

```text
XML attributes = Android layout
CSS properties = browser layout
```

That is wrong.

eepp's CSS engine can set eepp-native properties too.

These are equivalent in purpose:

```xml
<TextInput
    layout_width="0dp"
    layout_weight="1"
    layout_height="wrap_content" />
```

and a stylesheet rule such as:

```css
.form-input {
    layout-width: 0dp;
    layout-weight: 1;
    layout-height: wrap_content;
}
```

The choice between XML and CSS is about where the property should live, not about selecting a different layout engine.

A useful rule:

- put one-off structural layout values directly in XML;
- put repeated/shared values in CSS;
- use CSS for visual styling;
- use classes instead of repeating styling attributes;
- do not move structural values into CSS merely because browsers do.

---

# 4. Native size policies

Ordinary eepp layouts use three core size policies:

```cpp
enum class SizePolicy {
    Fixed,
    MatchParent,
    WrapContent
};
```

In XML/CSS these are normally represented through `layout_width` and `layout_height` in XML (or `layout-width` and `layout-height` in CSS).

---

## `wrap_content`

```xml
layout_width="wrap_content"
layout_height="wrap_content"
```

Meaning:

> Size the widget from its content/intrinsic size, including the relevant padding.

Typical uses:

- labels;
- buttons;
- toolbars;
- compact rows;
- dialog action buttons;
- intrinsic controls.

Example:

```xml
<PushButton
    text="Save"
    layout_width="wrap_content"
    layout_height="wrap_content" />
```

`wrap_content` is also the default value of native layout width/height where applicable.

---

## `match_parent`

```xml
layout_width="match_parent"
layout_height="match_parent"
```

Meaning:

> Consume the available size provided by the parent layout on that axis, accounting for the layout's margins/padding rules.

Typical uses:

- top-level content;
- editors;
- lists;
- tables;
- child rows that should span the container;
- main panes.

Example:

```xml
<ListView
    layout_width="match_parent"
    layout_height="match_parent" />
```

Do not interpret `match_parent` as CSS `width: 100%`.

They may often produce similar geometry, but they belong to different layout models and have different surrounding rules.

---

## Fixed size

A concrete dimension implies fixed sizing on that axis:

```xml
layout_width="240dp"
layout_height="32dp"
```

Use fixed sizes when the size itself is meaningful.

Avoid hard-coding dimensions simply to force a layout to look correct.

---

# 5. `layout-weight`: distributing remaining space

`layout_weight` in XML (`layout-weight` in CSS) is a `UILinearLayout` concept derived from Android's linear layout model.

It distributes remaining space along the layout orientation.

The essential pattern is:

## Horizontal flexible child

```xml
<hbox layout_width="match_parent"
      layout_height="wrap_content">

    <TextView
        text="Project"
        min-width="90dp"
        layout_gravity="center_vertical" />

    <TextInput
        layout_width="0dp"
        layout_height="wrap_content"
        layout_weight="1" />
</hbox>
```

For a **horizontal** linear layout:

```text
layout-width = 0
layout-weight > 0
```

means:

> Allocate this child a weighted share of the remaining horizontal space.

---

## Vertical flexible child

```xml
<vbox layout_width="match_parent"
      layout_height="match_parent">

    <TextView text="Build queue" />

    <ListView
        layout_width="match_parent"
        layout_height="0dp"
        layout_weight="1" />

    <hbox layout_width="match_parent"
          layout_height="wrap_content">
        ...
    </hbox>
</vbox>
```

For a **vertical** linear layout:

```text
layout-height = 0
layout-weight > 0
```

means:

> Allocate this child a weighted share of the remaining vertical space.

This is a canonical eepp pattern.

Real examples exist in:

```text
src/examples/ui_data_handling/ui_data_handling.cpp
src/examples/ui_data_collections/ui_data_collections.cpp
src/tools/ecode/plugins/pluginmanager.cpp
src/eepp/ui/tools/uiaudioplayer.cpp
```

---

# 6. `gravity` vs `layout-gravity`

This distinction is extremely important.

## `gravity`

`gravity` controls **content inside the widget**.

Example:

```xml
<TextView
    text="Hello"
    gravity="center" />
```

This centers the text/content inside the `TextView`.

Typical values include:

```text
left
right
top
bottom
center_horizontal
center_vertical
center
```

---

## `layout-gravity`

`layout-gravity` controls **the widget's placement relative to its parent layout**, when that parent/layout supports the operation.

Example:

```xml
<PushButton
    text="Close"
    layout_gravity="right|center_vertical" />
```

---

## Common agent mistake

Wrong reasoning:

> "The button itself is not centered, so set `gravity=center`."

That centers the button's **content**, not necessarily the button.

If the widget itself must move within layout-controlled space, inspect:

```text
layout-gravity
parent layout type
margins
size policies
layout weight
```

---

# 7. `UILinearLayout`: the default application layout

`UILinearLayout` is the workhorse layout for ordinary application UI.

Aliases:

```xml
<vbox> ... </vbox>
<hbox> ... </hbox>
```

Use it for most application structures.

---

## Vertical box

```xml
<vbox
    layout_width="match_parent"
    layout_height="match_parent">

    <WidgetA />
    <WidgetB />
    <WidgetC />
</vbox>
```

Children are packed vertically.

Use for:

- settings pages;
- sidebar sections;
- forms;
- panels;
- dialogs;
- stacked tool areas.

---

## Horizontal box

```xml
<hbox
    layout_width="match_parent"
    layout_height="wrap_content">

    <WidgetA />
    <WidgetB />
    <WidgetC />
</hbox>
```

Children are packed horizontally.

Use for:

- form rows;
- button rows;
- toolbars;
- status rows;
- label + field pairs.

---

## Canonical growing-content pattern

A very common application shell is:

```xml
<vbox lw="mp" lh="mp">

    <hbox lw="mp" lh="wc">
        <!-- toolbar -->
    </hbox>

    <SomeMainWidget
        lw="mp"
        lh="0dp"
        lw8="1" />

    <hbox lw="mp" lh="wc">
        <!-- status / actions -->
    </hbox>
</vbox>
```

Note: use the weight on the axis controlled by the linear layout.

For a vertical layout, the flexible child uses height `0dp` plus weight.

---

# 8. Property naming convention and aliases

eepp accepts property-name aliases, including both underscore and hyphenated forms, but normal eepp
code follows a useful convention:

```text
XML attributes  -> underscore-separated
CSS properties  -> hyphen-separated
```

Preferred examples:

```xml
<TextInput
    layout_width="0dp"
    layout_height="wrap_content"
    layout_weight="1"
    layout_gravity="center_vertical" />
```

```css
.form-input {
    layout-width: 0dp;
    layout-height: wrap_content;
    layout-weight: 1;
    layout-gravity: center_vertical;
}
```

This distinction is intentional even though eepp can resolve aliases. It makes XML feel like widget
configuration while CSS retains normal CSS spelling, and it makes mixed XML/CSS code easier to read.

Compact aliases are also used heavily in production code:

```text
layout_width    -> lw
layout_height   -> lh
layout_weight   -> lw8
layout_gravity  -> lg
```

Special size abbreviations commonly used in eepp layouts:

```text
mp = match_parent
wc = wrap_content
```

So:

```xml
<vbox lw="mp" lh="mp">
```

is an idiomatic compact form of:

```xml
<vbox
    layout_width="match_parent"
    layout_height="match_parent">
```

Agents should be able to read both styles.

When writing new documentation-oriented examples, prefer the full names first.

When editing an existing codebase, follow the local style.

---

# 9. `UIRelativeLayout`

Use `UIRelativeLayout` when children need relationships to each other or the parent that are awkward in a simple linear hierarchy.

It supports relationships such as:

```text
layout-to-left-of
layout-to-right-of
layout-to-top-of
layout-to-bottom-of
```

Conceptually:

```xml
<RelativeLayout
    layout_width="match_parent"
    layout_height="match_parent">

    <Widget id="sidebar" ... />

    <Widget
        id="content"
        layout_to_right_of="sidebar"
        ... />
</RelativeLayout>
```

Do not use a relative layout just because it allows arbitrary placement.

If a hierarchy can be naturally represented as nested `vbox`/`hbox`, that is usually easier to maintain.

ecode uses `UIRelativeLayout` for several larger composite areas where the relative relationships are genuinely useful.

Examples:

```text
src/tools/ecode/uiwelcomescreen.cpp
src/tools/ecode/uirightpanel.cpp
src/tools/ecode/uibuildsettings.cpp
```

---

# 10. Native `UIGridLayout` is NOT CSS Grid

eepp has a native `UIGridLayout`.

This must not be confused with the HTML/CSS Grid implementation.

Native `UIGridLayout` lays children into repeated rows/columns using eepp-native sizing rules.

It supports properties such as:

```text
column-mode
row-mode
column-weight
row-weight
column-width
row-height
column-margin
row-margin
```

Modes can use fixed sizes or weighted sizes.

Use native `UIGridLayout` when building a normal application UI that needs a repeated grid of similarly sized children.

Do not reach for CSS:

```css
display: grid;
grid-template-columns: ...;
```

unless you are intentionally working in the HTML compatibility layout world.

---

# 11. `UIFlowLayout`

`UIFlowLayout` is eepp's native wrapping flow layout.

It places visible children horizontally and starts a new row when the available width is exhausted:

```text
A B C D
E F G
H I
```

Its behavior is similar in spirit to layout types commonly called `FlowLayout`, `Wrap`, `WrapPanel`,
or `FlowRow` in other UI frameworks.

It also supports per-row vertical alignment through `row-valign`.

Use it for things such as:

- tag/chip collections;
- language selectors;
- wrapping tool controls;
- compact option groups;
- responsive rows of controls.

ecode uses it for dynamic language buttons, build/run options, search controls, and other wrapping
control groups. eepp also uses it in merge-view tooling.

Example production locations:

```text
src/tools/ecode/ecode.cpp
src/tools/ecode/uibuildsettings.cpp
src/eepp/ui/tools/uimergeview.cpp
```

# 12. `UISplitter` and workspace layout

For resizable application panes, use `UISplitter`.

Do not emulate resizable panes with manual dimensions or HTML flex resizing.

Typical uses:

```text
sidebar | editor
editor | terminal
tree | details
multi-pane workspace
```

For tabbed/split editor workspaces, eepp also provides `UITabWidget` and `UITabWidgetSplitter`.

ecode is the primary production reference.

---

# 13. Margins and padding

The distinction follows the usual box-model intuition:

```text
margin
    outside the widget

padding
    inside the widget, between its edge and its content/children
```

Typical native layout usage:

```xml
<vbox
    padding="12dp">

    <TextView
        margin-bottom="8dp"
        text="Settings" />
</vbox>
```

Margins participate in native layout calculations.

Do not add spacer widgets unless spacing represents actual flexible space.

Use margins/padding for ordinary spacing.

A zero-sized weighted widget can still be appropriate when the goal is intentionally consuming remaining space:

```xml
<Widget
    layout_width="0dp"
    layout_height="1dp"
    layout_weight="1" />
```

---

# 14. Use `dp` for application UI dimensions

eepp uses device-independent pixels:

```text
dp
```

for scalable application UI metrics.

Examples:

```xml
padding="8dp"
margin-left="4dp"
layout_height="32dp"
font-size="14dp"
```

Do not use raw pixel dimensions for ordinary application UI unless physical/native pixels are specifically required.

Pixel-density behavior is part of the framework's UI environment.

---

# 15. CSS in ordinary eepp application UI

CSS is still a major part of normal eepp UI.

Use it for:

- colors;
- backgrounds;
- borders;
- fonts;
- icons;
- transitions;
- animations;
- pseudo-classes;
- shared dimensions;
- margins/padding;
- native layout properties;
- theme variables;
- media queries;
- reusable classes.

Example:

```css
.settings-row {
    layout-width: match_parent;
    layout-height: wrap_content;
    margin-bottom: 8dp;
}

.settings-row > TextView {
    min-width: 120dp;
    layout-gravity: center_vertical;
}

.settings-row > TextInput {
    layout-width: 0dp;
    layout-weight: 1;
}
```

This is still the **native eepp layout system**, even though the rules are written in CSS.

---

# 16. Do not casually use HTML layout properties in application UI

For ordinary application UI, avoid defaulting to:

```css
display: flex;
display: grid;
display: block;
float: left;
position: sticky;
```

Those are primarily part of the HTML compatibility system.

The fact that eepp can interpret them does not make them the preferred application-layout abstraction.

Bad default agent reasoning:

> "I need two controls next to each other, so use `display:flex`."

Preferred eepp reasoning:

```xml
<hbox>
    <WidgetA />
    <WidgetB />
</hbox>
```

Bad default reasoning:

> "I need a sidebar and content area, so use CSS Grid."

Preferred eepp reasoning:

```text
UISplitter
or
nested native layouts
```

depending on whether the divider must be user-resizable.

---

# 17. HTML layout is appropriate when rendering HTML

When working inside:

```text
UIWebView
UIMarkdownView
HTML document content
HTML-compatible rich content
```

use normal web-layout reasoning where supported.

There:

```css
display: flex;
min-width: 0;
position: sticky;
```

may be exactly correct.

The Runtime UI Inspector treats a `UIWebView` document as its own scene. Do not assume application-scene selectors automatically enter the HTML document scene.

---

# 18. Choose layouts by behavior

Use this decision guide.

## Need a vertical sequence?

Use:

```xml
<vbox>
```

---

## Need a horizontal sequence?

Use:

```xml
<hbox>
```

---

## Need one child to consume leftover space?

Use `layout-weight` on the linear-layout axis.

Horizontal:

```xml
layout_width="0dp"
layout_weight="1"
```

Vertical:

```xml
layout_height="0dp"
layout_weight="1"
```

---

## Need resizable panes?

Use:

```text
UISplitter
```

---

## Need tabs/workspace splitting?

Use:

```text
UITabWidget
UITabWidgetSplitter
```

---

## Need children positioned relative to siblings?

Use:

```text
UIRelativeLayout
```

---

## Need repeated same-sized/weighted grid cells?

Use:

```text
UIGridLayout
```

not CSS Grid.

---

## Need items to flow horizontally and wrap?

Use:

```text
UIFlowLayout
```

This is the canonical native eepp flow/wrap layout.

---

## Need document/web flow?

Use the HTML compatibility system through:

```text
UIWebView
UIMarkdownView
```

---

# 19. Canonical application patterns

Agents should prefer production-tested patterns instead of inventing layout strategies.

---

## Pattern: form row

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
        layout_height="wrap_content"
        layout_weight="1" />
</hbox>
```

Reference:

```text
src/examples/ui_data_handling/ui_data_handling.cpp
```

---

## Pattern: header + flexible content + footer

```xml
<vbox
    layout_width="match_parent"
    layout_height="match_parent">

    <hbox
        layout_width="match_parent"
        layout_height="wrap_content">
        <!-- header -->
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

This is one of the most important patterns to understand.

---

## Pattern: actions aligned to the right

```xml
<hbox
    layout_width="match_parent"
    layout_height="wrap_content">

    <Widget
        layout_width="0dp"
        layout_height="1dp"
        layout_weight="1" />

    <PushButton text="Cancel" />
    <PushButton text="Save" margin-left="8dp" />
</hbox>
```

Alternatively, where parent gravity semantics make it appropriate:

```xml
<hbox layout_gravity="right">
    ...
</hbox>
```

Follow the existing component's style.

---

## Pattern: settings UI

Prefer reusable framework facilities where available.

eepp now provides:

```text
EE::UI::Tools::UISettingsPanel
```

which is used by real applications including ecode, eterm, and eproc.

Do not recreate a settings framework from primitive widgets without first checking whether `UISettingsPanel` covers the requirement.

---

## Pattern: application charts

eepp provides:

```text
UIChart
```

with framework-level charting behavior.

Do not implement charts manually with primitive drawing unless the requirement falls outside the chart system.

See:

```text
docs/articles/ui_charts.md
```

and eproc for a production consumer.

---

# 20. Common agent mistakes

## Mistake: using browser Flexbox for an application toolbar

Avoid:

```css
.toolbar {
    display: flex;
}
```

for ordinary application UI.

Prefer:

```xml
<hbox class="toolbar">
```

---

## Mistake: confusing native GridLayout and CSS Grid

These are separate systems.

Use native `UIGridLayout` for ordinary eepp application grids.

Use CSS Grid for HTML compatibility content.

---

## Mistake: setting `gravity` to move a widget

`gravity` generally aligns the widget's content.

Use `layout-gravity`, layout hierarchy, margins, or the appropriate parent-layout mechanism to place the widget itself.

---

## Mistake: weight without zero size on the weighted axis

Avoid:

```xml
<hbox>
    <TextInput
        layout_width="match_parent"
        layout_weight="1" />
</hbox>
```

Preferred:

```xml
<hbox>
    <TextInput
        layout_width="0dp"
        layout_weight="1" />
</hbox>
```

For a vertical layout, use height `0dp`.

---

## Mistake: fixed sizing to solve parent-layout problems

Before forcing:

```xml
layout_width="417dp"
```

inspect:

```text
parent type
layout-width policy
layout-height policy
layout-weight
layout-gravity
margin
padding
min/max dimensions
```

A hard-coded size often hides the real layout mistake.

---

## Mistake: assuming CSS means browser behavior everywhere

eepp's stylesheet engine applies to ordinary widgets too, but ordinary widgets often use **eepp-native layout semantics**.

Read the property definition and parent layout behavior instead of assuming browser behavior from the property syntax.

---

## Mistake: treating `UIFlowLayout` as an overlay

`UIFlowLayout` is a wrapping flow layout: children advance horizontally and wrap into new rows.

It is not a z-stack or absolute overlay container.

---

# 21. Debug layout with the Runtime UI Inspector & Automation Protocol

Do not guess geometry when the application can tell you the answer.

Enable the inspector and inspect the real running UI. See
[`ui_inspector.md`](ui_inspector.md) for activation, client setup, and scene selection.
The handles and scene IDs in the commands below are examples; get the actual values from
`contexts` and `query` for the running application.

Basic workflow:

```text
1. discover windows/scenes
2. query the target by selector
3. inspect geometry and relevant properties
4. inspect its parent
5. capture a screenshot if visual context matters
6. interact if necessary
7. wait for the next frame
8. inspect again
```

Example commands:

```sh
python3 projects/scripts/eepp-inspect.py contexts

python3 projects/scripts/eepp-inspect.py query '#project'

python3 projects/scripts/eepp-inspect.py inspect \
    w:42 \
    geometry.size \
    geometry.position \
    css.layout-width \
    css.layout-height

python3 projects/scripts/eepp-inspect.py tree --depth 4

python3 projects/scripts/eepp-inspect.py screenshot --scene scene:1
```

Use the inspector instead of inferring runtime layout solely from source code or screenshots.

---

# 22. Debugging a wrong size

When a widget is unexpectedly too large or too small, inspect in this order:

```text
1. What parent layout owns it?
2. What are layout-width and layout-height?
3. Is the relevant axis Fixed, MatchParent, or WrapContent?
4. Does it have layout-weight?
5. If weighted, is the weighted axis set to 0?
6. What margins does the child have?
7. What padding does the parent have?
8. Are min-width/max-width/min-height/max-height constraining it?
9. Is the widget's intrinsic content changing wrap_content?
10. Is the parent itself constrained correctly?
```

Only after that consider a fixed size.

---

# 23. Debugging wrong alignment

Inspect:

```text
gravity
layout-gravity
parent layout type
layout direction/orientation
margin
widget size
available parent space
```

Remember:

```text
gravity
    content inside widget

layout-gravity
    widget placement in parent layout
```

---

# 24. Debugging weighted layouts

For a horizontal `hbox`:

```text
weighted dimension = width
```

Expected flexible-child pattern:

```xml
layout_width="0dp"
layout_weight="..."
```

For a vertical `vbox`:

```text
weighted dimension = height
```

Expected pattern:

```xml
layout_height="0dp"
layout_weight="..."
```

If the result is wrong, inspect sibling fixed sizes and margins because weights distribute **remaining** space.

---

# 25. Debugging `UIWebView`

A `UIWebView` document is a separate nested scene.

An application-scene query does not cross into it.

Workflow:

```text
query the UIWebView in parent scene
    ↓
read its documentScene handle
    ↓
query that scene separately
```

At that point web-layout rules are appropriate because the target is actual HTML-compatible content.

---

# 26. Learn from production code

The best reference application is ecode.

It uses a very broad cross-section of eepp UI:

- linear layouts;
- relative layouts;
- flow layouts;
- splitters;
- tabs;
- menus;
- settings;
- status UI;
- dialogs;
- tables/trees/models;
- code editors;
- terminal integration;
- plugins;
- rich text;
- Markdown/HTML;
- notifications;
- complex commands;
- data binding;
- nested scenes.

Most ecode source lives inside the eepp repository:

```text
src/tools/ecode/
```

Useful starting points include:

```text
src/tools/ecode/applayout.xml.hpp
src/tools/ecode/uiwelcomescreen.cpp
src/tools/ecode/uirightpanel.cpp
src/tools/ecode/uibuildsettings.cpp
src/tools/ecode/plugins/pluginmanager.cpp
src/tools/ecode/plugins/git/gitplugin.cpp
src/tools/ecode/plugins/debugger/debuggerplugin.cpp
src/tools/ecode/plugins/aiassistant/chatui.cpp
```

Framework-level reusable components are also excellent references:

```text
src/eepp/ui/tools/uisettingspanel.cpp
src/eepp/ui/tools/uiaudioplayer.cpp
src/eepp/ui/tools/uidocfindreplace.cpp
src/eepp/ui/tools/uimergeview.cpp
```

Simple examples remain useful when learning one concept in isolation:

```text
src/examples/ui_application_hello_world/
src/examples/ui_data_handling/
src/examples/ui_data_collections/
src/examples/ui_richtext/
```

---

# 27. Source priority for agents

When sources disagree or an assumption is uncertain, use this priority:

```text
1. Current eepp implementation
2. Current eepp tests
3. Current production eepp/ecode usage
4. Current eepp documentation
5. Android/web analogy
6. Generic prior knowledge
```

Android and browser knowledge are useful for intuition only.

They are **not authoritative** for eepp behavior.

---

# 28. When modifying existing UI

Do not rewrite an existing screen into a different layout paradigm without a strong reason.

Before editing:

```text
identify current parent layout
inspect nearby sibling patterns
find similar code in the same subsystem
preserve local XML/CSS naming style
preserve compact/full property naming style
```

For example, if nearby code uses:

```xml
lw="mp"
lh="wc"
lw8="1"
lg="center_vertical"
```

an agent can use the same aliases.

For new explanatory examples and documentation, prefer full names first.

---

# 29. Runtime verification is expected

For non-trivial visual changes, an agent should not stop after editing code.

Preferred closed-loop workflow:

```text
read layout rules
    ↓
find canonical production pattern
    ↓
edit XML/CSS/C++
    ↓
build/run
    ↓
inspect runtime tree
    ↓
inspect geometry/properties
    ↓
capture screenshot when useful
    ↓
interact with UI
    ↓
wait next frame
    ↓
verify final state
```

The **Runtime UI Inspector & Automation Protocol** exists specifically to make this workflow reliable and automatable.

---

# 30. Do not create a second mental model unnecessarily

For ordinary eepp application UI, think:

```text
native widgets
+
native layouts
+
CSS styling
```

Do not think:

```text
HTML page rendered as desktop UI
```

unless you are actually working in the HTML compatibility layer.

This single distinction prevents many layout mistakes.

---

# 31. Quick reference

## Preferred ordinary UI containers

```text
<vbox> / <hbox>    sequential layout
RelativeLayout     sibling-relative placement
GridLayout         repeated native grid
FlowLayout        horizontal wrapping/flow
Splitter           resizable panes
TabWidget          tabs
TabWidgetSplitter  split/tab workspaces
```

## Core sizing

```text
wrap_content       intrinsic/content size
match_parent       consume parent allocation
fixed dimension    explicit size
layout-weight      share remaining LinearLayout space
```

## Alignment

```text
gravity            content inside widget
layout-gravity     widget against parent layout
```

## Styling

```text
CSS is normal and encouraged.
CSS does not imply browser layout.
```

## HTML

```text
Flexbox/Grid/block/inline/etc.
primarily belong to UIWebView/UIMarkdownView/HTML compatibility.
```

## Debugging

```text
Use the Runtime UI Inspector & Automation Protocol.
Inspect actual state instead of guessing.
```
