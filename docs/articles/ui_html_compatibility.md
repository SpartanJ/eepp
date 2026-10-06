# HTML Compatibility Layer

eepp includes an HTML/CSS compatibility layer for rendering document-like content inside the UI
system.

This layer is used primarily by:

- `UIWebView`;
- `UIMarkdownView`;
- HTML-derived widgets such as `UIRichText`, `UITextSpan`, HTML tables, forms, images, and inputs.

It is intentionally distinct from normal eepp application UI authoring.

For ordinary application UI, use native eepp layouts such as `vbox`, `hbox`,
`UIRelativeLayout`, `UIGridLayout`, `UIFlowLayout`, `UISplitter`, and `UITabWidget`.
See [`ui_authoring.md`](ui_authoring.md),
[`ui_layout_reference.md`](ui_layout_reference.md), and
[`ui_css_for_applications.md`](ui_css_for_applications.md).

This document describes the **HTML compatibility model**, the browser-style layout concepts it
implements, and the important places where it differs from a full browser engine.

---

## 1. The most important boundary

eepp has two related but different layout worlds.

### Native application UI

Normal eepp application interfaces use:

```text
UILinearLayout / vbox / hbox
UIRelativeLayout
UIGridLayout
UIFlowLayout
UISplitter
UITabWidget
UIScrollView
layout-width
layout-height
layout-weight
layout-gravity
```

CSS can configure those widgets, but CSS does not replace their native layout algorithms.

### HTML compatibility UI

HTML-derived widgets can instead participate in browser-style formatting contexts:

```text
block
inline
inline-block
list-item

flex
inline-flex

grid
inline-grid

table

float / clear
absolute / fixed / sticky positioning
```

The same CSS parser and property system are shared by both worlds, but the **layout semantics depend
on the widget type and formatting context**.

Do not use browser layout properties merely because the stylesheet parser accepts them.

---

## 2. `UIWebView` is the full document host

`UIWebView` is the main host for HTML documents.

It is implemented as a `UIScrollView` containing an embedded `UISceneNode` dedicated to the loaded
document.

Conceptually:

```text
UIWebView
    scroll viewport
        embedded UISceneNode
            document container
                html
                    head
                    body
```

The embedded scene gives the document its own:

- widget tree;
- stylesheet state;
- document URI;
- resource loading context;
- font-face registrations;
- navigation handling;
- text-selection context.

Use:

```cpp
auto* webView = UIWebView::New();
webView->loadURI( URI( "file:///path/document.html" ) );
```

or an HTTP/HTTPS URI.

`UIWebView` supports navigation history through:

```cpp
goHistoryBack();
goHistoryForward();
refresh();
reload();
canGoBack();
canGoForward();
```

It also exposes navigation events for started, completed, and failed loads.

---

## 3. `UIMarkdownView` uses the HTML compatibility layer too

`UIMarkdownView` converts Markdown to XHTML and then loads the resulting body children as eepp HTML
widgets.

Its pipeline is approximately:

```text
Markdown
    -> Markdown::toXHTML()
    -> HTMLFormatter::HTMLBodyToXML()
    -> eepp HTML widgets
```

Unlike `UIWebView`, `UIMarkdownView` shares its enclosing scene and does not create a dedicated
embedded scene. It can follow local Markdown links without changing the shared scene's URI.

It is a native vertical layout hosting HTML-derived content and loads the **basic HTML default
styles** needed for Markdown rendering.

Use `UIMarkdownView` for rendered Markdown content, not as a general browser replacement.

---

## 4. HTML is parsed before eepp widgets are created

`UIWebView` does not feed arbitrary HTML directly into the normal XML layout parser.

HTML is first parsed with Gumbo, then serialized into strict XML through
`Tools::HTMLFormatter`.

This stage handles browser-style HTML parsing details such as:

- malformed/non-XML HTML syntax;
- omitted closing syntax where HTML permits it;
- HTML void elements;
- boolean attributes;
- normalized tag names.

The resulting strict XML is then loaded through eepp's widget creation system.

This architecture is important:

```text
HTML source
    -> Gumbo HTML tree
    -> strict XML representation
    -> UIWidgetCreator
    -> eepp widget tree
```

The final rendered document is therefore still made of eepp widgets.

