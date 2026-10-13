# UI Accessibility

eepp can expose its UI widget hierarchy to the native accessibility services on Linux, macOS, and
Windows. This allows screen readers, accessibility inspectors, and automation clients to discover
widgets, read their state and values, follow focus, and perform supported actions.

The native backends are:

- AT-SPI on Linux;
- `NSAccessibility` on macOS;
- Microsoft UI Automation (UIA) on Windows.

Other platforms currently use the null backend. Accessibility does not change how widgets are
rendered and does not require the application to redraw continuously.

To work on the implementation itself, see [UI accessibility architecture](ui_accessibility_architecture.md).

## Enabling accessibility

Native accessibility is enabled by default on supported platforms. Applications using
`UIApplication` can select the policy in `UIApplication::Settings`:

```cpp
UIApplication::Settings settings;
settings.accessibilityPolicy = AccessibilityPolicy::Auto;

UIApplication app( { 1280, 720, "Accessible application" }, settings );
```

The available policies are:

- `AccessibilityPolicy::Auto`: use the platform default, which currently enables the native
  backend on supported desktop platforms;
- `AccessibilityPolicy::Enabled`: explicitly request the native backend;
- `AccessibilityPolicy::Disabled`: use the null backend and avoid native integration.

The `EEPP_DISABLE_ACCESSIBILITY` environment variable overrides `Auto` and `Enabled` and forces the
null backend. This is useful for diagnostics and performance comparisons:

```sh
EEPP_DISABLE_ACCESSIBILITY=1 ./my-application
```

A root `UISceneNode` can also be configured directly:

```cpp
sceneNode->setAccessibilityPolicy( AccessibilityPolicy::Disabled );
```

Only root UI scenes own a native accessibility manager. Nested scenes participate through their
host root scene, and every application-owned top-level window is exposed through its own native
window representation.

## Automatic widget semantics

Standard eepp widgets receive accessibility semantics automatically. This includes common
controls such as:

- buttons;
- checkboxes and radio buttons;
- text labels, text inputs, and multiline text editors;
- combo boxes;
- sliders, spin boxes, and progress bars;
- tabs;
- menus and menu items;
- lists, tables, and trees;
- UI windows.

The framework resolves each widget's role, name, state, value, available actions, range, and text
selection from the live UI. Applications normally do not need to construct or synchronize a
separate accessibility tree.

Model-backed list, table, and tree rows are represented as virtual accessible nodes, so they do not
need to be materialized as `UIWidget` objects merely for accessibility.
While a cell is being edited, its editor widget is exposed as the only child of the virtual row
(lists and trees) or cell (tables) it edits, and keyboard focus is reported on the editor.

Text editing support covers `UITextInput`, `UITextEdit` and `UICodeEditor`, including caret and
selection queries/actions and native change notifications. A code editor with a file is named after
that file unless it has a label. Setting its text through an assistive client replaces the document
contents as one undoable edit, so the file path and undo history are kept.

Text queries read only the range a client asks for, through a lazily created line index. Single-line
edits update it incrementally; line insertions and removals update affected lengths and rebuild
block totals. Resets and missed changes invalidate it for a lazy rebuild. Moving the caret in a
large file does not copy the file. Edits reach Linux clients as exact insert and delete records
taken from the document. A wholesale replacement (a reset, a reload, or
many edits in one frame, such as replace-all) is reported once as "everything removed, everything
inserted". Windows and macOS need UTF-16 offsets, so they convert the text once per edit and reuse
that copy for every query until the next edit.

Controls whose content is their own name, value and state (buttons, check boxes, radio buttons,
labels, tabs, menu items, sliders, spin boxes, progress bars and images) do not expose their
internal widgets, so a button's text is read once, as its name, and not again as a child label.

Menu items expose their keyboard shortcut (AT-SPI key binding, UIA `AcceleratorKey`, macOS menu
command character and modifiers). Any widget inside a `UIScrollView` offers a scroll-into-view
action, which scrolls every enclosing scroll view the least distance needed. The same behavior is
available to applications as `UIScrollView::scrollIntoView( node )`.

## Accessible names and descriptions

Use `aria-label` when a widget's visible content does not provide a useful name. Use
`aria-description` for additional instructions or context:

```xml
<TextInput id="project-name"
           aria-label="Project name" />

<TextEdit id="description"
          aria-label="Description"
          aria-description="Multiline editor. Press Control Tab to move to the next control." />
```

When the name is visible elsewhere, point at it instead of repeating it. `aria-labelledby` names a
widget after the widget with that id, and `aria-describedby` uses another widget's text as the
description. Both ids are resolved from the scene root. When the referenced widget's text changes,
or it is added, removed or given another id, assistive clients are told that the dependent widget's
name or description changed:

