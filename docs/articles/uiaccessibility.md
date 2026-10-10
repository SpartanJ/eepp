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

To work on the implementation itself, see [UI accessibility architecture](uiaccessibilityarchitecture.md).

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

Text queries read only the range a client asks for, through a line index that is rebuilt once per
edit, so moving the caret in a large file does not copy the file. Edits reach Linux clients as exact
insert and delete records taken from the document. A wholesale replacement (a reset, a reload, or
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

Custom widgets should use the normal eepp state and action mechanisms whenever possible. Native
backends query the shared accessibility resolver; platform-specific accessibility behavior should
not be implemented inside individual widget classes.

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

The first implementation focuses on the semantics used by standard eepp desktop widgets. Native
platform APIs contain additional advanced interfaces that are not yet exposed uniformly, including
some rich-document formatting, complex table metadata, and container scroll patterns (scroll
position and paging, as opposed to scrolling an element into view). HTML content (`UIRichText`,
headings, links, lists) is not yet exposed as semantic elements. Relations are used to compute names
and descriptions but are not published as native relation sets.

Do not advertise a native pattern or capability unless its required behavior is implemented.
Assistive technologies can still navigate the semantic hierarchy and operate the supported
controls when an optional advanced pattern is absent.

The implementation currently targets Linux, macOS, and Windows desktop applications. Mobile and
web accessibility bridges are outside the current scope.