---

## 5. Only registered HTML elements are materialized

HTML compatibility is not based on a generic DOM element class that accepts every possible tag.

HTML tags are mapped through `UIWidgetCreator`.

Currently registered HTML elements include the major supported groups:

```text
document:
    html
    head
    body

text and inline:
    a
    label
    span
    em
    b
    strong
    small
    i
    cite
    kbd
    sub
    sup
    time
    u
    ins
    s
    strike
    del
    font
    code
    abbr
    tt
    mark

blocks:
    div
    p
    blockquote
    h1 ... h6
    br
    hr
    pre

lists:
    ul
    ol
    dl
    dt
    dd
    li

semantic containers:
    header
    article
    figure
    figcaption
    footer
    main
    section
    nav
    aside
    center

interactive/details:
    details
    summary

media/replaced content:
    picture
    img
    svg

forms:
    form
    input
    textarea
    button

tables:
    table
    thead
    tbody
    tfoot
    tr
    th
    td
```

The exact set is defined in:

```text
src/eepp/ui/uiwidgetcreator.cpp
```

An unregistered tag does not automatically become a transparent generic HTML container.

Applications can register additional widget creators if they need custom tags.

---

## 6. This is not a JavaScript browser runtime

`<script>` nodes are ignored by the UI layout loader.

The HTML compatibility layer should therefore be understood as:

```text
HTML parsing
CSS styling
document layout
native interaction for supported elements
navigation/resource loading
```

not as:

```text
HTML + DOM + JavaScript browser engine
```

There is no browser JavaScript environment or general DOM scripting model implied by `UIWebView`.

If application logic is required, implement it through eepp/C++ behavior.

---

## 7. Default HTML styles

eepp provides built-in HTML default styles.

The basic defaults cover conventional document presentation for elements such as:

```text
body
h1 ... h6
p
pre
blockquote
hr
ul / ol / dl
b / strong
i / em
small
u / ins
s / strike / del
code / kbd
sub / sup
mark
a
summary
```

`UIWebView` also loads document-oriented defaults for controls and document colors, including
inputs, textareas, buttons, checkboxes, and radio buttons.

These defaults are inserted with lower selector specificity so author styles can override them.

`UIMarkdownView` loads the basic HTML defaults rather than the complete standalone-document defaults.

The source of the defaults is:

```text
src/eepp/ui/uiwidgetcreator.cpp
```

---

## 8. HTML attributes and native XML attributes have different cascade behavior

This is an important distinction.

For **normal native eepp widgets**, XML attributes are treated as inline-style properties with
inline specificity.

For widgets marked as HTML elements, ordinary HTML attributes are loaded with low specificity so
CSS can override them in the expected HTML-like way.

For example:

```html
<img width="200" class="preview">
```

can still be overridden by author CSS:

```css
.preview {
    width: 100px;
}
```

The explicit HTML `style` attribute remains inline style:

```html
<img style="width: 200px">
```

and therefore has inline specificity.

This distinction exists specifically so HTML presentation attributes do not behave like native eepp
XML configuration.

---

## 9. Attribute selectors are supported

The selector engine supports attribute selectors.

For HTML widgets, `data-*` attributes are stored as HTML data properties and can participate in
attribute matching.

Examples:

```css
input[type="text"] {
    ...
}

input[type="password"] {
    ...
}

[data-state="warning"] {
    ...
}
```

Supported attribute operators include:

```text
[attr]
[attr=value]
[attr~=value]
[attr|=value]
[attr^=value]
[attr$=value]
[attr*=value]
```

For native widgets, attribute selectors operate through properties exposed by the widget property
system.

For HTML widgets, `data-*` attributes are additionally available directly.

---

## 10. Block and inline formatting

The HTML layer implements separate formatting behavior for:

```css
display: block;
display: inline;
display: inline-block;
display: list-item;
display: none;
```

Block and inline content is laid out through the HTML layouter system rather than native
`UILinearLayout`.

Inline text and inline widgets are integrated into `RichText` line construction.

This includes:

- text runs;
- line wrapping;
- inline boxes;
- atomic inline boxes;
- inline replaced elements;
- baselines;
- line-height;
- vertical alignment;
- inline background/text painting.