```xml
<TextView id="user-label" text="User name" />
<TextInput aria-labelledby="user-label" aria-describedby="user-hint" />
<TextView id="user-hint" text="The name you sign in with" />
```

Names resolve in this order: `aria-labelledby`, `aria-label`, the widget's own text (button text,
label text, window title, code editor file name), then its tooltip. An icon-only button with a
tooltip is therefore named by the tooltip. A tooltip that did not become the name is used as the
description when no other description is set.

Use `aria-hidden="true"` for decorative or redundant widgets that should not appear in the native
accessibility hierarchy. It excludes the whole subtree and silences its native change events.
Hidden labels still update visible controls that reference them with `aria-labelledby` or
`aria-describedby`:

```xml
<Image src="decorative-separator"
       aria-hidden="true" />
```

These properties are also available through C++:

```cpp
widget->setAccessibilityLabel( "Project name" );
widget->setAccessibilityDescription( "Enter the project display name." );
widget->setAccessibilityHidden( false );
widget->setAccessibilityLabelledBy( "user-label" );
widget->setAccessibilityDescribedBy( "user-hint" );
```

Setting a property to its current value does nothing, so re-applying styles is free.

Prefer a concise label that identifies the control. A description should add information rather
than repeat the label or visible text.

## Custom widgets

A custom widget derived from a standard control normally inherits that control's automatic
semantics. The resolver uses widget-family checks, so subclasses do not need to duplicate
accessibility implementations.

For a new semantic control, set an explicit role and supply a useful label when one cannot be
derived from visible content:

```cpp
widget->setAccessibilityRole( AccessibilityRole::Button );
widget->setAccessibilityLabel( "Refresh projects" );
```

An explicit `AccessibilityRole::None` can keep a custom widget out of the semantic hierarchy. Use
`setAccessibilityHidden( true )` when the widget and its accessible descendants should be hidden
from assistive technology.

Custom widgets can selectively override `UIWidget`'s virtual accessibility methods: role, name,
description, value, range, state, actions and action execution. The default implementations remain
in the central resolver; existing standard controls need no additional overrides. The manager and
native backends use the effective widget semantics, including overridden leaf roles for hierarchy
and focus projection.

For example, a custom button can override `getAccessibilityRole()`, advertise `Press` from
`getAccessibilityActions()`, and handle it in `performAccessibilityAction()`. It need not derive
from `UIPushButton`. Calling the base implementation retains standard state or actions when useful;
the default action handler checks the effective action mask, including custom overrides.

Custom widgets should use the normal eepp state, action and notification mechanisms whenever
possible. Platform-specific accessibility behavior should not be implemented inside widget classes.

## Dynamic state and notifications

Standard widgets notify the accessibility manager when relevant UI state changes, including:

- keyboard focus;
- names and descriptions;
- values and text selection;
- enabled and visible state;
- checked, selected, and expanded state;
- widget creation and destruction;
- model and child hierarchy changes.

Bounds are queried on demand. Routine position and size changes do not emit per-widget native
layout notifications; this avoids event floods while scrolling or running layout. A window resize is
announced once, on the window root.

### Announcements and live regions

Use an announcement for an event the user should hear without moving focus, such as a finished
build:

```cpp
sceneNode->announceForAccessibility( "Build finished" );
sceneNode->announceForAccessibility( "Build failed", AccessibilityLive::Assertive );
```

`Polite` waits until the screen reader finishes speaking; `Assertive` interrupts it. Announcements
use UIA notifications (Windows 10 1709 and later), the AT-SPI `object:announcement` signal
(at-spi2-core 2.46 and later, read by Orca) and `NSAccessibilityAnnouncementRequestedNotification`
on macOS. Older systems ignore them.

For text that changes in place, such as a status bar, mark it as a live region instead:

```xml
<TextView id="status" text="Ready" aria-live="polite" />
```

Name and value changes of a widget inside a live region (`polite` or `assertive`) are announced,
unless the widget or one of its ancestors is `aria-hidden`.
Several changes to the same widget in one frame produce one announcement with the final text, so a
progress label updated every frame does not flood the screen reader. Both APIs are free while no
assistive client is active.

Applications using the standard widget APIs receive these notifications automatically. A custom
control that changes semantic state outside the standard paths may notify its scene accessibility
manager explicitly, but it should do so only after the new state is queryable.

Accessibility information is resolved on demand. Native provider objects identify live nodes and
become unavailable safely after the corresponding widget, model item, scene, or window is
destroyed.

## Keyboard navigation

Accessible controls should be keyboard focusable and have a logical focus order. Standard Tab and
Shift+Tab navigation moves between controls.

