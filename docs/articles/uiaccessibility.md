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

## Accessible names and descriptions

Use `aria-label` when a widget's visible content does not provide a useful name. Use
`aria-description` for additional instructions or context:

```xml
<TextInput id="project-name"
           aria-label="Project name" />

<TextEdit id="description"
          aria-label="Description"
          aria-description="Multiline editor. Press Mod Tab to move to the next control." />
```

Use `aria-hidden="true"` for decorative or redundant widgets that should not appear in the native
accessibility hierarchy:

```xml
<Image src="decorative-separator"
       aria-hidden="true" />
```

These properties are also available through C++:

```cpp
widget->setAccessibilityLabel( "Project name" );
widget->setAccessibilityDescription( "Enter the project display name." );
widget->setAccessibilityHidden( false );
```

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
- model and child hierarchy changes;
- bounds and layout changes.

Applications using the standard widget APIs receive these notifications automatically. A custom
control that changes semantic state outside the standard paths may notify its scene accessibility
manager explicitly, but it should do so only after the new state is queryable.

Accessibility information is resolved on demand. Native provider objects identify live nodes and
become unavailable safely after the corresponding widget, model item, scene, or window is
destroyed.

## Keyboard navigation

Accessible controls should be keyboard focusable and have a logical focus order. Standard Tab and
Shift+Tab navigation moves between controls.

Multiline text editors reserve Tab for inserting indentation. Use Mod+Tab to move to the next
focusable widget and Mod+Shift+Tab to move to the previous one. `Mod` means Command on macOS and
Control on other desktop platforms.

Screen readers may provide an additional browse or navigation mode. Their commands and speech
settings are controlled by the screen reader and are separate from eepp's keyboard focus handling.

## Testing an application

The `eepp-ui-accessibility` example contains representative controls, model-backed views, dynamic
metadata, and multiple native windows. Build and run the optimized release target before using a
platform accessibility tool.

### Linux

Use Orca for a screen-reader pass. Accerciser and desktop accessibility inspectors can inspect the
AT-SPI hierarchy and events. The repository provides automated validation and latency measurement:

```sh
python3 projects/scripts/test_atspi.py --executable bin/eepp-ui-accessibility
python3 projects/scripts/benchmark_atspi.py --executable bin/eepp-ui-accessibility
```

These commands need an active AT-SPI session bus and a usable graphical session.

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
native client has interacted with the application, widget notifications take a cached inactive
fast path and avoid resolving accessibility data or walking the widget hierarchy.

Use `EEPP_DISABLE_ACCESSIBILITY=1` as the disabled baseline when investigating a suspected
regression. Always profile optimized release binaries without AddressSanitizer. Accessibility work
must not depend on rendering, and native queries or notifications must never stall the render loop.

## Current scope

The first implementation focuses on the semantics used by standard eepp desktop widgets. Native
platform APIs contain additional advanced interfaces that are not yet exposed uniformly, including
some rich-document formatting, complex table metadata, and container-specific scrolling APIs.

Do not advertise a native pattern or capability unless its required behavior is implemented.
Assistive technologies can still navigate the semantic hierarchy and operate the supported
controls when an optional advanced pattern is absent.

The implementation currently targets Linux, macOS, and Windows desktop applications. Mobile and
web accessibility bridges are outside the current scope.