Block elements participate in vertical document flow and intrinsic sizing.

`display: none` removes the element from layout and hit testing.

---

## 11. Flexbox

HTML widgets support:

```css
display: flex;
display: inline-flex;
```

with a dedicated `FlexLayouter`.

The currently implemented flex state includes:

```text
flex-direction
flex-wrap
justify-content
align-items
align-content
align-self

flex-grow
flex-shrink
flex-basis
order

row-gap
column-gap
gap
```

Supported direction values include:

```text
row
row-reverse
column
column-reverse
```

Wrapping includes:

```text
nowrap
wrap
wrap-reverse
```

The layouter handles multiple flex lines, grow/shrink distribution, gaps, alignment, auto margins,
baseline alignment, order, percentage flex bases, intrinsic measurement, and stretching.

Children of flex containers are blockified for layout, including inline children.

### Important distinction

Flexbox belongs to the HTML formatting layer.

Do not replace a normal application `<hbox>` with:

```css
display: flex;
```

unless the subtree is intentionally HTML content.

For normal application UI, `UILinearLayout` remains the canonical row/column layout.

---

## 12. CSS Grid

HTML widgets support:

```css
display: grid;
display: inline-grid;
```

through `GridLayouter`.

The current grid model supports properties including:

```text
grid-template-rows
grid-template-columns
grid-template-areas

grid-auto-rows
grid-auto-columns
grid-auto-flow

grid-row-start
grid-row-end
grid-column-start
grid-column-end

grid-row
grid-column
grid-area

justify-items
justify-self
align-items
align-self

row-gap
column-gap
gap

order
```

The track model supports:

```text
fixed lengths
percentages
fr tracks
auto
min-content
max-content
fit-content(...)
minmax(...)
repeat(...)
auto-fill
auto-fit
named lines
template areas
implicit tracks
```

Grid items support definite placement, spans, automatic placement, dense auto-flow, intrinsic track
sizing, and alignment.

### Native `UIGridLayout` is unrelated

Do not confuse:

```text
UIGridLayout
```

with:

```text
display: grid
```

`UIGridLayout` is a native eepp repeated-cell layout.

CSS Grid is implemented by the HTML compatibility layer.

They have different APIs, sizing rules, and intended uses.

---

## 13. Tables

HTML tables use dedicated HTML widgets and `TableLayouter`.

Supported structural elements include:

```text
table
thead
tbody
tfoot
tr
th
td
```

The table implementation includes:

```text
intrinsic column sizing
table-layout: auto
table-layout: fixed
cell padding
cell spacing
colspan
row/cell sizing
```

Table layout is not implemented by native `UIGridLayout`.

If the source content is HTML tabular content, use HTML table semantics.

If the application is building an interactive application data view, prefer native widgets such as
`UITableView` or `UITreeView`.

---

## 14. Floats and `clear`

HTML widgets support:

```css
float: left;
float: right;
float: none;

clear: left;
clear: right;
clear: both;
```

Floats participate in block/inline document formatting and create text exclusions so following
inline content can wrap around them.

Floated inline elements are blockified for layout.

Floats also establish a block formatting context where applicable.

These semantics exist for document layout and should not be used as a general native application
placement system.

---

## 15. Positioned layout

The HTML property system recognizes:

```css
position: static;
position: relative;
position: absolute;
position: fixed;
position: sticky;
```

### Absolute positioning

`absolute` elements are removed from normal flow.

The containing block is selected from the nearest positioned HTML ancestor, with root fallback.

Insets are supported:

```css
top
right
bottom
left
```

including percentage values where the containing block has the required definite size.

Opposing insets can determine the used size of an auto-sized positioned box.

Auto margins are also handled in the positioned constraint equation.

### Fixed positioning

`fixed` elements are positioned against the relevant scroll/document viewport rather than ordinary
normal flow and are updated when scrolling changes.

### Sticky positioning

`sticky` participates in normal flow and is adjusted relative to a scroll viewport while respecting
the containing block.

Current sticky handling focuses on vertical top/bottom constraints.

### Relative positioning

`relative` currently participates in the positioned-element model, including containing-block and
stacking behavior.

Do not assume complete browser parity for relative `top/right/bottom/left` displacement; the current
implementation's explicit inset positioning logic is primarily implemented for absolute/fixed and
sticky positioning.