`UIWidget` and informational/structural widgets are **not tabbable by default**. Buttons,
checkboxes, radios, text editors, sliders and list/model views opt in. Static labels, images,
progress indicators and layouts still retain their accessibility semantics; Tab eligibility is
not accessibility visibility and does not prevent mouse or programmatic focus.

Migration: custom interactive `UIWidget` subclasses must call `setTabFocusable()` in their
constructor, or declare `focusable="true"` in XML / `focusable: true` in CSS. Explicit
`focusable=false` and `unsetTabFocusable()` remain supported. Non-tabbable containers remain
traversable, so their interactive descendants are reachable in both directions.

Composite defaults use the working keyboard target:

- Spinboxes and combos enter through their embedded text input/dropdown, not the outer host.
  Native accessibility Focus actions target that editor, preserving existing semantic identities
  and focus projection. Spinbox Up/Down keys adjust the value from that editor.
- HTML inputs forward explicit XML/CSS `focusable` policies to their anonymous native control,
  preserving the policy when the input type changes. The HTML host is not a second Tab stop.
- Dropdowns and date pickers open their popup with F4 or Alt+Down; Escape returns to the editor.
  Tab/Shift+Tab leave a dropdown popup relative to its field, not its temporary scene-root
  position. The date picker's calendar button is not an additional Tab stop.
- Lists enter through the list itself; rows and recycled model cells are not individual stops.
  A live cell editor can still receive focus. Table header buttons retain their existing keyboard
  sorting entry until the view has an equivalent header-navigation interface.
- A calendar enters through its root and navigates dates with its existing arrow and paging keys.
  Day/month/year cells are not stops. Previous/title/next/Today buttons remain reachable, preserving
  operations not completely covered by the root's key commands.
- Tabs and radio buttons remain individually tabbable. Roving tablists/radio groups are deferred;
  inactive members must remain keyboard-reachable until those patterns are implemented.
- Menu-bar buttons remain tabbable; popup items use the menu's arrow navigation instead of joining
  the ordinary Tab ring.

Arrow-navigated list rows and popup menus/menu items retain native `Focusable` state and Focus
actions even though they are not Tab stops. Sequential eligibility is not general keyboard
focusability.

Scrollbars and scroll/document/viewer containers do not automatically enter Tab order. Opt a
keyboard-operable reading region in explicitly with `focusable=true`. For a standalone scrollbar,
opt in its `getSlider()` target, not both host and slider; that target already handles arrows and
PageUp/PageDown. Overflow, image loading and pager locking do not dynamically alter these defaults.

Links loaded from XML/HTML opt in when their initial `href` is nonempty, before author focusable
properties are applied. Enter/keypad Enter activates span links through the same navigation
interception path as mouse clicks. Runtime `setHref()` deliberately preserves the focus flag:
applications creating links programmatically, or removing/readding destinations, should explicitly
set/unset Tab focus as appropriate. Automatic runtime recomputation is deferred because it cannot
distinguish an author override from a default without additional bookkeeping.

Multiline text editors reserve Tab for inserting indentation. Use Control+Tab to move to the next
focusable widget and Control+Shift+Tab to move to the previous one. This matches AppKit on macOS,
where Command+Tab is reserved for switching applications.

Screen readers may provide an additional browse or navigation mode. Their commands and speech
settings are controlled by the screen reader and are separate from eepp's keyboard focus handling.

## Testing an application

The `eepp-ui-accessibility` example contains representative controls, model-backed views, dynamic
metadata, a live region, and multiple native windows. Build and run the optimized release target
before using a platform accessibility tool.

The widget inspector (`UIWidgetInspector`) has an Accessibility tab with the selected widget's
resolved role, name, description, value, shortcut, relations, states and actions, and an Audit tab
that lists focusable controls without an accessible name and `aria-labelledby` /
`aria-describedby` ids that match nothing. Selecting an issue selects the widget in the tree. The
same check is available to tests as `AccessibilityWidgetResolver::audit( root )`.

The Accessibility table uses proportional column widths and previews the first 1,024 code points
of a value. Longer values are labelled **Value (preview)** with a truncation marker. Editor previews
are read directly from a bounded document prefix and reused on non-text events while the document's
identity and revision are unchanged. **Copy full value** explicitly copies the complete accessible
value to the clipboard; large documents may take time. Password inputs expose no value. These
Inspector-only limits do not truncate the text or values exposed to native accessibility clients.

### Linux

Use Orca for a screen-reader pass. Accerciser and desktop accessibility inspectors can inspect the
AT-SPI hierarchy and events. The repository provides automated validation and latency measurement:

```sh
python3 projects/scripts/test_atspi.py --executable bin/eepp-ui-accessibility
python3 projects/scripts/benchmark_atspi.py --executable bin/eepp-ui-accessibility
```

