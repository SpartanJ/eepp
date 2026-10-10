# UI Accessibility Architecture

This article is for people working on eepp's accessibility implementation. For the application
facing API (roles, labels, policies and testing an application) read
[UI accessibility](uiaccessibility.md) first.

The implementation has two halves:

- a small, platform independent **core** that projects the live widget tree into an accessible
  tree on demand;
- three **native backends** that translate that projection to AT-SPI (Linux/FreeBSD),
  NSAccessibility (macOS) and UI Automation (Windows).

Most day to day work (new widgets, new roles, lifecycle fixes) happens in the core. The backends
mostly change when a platform feature is added.

## Map

```text
 Screen reader / inspector / automation client
                    |
     AT-SPI (D-Bus)  |  NSAccessibility  |  UI Automation (COM)
                    |
 +------------------v-------------------------------------------+
 | Native backend (one per platform)        accessibilitybackend*|
 |  - answers native queries by calling the manager             |
 |  - turns manager events into native notifications            |
 +------------------+-------------------------------------------+
                    | AccessibilityNodeRef, AccessibilityNodeInfo,
                    | AccessibilityPendingEvent (accessibility.hpp)
 +------------------v-------------------------------------------+
 | AccessibilityManager (one per top-level UISceneNode)          |
 |  - node identities, tree navigation, focus, hit testing       |
 |  - event queue, teardown and ownership rules                  |
 +--------+-------------------------+---------------------------+
          |                         |
 +--------v-----------+   +---------v------------------------------+
 | Widget resolver    |   | AccessibilitySource (virtual nodes)    |
 | role/name/state/   |   |  AccessibilityModelViewSource: rows,   |
 | actions/text of a  |   |  cells and the cell editor of list,    |
 | live UIWidget      |   |  table and tree views                  |
 +--------^-----------+   +----------------------------------------+
          |
 UIWidget hooks: notifyAccessibilityEvent(), ~UIWidget, Node::childRemove,
 UISceneNode lifecycle, widget-specific state changes
```

### Files