---

## 16. `z-index` and paint order

The HTML layer has CSS-aware paint ordering rather than relying only on raw widget child order.

The current model distinguishes categories broadly corresponding to the supported CSS stacking
subset:

```text
negative positioned/z-index content
normal-flow content
floats
positioned auto/zero content
positive positioned/z-index content
```

`z-index` applies to positioned elements and to flex/grid items where supported.

The implementation supports stacking groups for:

```text
fixed/sticky elements
positioned elements with applicable z-index
flex/grid items with applicable z-index
```

This is intentionally a supported subset of browser stacking-context behavior, not a claim of full
CSS Appendix E parity.

---

## 17. CSS sizing and the box model

HTML widgets support browser-style CSS sizing properties:

```css
width
height
min-width
min-height
max-width
max-height

margin
padding
border
box-sizing
```

`box-sizing` supports:

```text
content-box
border-box
```

For HTML widgets, `width` and `height` are interpreted through the CSS box model rather than merely
as raw native widget dimensions.

This is one reason the HTML compatibility layer should remain conceptually separate from normal
native application layout.

---

## 18. Percentage sizing uses containing blocks

HTML percentage sizing is resolved against CSS containing-block dimensions.

This applies to relevant properties such as:

```text
width
height
margin
padding
position offsets
flex-basis
grid tracks
```

The implementation also distinguishes cases where percentage height cannot resolve because the
containing block does not have a definite height.

In those cases, the HTML layer avoids treating the percentage as an ordinary fixed native
dimension.

The root document uses the document viewport as the initial containing block for relevant root/body
calculations.

---

## 19. Intrinsic sizing

The HTML layout layer has explicit intrinsic sizing support.

Concepts used internally include:

```text
minimum intrinsic width
maximum intrinsic width
min-content
max-content
fit-content
shrink-to-fit
```

These are used by block, flex, grid, table, replaced-element, and out-of-flow layout.

This is different from native eepp `WrapContent`.

They may eventually produce similar-looking results, but they are not the same algorithm.

Do not explain HTML `min-content` or `max-content` in terms of native `SizePolicy`.

---

## 20. Auto margins

HTML formatting contexts implement context-specific auto-margin behavior.

Examples include:

- horizontal centering of normal-flow blocks;
- free-space absorption in flex layouts;
- item alignment behavior in grid/flex;
- auto margins in positioned constraint equations.

This is distinct from ordinary native-layout auto-margin handling.

The HTML layer resolves used margins for the current formatting role without treating every
`margin: auto` as the same generic layout operation.

---

## 21. Overflow is only partially browser-equivalent

The CSS property is recognized:

```css
overflow: visible;
overflow: hidden;
overflow: auto;
overflow: scroll;
```

For HTML widgets, non-`visible` overflow also establishes a block formatting context.

However, the underlying native widget behavior currently maps:

```text
hidden
auto
scroll
```

to clipping of the widget content box.

It does **not** create an independent browser-like scroll container for every arbitrary HTML
element.

`UIWebView` itself provides document scrolling through its native `UIScrollView`.

Also note that `overflow-x` and `overflow-y` currently alias the same `overflow` property; they are
not independent axes yet.

This is an important compatibility limitation.

---

## 22. `visibility` is not fully browser-equivalent

HTML widgets recognize:

```css
visibility: visible;
visibility: hidden;
visibility: collapse;
```

The current implementation maps `hidden` to native widget visibility.

As a result, `visibility: hidden` does not currently guarantee the browser behavior of preserving
the element's normal-flow geometry while suppressing painting.

Flex layout has specific handling for `visibility: collapse`, but this should not be interpreted as
complete `visibility` parity across every formatting context.

When precise browser-compatible visibility geometry matters, verify the current behavior rather
than assuming browser semantics.

---

## 23. Text formatting and inheritance

HTML text is rendered through `UIRichText`, `UITextSpan`, and `UITextNode`.

Supported text-oriented CSS includes concepts such as:

```text
font-family
font-size
font-style
font-weight
color
text-decoration
text-align
text-indent
text-transform
line-height
white-space
white-space-collapse
tab-size
text shadow/stroke
```