These commands need an active AT-SPI session bus and a usable graphical session.

Pass `--hidden` to `test_atspi.py` to keep all example windows hidden. Add `--multi-window` to
validate secondary-window closure, or `--close-primary` to validate that the surviving window
continues responding after the primary window is destroyed.
The `--close-primary` validation also closes the remaining window with its AT-SPI client still
connected and checks that the application exits cleanly.

### macOS

Use VoiceOver and Accessibility Inspector. The repository's native semantic and lifecycle harness
requires Accessibility permission for the process running Swift:

```sh
swift projects/scripts/test_macos_accessibility.swift \
  --executable bin/eepp-ui-accessibility
```

The harness validates native discovery, properties, actions, text, notifications, model changes,
stale elements, and multiple-window lifecycle behavior.

### Windows

Use Narrator and Accessibility Insights for Windows. Windows SDK Inspect and AccEvent are useful
secondary diagnostics. The native UIA harness is built with the Windows targets:

```bat
bin\unit_tests\eepp-windows-accessibility-tests.exe bin\eepp-ui-accessibility.exe
```

The harness validates late discovery, fragment navigation, UIA properties and control patterns,
UTF-16 text ranges, events, hit testing, provider invalidation, and repeated multi-window creation
and destruction.

## Performance behavior

Native accessibility backends are query-centered and create provider objects lazily. When no
native client has interacted with the application, widget notifications take an inline inactive
fast path: one atomic load, no call, no allocation, and no accessibility data resolved. Editing
text, building widgets (spin boxes included), deleting widget hierarchies and updating models
allocate no more than they would without accessibility, as measured by C++ `new` on the UI thread
(see the architecture guide's benchmark notes). When the last client disconnects, rows it
queried are released, so model updates stop paying for them; a client connecting later starts
fresh.

Model child counts and individual child queries do not instantiate every accessible row. Queried
rows retain identities across insertion and removal, and model sibling indexes are resolved
without enumerating every preceding row. A client explicitly requesting all children still pays
for that enumeration; the logical model is not restricted to its visible viewport.

Linux tracks querying clients by their accessibility-bus connection and becomes inactive when
those connections disappear. Windows requires window-specific client activity rather than global
UIA listeners alone. macOS does not provide equivalent per-client disconnect tracking, so after a
native query its observation flag remains enabled for the application's lifetime.
Windows can also retain the observation flag for a previously queried window while global UIA
listeners remain connected: the global listening API does not identify which window they use.

Use `EEPP_DISABLE_ACCESSIBILITY=1` as the disabled baseline when investigating a suspected
regression. Always profile optimized release binaries without AddressSanitizer. Accessibility work
must not depend on rendering, and native queries or notifications must never stall the render loop.

## Current scope

### Basic HTML semantics

HTML and Markdown content rendered by eepp now exposes source text, `h1`–`h6` headings with their
levels, anchors with nonempty `href`, and `ul`/`ol` lists with their `li` items. Images with nonempty
alternative text expose that text. Anonymous form controls retain their native widget semantics.

Ordinary block and formatting wrappers are transparent. Text is exposed through the text nodes
and spans that own it, rather than copying each enclosing paragraph's flattened rendering. Headings
and links have names derived from their visible textual contents; they remain containers so nested
links and controls are reachable. Input values, including passwords, never contribute to those
names. `aria-hidden` content is excluded from traversal and descendant-name calculation.

Link Press/Invoke actions use the existing navigation path, including WebView/Markdown interception
and relative URI resolution. Merely focusing a link never activates it. Focus on a passive text or
image fragment inside a link is reported on the link; embedded interactive controls keep their own
focus. Runtime `setHref()` continues to preserve the application's explicit Tab-focus policy.

This is basic object navigation, not a complete browser/document accessibility implementation.
Static HTML text is exposed by name/value; it does not advertise unimplemented Text/Hypertext
interfaces, document-wide cursor navigation, formatting ranges or cross-element text selection.
HTML lists are not selectable model views and do not advertise native Selection patterns.

The first implementation focuses on the semantics used by standard eepp desktop widgets. Native
platform APIs contain additional advanced interfaces that are not yet exposed uniformly, including
some rich-document formatting, complex table metadata, and container scroll patterns (scroll
position and paging, as opposed to scrolling an element into view). Relations are used to compute
names and descriptions but are not published as native relation sets.

Do not advertise a native pattern or capability unless its required behavior is implemented.
Assistive technologies can still navigate the semantic hierarchy and operate the supported
controls when an optional advanced pattern is absent.

The implementation currently targets Linux/FreeBSD, macOS, and Windows desktop applications. Mobile and
web accessibility bridges are outside the current scope.