| Area | Files | What lives there |
| --- | --- | --- |
| Public types | `include/eepp/ui/accessibility/accessibility.hpp` | Roles, states, actions, events, `AccessibilityNodeRef`, `AccessibilityNodeInfo`. |
| Manager | `accessibilitymanager.{hpp,cpp}` | Identities, navigation, focus, hit testing, `notify()`, teardown. |
| Widget semantics | `accessibilitywidgetresolver.{hpp,cpp}` | Role, name, value, range, text, state and actions for each standard widget type; focus projection; event routing. |
| Virtual nodes | `accessibilitysource.hpp`, `accessibilitymodelviewsource.{hpp,cpp}` | The source interface and its model-view implementation. |
| Backend interface | `src/eepp/ui/accessibility/accessibilitybackend.{hpp,cpp}` | `AccessibilityBackend`, the null backend, shared helpers. |
| AT-SPI | `accessibilitybackendatspi.hpp` and `accessibilitybackendatspi{,events,tree,methods}.cpp` | See [AT-SPI](#at-spi-linux-and-freebsd). |
| UI Automation | `accessibilitybackenduia.hpp` and `accessibilitybackenduia{,provider,text}.cpp` | See [UI Automation](#ui-automation-windows). |
| NSAccessibility | `accessibilitybackendmacos.mm` | See [NSAccessibility](#nsaccessibility-macos). |
| Widget hooks | `uiwidget.cpp`, `uiscenenode.cpp`, `scene/node.cpp`, and the individual widgets | Notifications at the point where state changes. |
| Inspector | `src/eepp/ui/tools/uiwidgetinspector.cpp` | The inspector's *Accessibility* tab shows the projected node of the selected widget. |

All backend and source files are private (`src/`), so they can change freely without affecting
the public API.

## Core concepts

### Node references

Every accessible node is an `AccessibilityNodeRef { source, id }`.

- `source == 1` (the manager's widget source) means `id` names a live `UIWidget`. The manager
  keeps `mWidgetIds` / `mWidgets` to map between the two.
- Any other `source` is an `AccessibilitySource` that resolves its own ids. Today the only source
  is the model-view source created for each `UIAbstractTableView` (list, table and tree views).

Identities are **lazy**: a widget gets an id the first time a client reaches it (`getNodeRef()`).
Ids are never reused, so a stale reference held by a client simply stops resolving
(`isValid()` returns false) instead of pointing at a different widget.

### What is exposed

A widget is an accessible element when its resolved role is not `None`
(`UIWidget::isAccessibilityElement()`). Containers with role `None` are transparent: their
element descendants are reported as children of the nearest element ancestor. A widget marked
hidden (`aria-hidden`) hides its whole subtree.

`AccessibilityWidgetResolver` derives semantics from the widget type (`isType()` chains) unless
the application overrode them through the widget's accessibility properties. It is the single
place that knows how a `UISpinBox`, `UIComboBox`, `UITextEdit`, and so on map to roles, states and
actions.

### Model views

List, table and tree views recycle their cell widgets, so those widgets are implementation
detail. The model-view source exposes rows (and cells for tables) as virtual nodes keyed by
`PersistentModelIndex`, so identities follow rows when the model inserts or removes them.

- Child counts and child access are lazy: a 100k row model creates nodes only for the rows a
  client asks for. Enumerating every child is still O(rows), as in other toolkits.
- While a cell is edited, the editor widget is a real widget exposed as the only child of the
  virtual row (lists and trees) or cell (tables) it edits.
- `AccessibilityWidgetResolver::getOwningModelView()` is the one test for "is this widget an
  implementation child of a view". Use it rather than walking ancestors by hand.

### Focus

Focus is projected, not reported raw: the focused node may be an implementation child (a spin
box's text input) or a cell widget. `AccessibilityWidgetResolver::getFocusOwner()` walks from the
event dispatcher's focus node to the element that represents it. Both
`AccessibilityManager::getKeyboardFocusedNode()` and the `Focused` state use it, so they always
agree. Ancestors that merely contain the focus are not `Focused`.

## The two flows

### Queries (client to eepp)

1. The client asks the backend (D-Bus method, `NSAccessibility` attribute or COM call).
2. The backend translates the native object to an `AccessibilityNodeRef` and calls the manager:
   `getNodeInfo()`, `getParent()`, `getChildCount()` / `getChild()`, `getIndexInParent()`,
   `hitTest()`, `getKeyboardFocusedNode()`, `performAction()`.
3. The manager resolves the ref to a widget (resolver) or to a source.

Queries always run on the UI thread (see [Threading](#threading)). Use the cheapest query that
answers the question: `getNodeInfo( ref, false, false )` skips copying the value and computing
text offsets, which matters for large text documents; `getChild()` and `getIndexInParent()` avoid
materializing a whole child list.

### Events (eepp to client)

1. A widget changes and calls `notifyAccessibilityEvent( event )`.
2. If no native client is active, this returns immediately (see [the gate](#1-the-dormant-fast-path)).
3. `AccessibilityWidgetResolver::getEventTarget()` routes the event to its semantic owner (a
   dropdown's state belongs to its combo box; an implementation child's focus belongs to its
   view) or drops it (implementation children, detached widgets).
4. `AccessibilityManager::notify()` invalidates caches, appends the event to the frame's queue and
   hands it to the backend immediately through `AccessibilityBackend::onEvent()`.
5. The backend emits the native notification. AT-SPI emits D-Bus signals right away (caret and
   selection changes are compared once per frame). UIA and macOS queue the event and raise it from
   their next `update()`, on the UI thread and outside any widget or model callback. The manager's
   queue is cleared at the end of every `AccessibilityManager::update()`.

Text edits take an extra step before their `ValueChanged`. `UICodeEditor` and `UITextInput` pass
the document's `DocumentContentChange` (range and inserted text) to
`UIWidget::notifyAccessibilityTextChanged()`. The removed text is not part of the change record,
so consumers that queue records (LSP clients) never copy it: the manager reads it, while the
document is still notifying, from `TextDocument::getNotifiedRemovedText()`. `AccessibilityManager::onTextChanged()` turns the
first four per element and frame into exact `AccessibilityTextChange` records (code-point offset,
removed, inserted). After that, and for resets, loads, reloads and document swaps, it sends one
whole-text change for the rest of the frame, so a replace-all costs one notification.
`AccessibilityBackend::onTextChanged()` receives them; AT-SPI turns exact ones into `text-changed`
signals immediately and a whole-text one into delete-all plus insert-all at its next update. A
backend returns false for a change it ignores, and an ignored change does not count toward the
frame's four. AT-SPI's `SetTextContents` reports its replacement as one minimal diff, so it
suppresses the element's records through `AccessibilityManager::setSuppressedTextChanges()` while
the action runs. Node refs are manager-local, so an editor in another window sharing the document
still reports the change.

Announcements take a parallel path. `UISceneNode::announceForAccessibility()` and live regions (a
`NameChanged` or `ValueChanged` from a widget with an `aria-live` ancestor and no `aria-hidden`
one) append to `AccessibilityManager`'s announcement queue. A live-region entry remembers its
source widget, and a later change from the same widget in the same frame replaces its text instead
of queueing another entry. `AccessibilityManager::update()` hands the queue to
`AccessibilityBackend::announce()` after the backend's own `update()`, so the frame's focus and
state changes reach the client first. Live-region entries are checked again at delivery and
dropped when their source was destroyed, hidden or made `aria-hidden` in the meantime.

Names and descriptions that come from `aria-labelledby` / `aria-describedby` depend on another
widget. When a client reads such a widget's info, the manager remembers the referenced id (ids,
not pointers, so no entry can dangle). A name change of a widget with a remembered id, an id
change, or the arrival, removal or deletion of a subtree containing one, makes the manager scan
the scene for dependents and notify `NameChanged` or `DescriptionChanged` for them. While no
client has read a dependent, the check is one lookup in an empty set. `UIWidget::setId()` asks
`AccessibilityManager::isRelationTarget()` (an allocation-free hash lookup) before renaming, and
keeps a copy of the old id only when it is remembered. Dependents are notified after the id has
changed, for the old id and then the new one (which may complete a relation that pointed at a
missing widget), so a backend that reads the node from inside the notification sees the new
relation.

## Rules the implementation relies on

These are the invariants that past bugs came from. Keep them when changing the code.

### 1. The dormant fast path

Applications without an assistive client must pay close to nothing.

- `UIWidget::notifyAccessibilityEvent()`, `notifyAccessibilityTextChanged()` and
  `notifyAccessibilityWholeTextChanged()` are inline. They test one process-wide counter of root
  scenes with an active client (`UIWidget::isAccessibilityActive()`, a relaxed atomic load) and
  only then call the out-of-line `deliver*()` implementation, which checks the scene's own bit
  (`UISceneNode::hasActiveAccessibilityClients()`, `AccessibilityClientActive`). Callers that
  compute a payload before notifying test `isAccessibilityActive()` first. Pick the event by
  overriding, not by checking the type at the call site: `UITextView::onTextChanged()` reports
  `NameChanged` and `UITextInput` overrides it to report `ValueChanged`. Release code for a dormant
  `UITextView::onTextChanged()` is one load, one test and a jump.
- The bit is set when a backend reports a client (`hasActiveNativeClients()`, refreshed in
  `UISceneNode::update()`) or when a native query arrives (`onNativeClientObserved()`). Every
  change goes through `UISceneNode::setAccessibilityClientActive()`, which keeps the process-wide
  counter in sync and, when the last client goes away, calls
  `AccessibilityManager::onClientsDisconnected()` (see [rule 6](#6-what-survives-a-disconnection)).
- Backends decide what "active" means. AT-SPI tracks client bus names and clears the bit when they
  disconnect. UIA requires window-specific activity. macOS has no disconnect signal, so the bit
  stays set after the first query.
- Without a client, `AccessibilityManager::notify()` queues nothing: only identity invalidation
  (`Destroyed` without a related node) still reaches the backend, which detaches native wrappers
  on it. Removing a widget a disconnected client had seen invalidates its identity without
  resolving or registering its parent to announce the removal. While only the scene root has an
  identity, deleting any other widget returns before walking its ancestors, unless a client is
  active and has read a relation (`aria-labelledby`, `aria-describedby`) whose target may be in
  the deleted subtree.
- Lifecycle bookkeeping never creates a manager: destruction, scene and host changes use
  `UISceneNode::getExistingAccessibilityManager()`, and the const `getAccessibilityManager()` does
  not create one at any nesting level.
- Do not add per-frame or per-layout work that runs before this check. Bounds are queried on
  demand; there is deliberately no per-widget bounds notification.
- `UISceneNode::announceForAccessibility()` returns before doing anything while no client is
  active, but a caller that formats its message first still pays for that formatting.

`benchmark_accessibility_inactive.py` compares the enabled and environment-disabled backends of the
same build: both run the same compiled-in hooks, so it cannot show costs they share.
`benchmark_accessibility_dormant.py` measures those directly and can compare against the same
benchmark built against another tree (`--compare LABEL=PATH`), such as an exported develop
revision. Its allocation counts and bytes cover ordinary C++ `new`/`new[]` on the measuring thread
only: not `malloc`, other threads or background work such as AT-SPI initialization. "No more
allocations than without accessibility" means exactly that measurement. A run passes only when
every sample exits cleanly and reports every expected measurement, guarded scenarios included.

### 2. Teardown happens once, top-down

Widget destruction is where most of the dangerous bugs were.

- `~UIWidget` calls `AccessibilityManager::onWidgetDelete( this )` **before** `onClose()`, then sets
  `NODE_FLAG_DESTROYING`. The manager invalidates the whole subtree at once
  (`removeSubtreeIdentities()`), so descendants' destructors find nothing to do and never touch a
  partially destroyed ancestor.
- Anything that walks ancestors must stop at a destroying node: `getNodeRef()`, `getParent()`,
  `getEventTarget()` and `getFocusOwner()` all do.
- Deleting something no client has seen sends nothing and allocates no identity. Use
  `getWidgetParent()`, which does not register the widget, when you only need the parent of a widget
  that is going away.
- `~UISceneNode` destroys its manager (and therefore the native backend) **before** deleting its
  children, while the root is still alive, and then clears `mRoot`. A closing scene
  (`NODE_FLAG_CLOSE`) returns no manager from either `getAccessibilityManager()` overload.
- Nested scenes (`UISceneNode` inside another scene) share the host's manager. Moving or deleting
  them goes through `onSubtreeRemoved()`.

### 3. Event order is meaningful

`notify()` hands each event to the backend synchronously, so the backend sees exactly the
sequence that happened. Only adjacent query-again hints (`ModelChanged`, `ChildrenChanged`,
`BoundsChanged`) with identical arguments are coalesced. Never coalesce state or structural
events: dropping the second half of add/remove/add or on/off/on leaves clients with stale state.
Backends that need batching do it themselves, per frame:

- AT-SPI compares each text's caret and selection once, and reports a whole-text replacement
  once.
- UIA raises one focus change, for the final keyboard focus.
- macOS keeps only the last focus change, and posts every other non-structural notification at
  most once per (event, element, related element). AppKit notifications carry no state, so a
  repeat adds nothing. It uses a hash set beside the ordered queue, so a burst stays linear.
  `Created` and `Destroyed` are never de-duplicated.

### 4. Threading

The manager, the resolver and every widget are UI-thread only.

- **AT-SPI**: an I/O thread only `poll()`s the D-Bus socket and wakes the event loop. Messages are
  read and dispatched on the UI thread in `AtSpiApplication::update()`, at most 256 per frame.
- **UIA**: COM calls arrive on arbitrary threads. Every provider method goes through
  `UIAutomationProviderContext::invoke()`, which runs the callback directly on the UI thread or
  posts a window message and waits up to `ProviderCallTimeoutMilliseconds`. The pending request
  holds a COM reference on the provider so a timed-out call cannot free it mid-callback.
  `UiaDisconnectProvider` is deferred to `update()` because outgoing COM calls can re-enter the
  STA during widget destruction.
- **macOS**: AppKit is main-thread only; events are queued and delivered on the next `update()`.

### 5. Cost is proportional to what the client asks for

- Never walk the whole widget tree or the whole model per event or per query. Prefer the event
  dispatcher's focus node, `getIndexInParent()` and source lookups over scanning children.
- Building `AccessibilityNodeInfo` with the value copies the text of a text widget; request it only
  where the value is needed. Text interfaces use `getTextLength()`, `getTextRange()` and
  `getTextLineBounds()`, which read only what they return. Offsets convert through a line index of
  the document (`lineStarts()` in the resolver), keyed by `getTextRevision()`: the document's UUID
  and modification id, which every content change bumps. Backends that need a converted copy
  (UTF-16 for UIA and AppKit) cache it per revision.
- Leaf controls (`AccessibilityWidgetResolver::isLeafRole()`) have no exposed children. Events and
  focus from inside one are reported on the control (`getLeafOwner()`), so the inner widgets never
  get identities.

### 6. What survives a disconnection

When a scene's last client disconnects, `onClientsDisconnected()` resets every model-view source:
queried rows are dropped together with their persistent model registrations, and backends detach
their native wrappers (`onSourceInvalidated()`). Without that, the model would keep updating one
persistent handle per queried row on every insert, delete and move for as long as it lives.
Identities are never reused, so a reconnecting client starts from fresh row identities and a stale
one stays invalid. Widget identities and metadata (labels, relations, live regions) are kept.

The model only drops a persistent handle when all its registrations are released
(`PersistentModelIndex::release()`); handles other consumers registered are unaffected. A queried
row's registration is released when its node is dropped, so pruned, reset and destroyed sources do
not leave handles behind either.

## Backends

### AT-SPI (Linux and FreeBSD)

One `AtSpi::AtSpiApplication` is shared by all windows of the process and owns a private
connection to the accessibility bus. libdbus is loaded at runtime (`DBusLibrary`), so there is no
build-time D-Bus dependency. Object paths encode `/org/eepp/a11y/<manager>/<source>/<id>`.

A peer other than the registry that calls the application becomes a client. Its disconnection is
watched with a `NameOwnerChanged` match on its own unique name, not a bus-wide match that would
wake the application for every program joining or leaving the bus. A non-blocking `NameHasOwner`
check sent after the match covers a client that left before the match was installed: the bus
processes both requests in order.

| File | Contents |
| --- | --- |
| `accessibilitybackendatspi.hpp` | `DBusLibrary` and the `AtSpiApplication` declaration, with members grouped by implementing file. |
| `accessibilitybackendatspi.cpp` | Connection setup and registry embedding, I/O thread, per-frame dispatch, client tracking, `AtSpiAccessibilityBackend`. |
| `accessibilitybackendatspievents.cpp` | Manager events to `org.a11y.atspi.Event.*` signals; text change records to `text-changed`, and once-per-frame caret and selection comparison (`text-caret-moved`, `text-selection-changed`). Keeps no text contents. |
| `accessibilitybackendatspitree.cpp` | Path/ref conversion, navigation, role and state mapping, action names, D-Bus marshalling helpers. |
| `accessibilitybackendatspimethods.cpp` | Incoming calls: `handleMessage()` routes to one handler per interface (`Accessible`, `Component`, `Action`, `Text`, `EditableText`, `Application`, `Properties`, `Cache`). |

### UI Automation (Windows)

Each top-level window has a `Uia::UIAutomationProviderContext`, created with the backend, which
subclasses the window to answer `WM_GETOBJECT` and caches one `UIAutomationProvider` per node.

| File | Contents |
| --- | --- |
| `accessibilitybackenduia.hpp` | The context (including `invoke()`), shared structs, helper declarations, provider and text range declarations. |
| `accessibilitybackenduia.cpp` | Provider cache and invalidation, window subclass, manager events to UIA events, `UIAutomationAccessibilityBackend`. |
| `accessibilitybackenduiaprovider.cpp` | The element provider: fragment navigation, properties and every control pattern; role and `VARIANT` helpers. |
| `accessibilitybackenduiatext.cpp` | `ITextRangeProvider`, UTF-16/code point conversion and text unit navigation. |

### NSAccessibility (macOS)

`accessibilitybackendmacos.mm` holds `MacAccessibilityState` (one per window: element cache and
manager access), `EEPPMacAccessibilityElement` (an `NSAccessibilityElement` subclass per node),
`MacAccessibilityRegistry` (process-wide window list and event delivery) and the backend class.
It is still a single file because it can only be built and tested on a macOS host.

## Common tasks

**Expose a new standard widget.** Add its role, name, state, actions and, if applicable, value,
range or text to `accessibilitywidgetresolver.cpp`. Call `notifyAccessibilityEvent()` from the
widget where its semantic state changes (for example `StateChanged` when it is checked). If it
contains implementation children, make sure they resolve to the widget (role `None`, hidden, or
routed in `getEventTarget()`), and add a unit test in `accessibility_tests.cpp`.

**Add a role.** Add it to `AccessibilityRole` (`accessibility.hpp`), map it in the resolver, then in
every backend: `role()` and `roleName()` in `accessibilitybackendatspitree.cpp`, `controlType()` in
`accessibilitybackenduiaprovider.cpp`, and `nativeRole()` / `nativeSubrole()` in
`accessibilitybackendmacos.mm`.

**Add an action or state.** Add the enum value, produce it in the resolver, then map it per backend:
AT-SPI `appendStates()` / `actionName()`, UIA patterns in the provider's `QueryInterface()` and
`GetPatternProvider()`, macOS action names and attributes.

**Announce something.** Call `UISceneNode::announceForAccessibility()`, or mark the widget that
changes with `aria-live`. A backend implements `AccessibilityBackend::announce()`; AT-SPI emits
`Announcement` on the root object, UIA calls `UiaRaiseNotificationEvent` (looked up at runtime),
macOS posts `NSAccessibilityAnnouncementRequestedNotification` on the window.

**Answer a new AT-SPI method.** Add it to the matching `handle<Interface>()` in
`accessibilitybackendatspimethods.cpp`. Return `1` for "not handled" so the dispatcher replies with
`UnknownMethod`.

**Add a virtual node type.** Implement `AccessibilitySource` and create it from
`AccessibilityManager::sourceFor()`. Only implement the optional hooks (`getIndexInParent()`,
`getSelectedChildren()`, `getEmbeddedWidgetParent()`) when the default fallback would be too slow
or wrong.

## Testing and debugging

| Layer | How it is tested | Where it runs |
| --- | --- | --- |
| Core (manager, resolver, model-view source, lifecycle) | `src/tests/unit_tests/accessibility_tests.cpp`, using the null backend and `onNativeClientObserved()` to open the notification gate. | Every platform's unit test job. |
| macOS backend semantics | `src/tests/unit_tests/accessibility_macos_tests.mm` | macOS unit tests. |
| AT-SPI end to end | `projects/scripts/test_atspi.py` (default, `--multi-window`, `--close-primary`) and `benchmark_atspi.py` | Linux CI, inside `dbus-run-session` and Xvfb. |
| Inactive overhead | `projects/scripts/benchmark_accessibility_inactive.py` | Linux CI. |
| Dormant allocations and costs | `src/benchmarks/accessibility_dormant_benchmark.cpp` (`eepp-benchmarks --filter='AccessibilityDormant.*'`), summarized by `projects/scripts/benchmark_accessibility_dormant.py [--compare LABEL=PATH]` | Locally, Release builds. Allocation counts are deterministic and asserted where they guard a fix; times are indicative. |
| UIA end to end | `src/tests/windows_accessibility/main.cpp` (`eepp-windows-accessibility-tests`) | Windows CI. Under Wine it builds and finds the window, but Wine's UIA client lacks several APIs, so most checks report `E_NOTIMPL`. |
| NSAccessibility end to end | `projects/scripts/test_macos_accessibility.swift`; `benchmark_macos_accessibility.swift` locally | macOS CI. |

Useful tools:

- the runtime inspector's *Accessibility* tab shows exactly what a widget projects, and its *Audit*
  tab (`AccessibilityWidgetResolver::audit()`) lists unnamed focusable controls and broken
  `aria-labelledby` / `aria-describedby` ids;
- `EEPP_DISABLE_ACCESSIBILITY=1` runs with the null backend, the baseline for performance and
  behavior comparisons;
- Accerciser (Linux), Accessibility Inspector (macOS) and Inspect / Accessibility Insights
  (Windows) show the native tree;
- to run the AT-SPI scripts without touching your desktop session, wrap them in
  `dbus-run-session` as CI does;
- `EEPP_ACCESSIBILITY_TRACE=1` prints client activity transitions, and AT-SPI client connections
  and disconnections, to stderr. It shows what activates an application that has no screen reader
  running.

## Known limitations

- Enumerating all children of a model view is O(rows).
- macOS cannot detect that a client went away, so its active bit stays set after the first query.
  Any process that queries one of the application's accessibility elements (not only screen
  readers) activates it for the rest of its life.
- Windows refreshes `UiaClientsAreListening()` on every scene update; a cheaper cadence needs
  native validation that listener changes are still observed promptly.
- Linux connects to the accessibility bus at startup, whether or not a client will ever attach,
  so applications are discoverable by tools started later and by clients that never set the
  `org.a11y.Status` flags. Gating the connection on those flags (as Qt does when no X11 root
  window atom publishes the bus) would trade that discovery for startup work; it is a policy
  choice, not a transparent optimization. Initialization runs off the UI thread: on the scene's
  thread pool, or on a detached thread when the scene has none. Either way the task shares
  ownership of the AT-SPI application, so tearing the backend down never waits for pending D-Bus
  calls or their timeouts; whichever owner releases it last destroys it.
- The line index is rebuilt in full after each edit (about 3 ms for a 100,000-line document),
  paid only while a client is querying that document.
- HTML content (`UIRichText`, headings, links, lists) is not exposed as semantic elements.
- Only one cell editor (the view's active editor) is exposed inside a model view.