Relevant properties participate in eepp's current inheritance system.

HTML inline elements therefore behave much more like document text than ordinary independent native
application widgets.

The exact supported property set is documented in
[`css_specification.md`](css_specification.md).

---

## 24. Text selection is document-wide

`UIWebView` and `UIMarkdownView` both provide a `UITextSelectionController`.

Selection can cross individual rich-text/span widget boundaries rather than being limited to a
single widget.

HTML widgets support:

```css
user-select: auto;
user-select: text;
user-select: none;
user-select: contain;
user-select: all;
```

`-webkit-user-select` is accepted as an alias.

The default used behavior is text-selectable unless inherited `none` or `all` semantics change it.

This is one of the important differences between document content and ordinary isolated text
widgets.

---

## 25. Links and navigation

`<a>` is represented by `UIAnchorSpan`.

Anchor activation uses the scene navigation path.

Inside a `UIWebView`, navigation requests are intercepted by the embedded document scene and routed
back into the owning `UIWebView`, including relative URL resolution.

This means ordinary links can navigate the current web view.

`NavigationRequest` carries the source node, mouse buttons, and keyboard modifiers. Middle click,
the platform modifier plus left click, or an explicit `Target::NewTab` request emits
`Event::OnLinkOpenRequested` on the WebView. Subscribe to this event, open the resolved URI in a
new view, and call `UIWebView::LinkOpenEvent::accept()` to handle it. Unhandled requests navigate
the current view. Applications add their own link menu actions through `OnCreateContextMenu`;
`ContextMenuEvent::getTarget()` identifies the clicked document node.

`getTitle()` returns the parsed HTML document title. `onTitleChanged()` reports the title after
loading, including an empty title for pages without one. The `ui_html` example uses it for tab labels
and falls back to the host or URL. Tabs cap their width at 200dp and truncate long titles with ellipsis.
The example enables `UITabWidgetSplitter::setShowTabBarWhenSplit(true)` to keep each pane's tab bar
visible while split. This option is disabled by default for other splitter users.

`<link rel="icon" href="...">` (including `rel="shortcut icon"`) loads the declared favicon through
the document's shared image cache. Relative URLs resolve against the document URL, and SVG icons
are supported. `Event::OnFaviconChanged` carries a `UIWebView::FaviconEvent` with the loaded texture;
an empty icon clears the previous document's favicon. Responses from an older navigation are ignored.
The `ui_html` example displays the texture as a 16dp tab icon.

`UIWebView` maintains its own navigation history.

`UIMarkdownView` follows local `.md` and `.markdown` links in the same view by default. Supply
the source filename when rendering an editor buffer so relative links resolve from that document:

```cpp
markdownView->loadFromString( markdown, "/project/README.md" );
// Or load the document from disk (uses the scene's worker pool when available):
markdownView->loadFromFile( "/project/README.md" );
```

Both `docs/intro.md` and `file://docs/intro.md` resolve relative to the source document;
`file:///project/docs/intro.md` is absolute. Subsequent links resolve from the newly loaded document.
Without a source filename, links use the enclosing scene's base URI. Copy Link uses the same
resolution. `getDocumentPath()` reports the displayed source filename. Failed reads preserve the
current document and emit `OnNavigationError`; successful reads emit `OnNavigationCompleted`, both
with `UIMarkdownView::NavigationEvent`. Replacing content cancels pending navigation.

Call `setFollowLocalLinks(false)` to disable default local navigation. Every link first emits
`OnLinkOpenRequested`, whose `UIMarkdownView::LinkOpenEvent::request` contains the resolved URI,
source node, mouse buttons, modifiers, and target. A consumer can open a new tab or implement another
flow and call `accept()` to prevent default handling. Middle click and the platform modifier plus
left click request `NavigationRequest::Target::NewTab`; consumers can also call `navigate()` with
an explicit target. Unaccepted local new-tab requests open in the same view. Other links continue
through the enclosing scene's navigation handler. No tab UI or navigation history is built into
the Markdown view.

Markdown views register their navigation handlers with
`UISceneNode::setNavigationInterceptorCb(root, callback)`, scoped to their own widget subtree.
Scoped callbacks run from the closest scope outward; returning `false` falls through to the next
scope and then to the scene-wide interceptor. Registering a scope does not replace the scene-wide
callback. A scope must belong to the scene's tree when registered. An empty callback unregisters
a scope, including one that has already moved out of the tree. The Markdown view manages registration
when it moves between scenes or is destroyed. The scoped overload returns `true` for registration,
replacement, or removal, and `false` for invalid roots or removal of a missing registration.

`UIMarkdownView` uses the same shortcuts through its enclosing `UIScrollView`; disable them on
that scroll view with `setEnableDefaultKeybindings(false)`.

`UIScrollableMarkdownView` provides the scroll container and owns a `UIMarkdownView` child. It is
declared in `uimarkdownview.hpp` and can also be created as `<ScrollableMarkdownView>` in XML layouts,
with inline Markdown text or CDATA. Configure the child through `getMarkdownView()`:

```cpp
auto* documentView = UIScrollableMarkdownView::New();
documentView->setParent( parent );
documentView->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
documentView->getMarkdownView()->loadFromFile( "/project/README.md" );
```

Successful file navigation resets both scrollbars to the start. String updates and failed loads
preserve scrolling. The wrapper implements `WidgetCommandExecuter`: register custom commands with
`setCommand()` and shortcuts with `getKeyBindings()`. Unhandled keys use the scroll view's default
shortcuts. Loading, link policy, selection, and navigation events belong to the Markdown child;
editor binding and session persistence remain application responsibilities.

History navigation is disabled by default. Call `setHistoryNavigationEnabled(true)` to enable it
and start history at the current named Markdown document. Disabling clears history and unregisters
the history commands and shortcuts; loading documents and following local links remain available.
Ecode explicitly enables history for its Markdown previews.

When enabled, the scrollable wrapper maintains history for named Markdown documents.
`goHistoryBack()` and `goHistoryForward()` traverse it; `canGoBack()` and `canGoForward()` report
available directions.
The `go-back` and `go-forward` commands default to `mod+[` and `mod+]`, using the platform's default
modifier. Once a direction is available, the document's context menu adds Go Back and Go Forward,
enabling each as appropriate. A successful new navigation discards the forward branch; updating the
current string buffer preserves it. Failed and canceled loads do not advance history. Traversal
reloads files by default; applications can override `loadHistoryDocument()` to render their current
buffer instead. Ecode uses this for its live source, including unsaved edits. History is kept for
the lifetime of the widget and is cleared when it loads an anonymous string document.

Default scrolling shortcuts are Space and Page Down (one viewport down), Shift+Space and Page Up
(one viewport up). Keys handled by a focused descendant do not reach the WebView. While text input is active,
Space scrolls only if its committed text reaches the WebView unhandled; custom controls can consume
it through `onTextInput()` like built-in editors. Call `setEnableDefaultKeybindings(false)` to disable scrolling
shortcuts, including the inherited arrow and Home/End bindings.

Anchor interaction also cooperates with document text selection so a completed text drag does not
accidentally activate the link.

---

## 26. Forms

The compatibility layer includes basic form support.

Supported form-oriented widgets currently include:

```text
form
input
textarea
button
label
```

`UIHTMLInput` hosts native eepp controls internally.

Recognized input types include:

```text
button
checkbox
color
date
datetime-local
email
file
hidden
image
month
number
password
radio
range
reset
search
submit
tel
text
time
url
week
```

Not every type has a specialized native implementation. Unsupported/specialized types fall back to
the generic text-input implementation unless explicitly mapped otherwise.

Current explicit native mappings include:

```text
button / submit / reset -> UIPushButton
checkbox                -> UICheckBox
hidden                  -> no visible child
date                    -> UIDatePicker
time                    -> UITimePicker
datetime-local          -> UIDateTimePicker
number                  -> UISpinBox
password                -> password text input
radio                   -> UIRadioButton
other text-like types   -> UIHTMLTextInput
```

Form controls use light user-agent defaults from `getHTMLDocumentDefaultsCSS()` in
`uiwidgetcreator.cpp`, independently of the application's native theme. Author CSS styles the
HTML control host; anonymous implementation parts retain their private defaults. Temporal controls
have intrinsic text/placeholder sizing, so they remain usable without an authored width.

`textarea` uses a `UITextEdit`-based implementation with `rows` and `cols` intrinsic sizing.

Forms collect named control values and can submit navigation requests.

Temporal inputs sanitize invalid value strings to empty and submit normalized ISO values independently
of their display locale. They support `min`, `max`, `step`, `required`, `readonly`, and `disabled`.
Range validation never silently changes the supplied value; reversed time ranges wrap midnight.
`UIHTMLInput::getValidity()` exposes the implemented temporal validity flags. `UIHTMLForm::requestSubmit()`
and submit-button activation validate these controls, while direct `submit()` bypasses validation.
Disabled temporal controls are excluded from form data, and named empty temporal controls submit an
empty string. See [date/time pickers](ui_date_time_pickers.md) for APIs and the supported subset.

An omitted or empty form `action` submits to the full current document URL, preserving its query.

Supported submission encodings include:

```text
application/x-www-form-urlencoded
multipart/form-data
text/plain
```

GET and POST submission paths are implemented.

---

## 27. Labels

`<label for="...">` is interactive.

The label resolves the referenced widget and can activate supported controls such as checkboxes and
radio buttons.

Keyboard activation is also supported.

This behavior is implemented natively rather than through JavaScript.

---

## 28. Images and SVG

HTML images use `UIHTMLImage`.

They participate as replaced elements with intrinsic sizing and CSS width/height behavior.

`<svg>` is backed by `UISvg` marked as an HTML element so it can participate in HTML sizing/layout.

Resource URLs can be:

```text
file URLs
HTTP/HTTPS URLs
data URLs where supported
eepp resource locators
```

Relative document/style resources are resolved against the relevant document or stylesheet base URI.

---

## 29. Stylesheets and resource loading

HTML documents can contain:

```html
<style>
...
</style>
```

and stylesheet links:

```html
<link rel="stylesheet" href="...">
```

`UIWebView` can load local and HTTP/HTTPS stylesheets.

Relative stylesheet resource URLs are resolved against the stylesheet/document base URI when one is
available.

Supported stylesheet at-rules include those documented in
[`css_specification.md`](css_specification.md), including:

```text
@import
@media
@font-face
@keyframes
```

`@font-face` can load local, data-backed, and HTTP/HTTPS font resources through the document scene.

---

## 30. Media queries

The same media-query engine used elsewhere in eepp is available to HTML document styles.

Supported features are documented in
[`css_specification.md`](css_specification.md).

Document layout should still rely primarily on normal CSS layout mechanisms such as:

```text
block flow
flex wrapping
grid tracks
intrinsic sizing
```

rather than reproducing layout entirely with breakpoint-specific absolute dimensions.

---

## 31. HTML visibility and document extent inside `UIWebView`

`UIWebView` maintains document viewport and content extent separately.

The document scene is updated when:

- the web view changes size;
- scrollbars appear/disappear;
- HTML layout changes;
- resources load and alter geometry;
- viewport-dependent percentages need recomputation.

The root `html` and `body` boxes are synchronized with the document viewport while still allowing
content to grow beyond it.

Out-of-flow content can contribute to the final scrollable document extent when appropriate.

Fixed-position content is treated separately from normal scrollable document extent.

---

## 32. Root `html` / `body` behavior is special

The root document boxes have browser-oriented special handling.

`body` maintains a minimum height based on:

```text
viewport height
its local min-height
document content extent
```

The root `html` box is kept at least large enough to cover the document viewport/content.

The body's background may propagate to the root HTML element when the root background is otherwise
transparent.

These rules should not be inferred from ordinary native `UILayout` behavior.

---

## 33. Paint order and hit testing follow the HTML layer

HTML positioned/floating content can be painted in an order different from raw widget child order.

Hit testing follows the corresponding HTML paint traversal when required.

This matters for:

```text
z-index
positioned descendants
floats
overlapping HTML content
flex/grid order
```

Do not debug overlapping HTML content by looking only at the native child insertion order.

Use the actual HTML layout/paint state.

---

## 34. Structural CSS selectors can react to document structure

The selector engine supports structural selectors such as:

```text
:first-child
:last-child
:nth-child(...)
:nth-last-child(...)
:first-of-type
:last-of-type
:nth-of-type(...)
:nth-last-of-type(...)
:only-child
:only-of-type
:empty
:not(...)
```

Structural and relationship-dependent selectors are treated as volatile where needed so relevant
widgets can be restyled when structure/state changes.

This is especially important for HTML-like document styling.

---

## 35. CSS state rollback still uses eepp's common style engine

HTML widgets share the same `UIStyle` implementation as native widgets.

That means the current limitation around properties that stop matching a pseudo-class/volatile
selector also applies to HTML content.

eepp currently preserves prior values through a best-effort stateless/current-value fallback rather
than a complete browser-style computed-cascade rollback model.

---

## 36. Compatibility does not mean complete browser parity

The HTML layer intentionally implements a useful subset of browser layout and interaction behavior.

It should not be described as a fully compliant browser engine.

Examples of known boundaries include:

```text
no JavaScript runtime
only registered HTML elements are materialized
overflow does not create arbitrary per-element scroll containers
overflow-x / overflow-y are not independent yet
visibility semantics are not fully browser-equivalent
relative positioning should not be assumed to implement every browser offset behavior
stacking/paint order is a supported subset
form controls map onto native eepp widgets
CSS state rollback is not a complete computed-style rollback model
```

Other browser features should be considered supported only when they are implemented and tested in
eepp.

When compatibility behavior matters, the implementation and HTML-specific unit tests are the source
of truth.

---

## 37. Where to look in the source

The main implementation is concentrated in:

```text
HTML host / navigation:
    include/eepp/ui/uiwebview.hpp
    src/eepp/ui/uiwebview.cpp

HTML widget semantics:
    include/eepp/ui/uihtmlwidget.hpp
    src/eepp/ui/uihtmlwidget.cpp

HTML parsing:
    include/eepp/ui/tools/htmlformatter.hpp
    src/eepp/ui/tools/htmlformatter.cpp

formatting contexts:
    src/eepp/ui/blocklayouter.cpp
    src/eepp/ui/inlinelayouter.cpp
    src/eepp/ui/flexlayouter.cpp
    src/eepp/ui/gridlayouter.cpp
    src/eepp/ui/tablelayouter.cpp

text:
    src/eepp/ui/uirichtext.cpp
    src/eepp/ui/uitextspan.cpp

HTML widgets:
    src/eepp/ui/uihtmlimage.cpp
    src/eepp/ui/uihtmlinput.cpp
    src/eepp/ui/uihtmltextarea.cpp
    src/eepp/ui/uihtmlform.cpp
    src/eepp/ui/uihtmltable.cpp
    src/eepp/ui/uihtmldetails.cpp
    src/eepp/ui/uihtmllistitem.cpp

Markdown:
    src/eepp/ui/uimarkdownview.cpp

HTML element registration/default CSS:
    src/eepp/ui/uiwidgetcreator.cpp
```

The most useful tests are under:

```text
src/tests/unit_tests/uihtml_tests.cpp
src/tests/unit_tests/uihtml_flex_test.cpp
src/tests/unit_tests/uihtml_grid_test.cpp
src/tests/unit_tests/uihtml_float_tests.cpp
src/tests/unit_tests/uihtml_position_tests.cpp
src/tests/unit_tests/uihtmlform_tests.cpp
src/tests/unit_tests/uiwebview_tests.cpp
```

---

## 38. Related documentation

- [`ui_introduction.md`](ui_introduction.md) — introduction to the eepp UI system.
- [`ui_authoring.md`](ui_authoring.md) — recommended native application UI mental model.
- [`ui_layout_reference.md`](ui_layout_reference.md) — exact native layout semantics.
- [`ui_css_for_applications.md`](ui_css_for_applications.md) — CSS conventions for ordinary
  application widgets.
- [`css_specification.md`](css_specification.md) — CSS property, selector, value, and at-rule
  reference.
- [`ui_inspector.md`](ui_inspector.md) — runtime UI inspection and automation protocol.
- [`ui_databinding.md`](ui_databinding.md) — UI data binding.
- [`ui_charts.md`](ui_charts.md) — charting widgets and chart-specific behavior.

The practical rule is simple:

> Use native eepp layouts to build applications. Use the HTML compatibility layer when the content
> itself is HTML/Markdown/document-oriented.
