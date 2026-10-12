# UI Accessibility Architecture

This article is for people working on eepp's accessibility implementation. For the application
facing API (roles, labels, policies and testing an application) read
[UI accessibility](ui_accessibility.md) first.

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
hidden (`aria-hidden`) hides its whole subtree. Native events and exact text-edit records from
that subtree are silent; hiding or exposing it refreshes its exposed parent's children. Hidden
labels can still update visible controls that reference them. Virtual model rows inherit their
view's enabled and visibility states, and reject actions while the view is disabled.

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
text offsets, which matters for large text documents. Ordinary `getChildCount()` does not materialize
identities; model-source `getChild()` and `getIndexInParent()` avoid enumerating all rows.

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
document is still notifying, from `TextDocument::getNotifiedRemovedText()`. The manager first maintains
an existing optional text index, independently of event support, suppression and the frame budget.
Then it checks `AccessibilityBackend::supportsTextChanges()`: only AT-SPI opts in. UIA and macOS use their queued
`ValueChanged` notifications, so their edits build no exact-edit records, copy no edit text and
calculate no document offsets here. `AccessibilityManager::onTextChanged()` turns the
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

The `unused_text_edit_records` allocation guard tests the UIA/macOS-style backend with an active
client: building unused records must allocate zero bytes. This is a record-path measurement, not
a claim that the document edit or its ordinary widget events allocate nothing.

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
keeps a copy of the old id only when it is remembered. Since `OnIdChange` callbacks can destroy the
manager or reparent the widget, it reacquires the current manager after the callback; dependents
left in a surviving old scene are also notified. Dependents are notified after the id has
changed, for the old id and then the new one (which may complete a relation that pointed at a
missing widget), so a backend that reads the node from inside the notification sees the new
relation.

## Rules the implementation relies on

These are the invariants that past bugs came from. Keep them when changing the code.

Native calls and backend updates use an allocation-free `AccessibilityManager::ScopedDispatch`.
If an action, model callback or notification requests a different accessibility policy, the scene
defers that change until the beginning of its next non-reentrant update. The last request wins;
a later explicit request outside dispatch supersedes it. The first deferred request wakes the
event loop, including when no widget needs repainting. The current manager and backend remain
alive until the native call has replied. Reentrant scene updates do not recursively dispatch
the same backend. Changing policy outside accessibility dispatch remains immediate.

Hierarchy changes cross from Scene to UI through a private, single-slot `SceneNode` bridge.
Only the internal `Accessibility` participant registers it, and `Node` dispatches it through
friendship. Registration, dispatch and storage are not an application extension API. The bridge
keeps Scene independent of UI headers and costs one relaxed load when no client is active.

Hit testing descends exposed containers such as groups and windows; only leaf controls and
model views stop traversal into implementation children. When a transparent layout or plain-node
subtree is moved, the manager invalidates both semantic parents and reports its exposed descendants,
preserving their identities within one scene. Their indices are found with one parent scan.
A widget appended right after the previously added one takes the next index without a scan,
so populating a container with a client attached stays linear. Property changes (name, value,
state and similar) of a widget without an identity are not reported: no client was handed it,
and registering it there would make its later move out of the scene root announce a removal.
Sibling reorders (`toFront()`, `toBack()` and `toPosition()`) report removal at the old
semantic index and arrival after relinking at the new index. They preserve element identities,
including exposed descendants of transparent layouts, without duplicating ordinary reparenting.
Indexed insertion into a different parent links directly at the requested position before the
scene/parent-change hooks run; it never announces a transient append. Moving a child to its
current position (or appending a child already last) emits no structural events.
The final insertion position is resolved after source-removal callbacks, which may themselves
reorder the destination; no destination sibling pointer is retained across those callbacks.

Role/state-only queries skip names, values, text offsets and relation tracking. Model sources
answer these queries without fetching model display data. Native AT-SPI role queries, UIA
identity/role/state properties, and macOS role and focused/selected/expanded/enabled accessors
use these scalar paths without recording relation targets; full metadata reads still record them.
Changing a widget between transparent and exposed roles refreshes its semantic parent's children.
Column visibility and main-column changes invalidate virtual view children, even though events
from implementation header/cell widgets are suppressed. Tree sources remember expansion state
only for queried rows and report changed rows on structural invalidation, including keyboard,
accessibility-action and bulk expansion. AT-SPI forwards these as `state-changed:expanded`, UIA
as expand/collapse property changes, and macOS as row-expanded/row-collapsed notifications.
No rows are materialized solely to announce expansion.

AT-SPI selection snapshots are initialized by the first container state/selection query or
selected-row state query, then
maintained by selection events and cleared on model/tree invalidation (including structural
add/remove and child-only selection events).
Repeated `IsChildSelected` and selected-row `GetState` calls reuse
the snapshot; they do not re-enumerate or sort the selection. An unqueried selection container
emits only `selection-changed`, without materializing selected rows or allocating a snapshot.
Custom widgets must emit `SelectionChanged` on the container or child whenever their
`Selected` state changes: queries reuse the snapshot rather than polling for silent changes.
Reading an unselected row's state does not initialize its container's snapshot. If the
selection later moves to that row before a selection query, clients receive the container hint
but no per-row selected-state events, and must refresh cached row states on that hint.

### 1. The dormant fast path

Applications without an assistive client must pay close to nothing.

- `UIWidget::notifyAccessibilityEvent()`, `notifyAccessibilityTextChanged()` and
  `notifyAccessibilityWholeTextChanged()` are inline. They test one process-wide counter of root
  scenes with an active client (`Accessibility::isActive()`, a relaxed atomic load) and
  only then call the out-of-line `deliver*()` implementation, which checks the scene's own bit
  (`UISceneNode::hasActiveAccessibilityClients()`, `AccessibilityClientActive`). Callers that
  compute a payload before notifying test `Accessibility::isActive()` first. Pick the event by
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
  not create one at any nesting level. Widget destruction checks retained state inline before
  querying shutdown state or calling the invalidation code. Empty and root-only managers skip that
  work; identities retained by explicit Inspector queries still need invalidation, even without
  native clients. Descendants of an already-destroying widget skip the manager lookup because
  the ancestor invalidated their identities before running its close callbacks.
  Core hierarchy changes use an optional, allocation-free `SceneNode` observer: accessibility
  installs its bridge when the first scene becomes active and removes it when the last goes idle.
  A null observer costs one relaxed load and no call. `Node` has no UI or accessibility dependency;
  the bridge resolves UI scenes only while at least one native client is active.
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
  read and dispatched on the UI thread in `AtSpiApplication::update()`, at most 256 per dispatch,
  stopping after a callback when 2 ms has elapsed. Remaining messages preserve the pending flag
  and wake the idle event loop. At least one callback runs; a single synchronous query may exceed
  the budget. This is not a frame-time guarantee: each live scene may service the shared connection,
  and text notifications, outgoing writes and window announcements occur outside the message budget.
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
  `getTextLineBounds()`, which read only what they return. Offsets convert through the manager's
  optional `AccessibilityTextIndex`, keyed by document UUID and revision. Backends that need a converted copy
  (UTF-16 for UIA and AppKit) cache it per revision.
- Leaf controls (`AccessibilityWidgetResolver::isLeafRole()`) have no exposed children. Events and
  focus from inside one are reported on the control (`getLeafOwner()`), so the inner widgets never
  get identities.
- Ordinary widget child counts traverse semantic children without assigning identities or building
  a child vector. A matching materialized children cache supplies its size directly. Model sources
  retain their specialized count paths; a cold ordinary count is still O(semantic descendants).

### Optional text indexing

The first query requiring offsets/length, or an exact native edit requiring an offset, creates an
index. Merely attaching a client or sending metadata-only queries does not. Multiple editors of the
same document in a manager share it; different windows have independent stores. No fields, cache,
observer or new bookkeeping are added to `TextDocument` or to each widget.

Each stored line length includes its newline; the final synthetic newline is excluded from exposed
text length. Lengths and sums use 64-bit integers; public offsets saturate to `Int32`. Blocks contain
256 lines and a compact Fenwick prefix-sum tree of block totals, so conversions examine at most one
block plus O(log(block count)) totals. A simple scan of block totals was measured and rejected because
it made warm queries near EOF scale with document size.
On the measured 64-bit build the index is 104 bytes, its store is 24 bytes, and each manager gains
one optional pointer (8 bytes). Active metadata requires 8 bytes per line plus 8 bytes per block,
subject to vector capacity; it is released with the last binding or the client disconnect.

An ordinary edit reads one affected line length and updates block totals without allocation. A
structural edit splices flat line metadata and rebuilds block totals, reading only affected document
lines: metadata shifting is O(lines), not a fresh document scan. Initial creation and lazy recovery
are O(lines), using `TextDocument::getLineLengths()` to fill the existing length buffer under one
scoped read lock. Both the line container and shared line contents are locked once, with no
per-line mutex calls or text copies. The modification id is returned inside that scope; no
document state or dormant-edit work is added. Ordinary incremental edits still read one line.
This is not an atomic revision/content transaction for concurrent ordinary edits: their revision
increments precede acquisition of the mutation locks. Ordinary editing is UI-thread-owned; caches
continue to invalidate after asynchronous loads rather than treating the returned revision as
proof that an independently running edit has completed.
Large insertions
(over 64k code points), whole-text resets, missed/nested revisions and ambiguous edit shapes invalidate
lazily. Revision increments precede mutation, so duplicate/same-revision callbacks validate affected
post-edit lengths instead of assuming that the revision means the edit has finished.
When callbacks forward nested edits out of order or skip an indexed revision, every editor sharing
that document uses the existing whole-text event fallback for the remainder of the frame. Rebuilding
lengths from a re-entrant query cannot make those event deltas trustworthy; the manager clears this
ambiguity only at the frame boundary. Ordinary contiguous edits still use exact records normally.

Existing indexes are maintained before backend edit-support checks, suppressed changes and exact-edit
limits. Widget deletion/identity removal, document switches and the last client disconnect release
bindings. The store retains UUIDs and widget identities, not document pointers or owning references;
widget pointers are never dereferenced there. Worker loads recover through the main-thread deferred
whole-text notification and revision validation, not worker-side cache mutation.

Explicit offset queries without native clients still recover lazily on revision changes; they do
not enable native notifications or bypass the dormant widget gates. No current tooling consumer needs
continuous offset-index maintenance without a native client, so no additional observer or usage flag is added.

The Inspector's Accessibility table requests metadata without full values or text offsets. Its editor
Value preview reads only a bounded prefix under the document read lock, without constructing a text
index; that table's column sizing never measures the full document. Non-text refreshes reuse the
preview only while the document UUID/revision match and no load is in progress. Text and document lifecycle
events invalidate it; incomplete loading snapshots are not cached. Text inputs are rechecked on
every event rather than reusing cached plaintext. `UITextInput::setMode()` publishes a value event
even without native clients, so switching to password mode clears the preview immediately; its
gated native state/value notifications also invalidate the client's protection metadata.

Worker-originated Inspector refreshes use widget-owned main-thread actions holding a weak model
reference and verifying the selected widget. Model strings are borrowed until invalidation rather
than copied for every cell read. These are explicit tooling queries, not native-client activity;
native backends still expose full values. Full accessible-value extraction in the Inspector is
reserved for the user's **Copy full value** action, which skips incomplete/loading documents.

### 6. What survives a disconnection

When a scene's last client disconnects, `onClientsDisconnected()` releases text indexes and resets every model-view source:
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
| `accessibilitybackendatspimethods.cpp` | Incoming calls: `handleMessage()` routes to one handler per interface (`Accessible`, `Component`, `Action`, `Selection`, `Text`, `EditableText`, `Application`, `Properties`, `Cache`). |

List, table, tree and tab-list selection changes emit `object:selection-changed` on the
container, not `state-changed:selected` on the container. Queried selections retain only sorted
selected refs, so selection changes can clear the previous items' selected state and set the
new items' state without materializing the whole model. That state is dropped on model changes,
window removal and client disconnection. The AT-SPI Selection interface exposes selected-child
queries and single-child selection; clear/select-all/deselect operations currently return false,
because those operations are not part of the engine's accessibility action API.

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

**Expose a custom widget.** Selectively override `UIWidget`'s virtual role, name, description,
value, range, state, actions or action-execution methods. Their defaults still use the central
resolver. Effective semantic consumers (including leaf ownership, focus projection and auditing)
use the virtual methods; default resolver implementations must not call their own corresponding
virtual entry point, which would recurse. The default action handler can query virtual actions
because it is a different extension point. Model-backed nodes still use `AccessibilitySource`.
Relation targets also use the effective virtual name. A stack-restored, thread-local query context
keeps the existing one-hop own-name rule when a target's override calls the base implementation,
so default relation chains and cycles do not recurse. It retains no widget state between queries
and allocates nothing.

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
| UIA provider regressions | `src/tests/windows_accessibility/provider_tests.cpp` (`eepp-windows-accessibility-tests --providers-only`) | Windows and Wine, using hidden windows; teardown and cross-window range identity are tested without system UIA client APIs. |
| UIA end to end | `src/tests/windows_accessibility/main.cpp` (`eepp-windows-accessibility-tests`) | Windows CI, including the provider regressions. Under Wine it builds and finds the window, but Wine's UIA client lacks several APIs, so most end-to-end checks report `E_NOTIMPL`. |
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
- `EEPP_ACCESSIBILITY_PROFILE=1` enables optional UI-thread dispatch measurements for AT-SPI and
  queued UIA requests. `EEPP_A11Y_PROFILE` reports cumulative work, message/batch maxima, counts and
  AT-SPI backlog batches, logarithmically and on new maxima. Durations are UI-thread wall time (including
  scheduling/preemption), not CPU counters. It does not measure application frames,
  AppKit callbacks, direct UI-thread UIA calls or category-level costs. Console output and timing have
  overhead, so compare instrumented runs against instrumented runs. Without the flag there is no
  profiling allocation or clock read; the AT-SPI message budget reads the clock only during dispatch.

### Reproducing the active performance checks

Use `.ecode/project_build.json`'s Linux Release Ninja configuration, no ASAN, and the host display.
Wait for competing builds or CPU work to finish; discard runs that overlap it. Never use local Xvfb
for performance comparisons. All windows below are hidden by their benchmark modes.

```sh
premake5 --disable-static-build --with-debug-symbols --with-backend=SDL3 ninja
ninja -C make/linux release
python3 projects/scripts/benchmark_accessibility_dormant.py --active-text --samples 5 --json
python3 projects/scripts/benchmark_accessibility_dormant.py --cold-text --samples 5 --json
a11y_runtime_dir=$(mktemp -d /tmp/eepp-a11y-runtime.XXXXXX)
env -u AT_SPI_BUS_ADDRESS XDG_RUNTIME_DIR="$a11y_runtime_dir" \
  dbus-run-session -- env EEPP_ACCESSIBILITY_PROFILE=1 \
  /usr/bin/python3 projects/scripts/benchmark_atspi.py --item-counts 1000 \
  --iterations 1 --burst-requests 200 --json
```

The private session and runtime directory prevent the native probe from reusing the desktop's
accessibility-bus socket or confusing an already running copy with the benchmark process.

The active text cases use a mock active backend to isolate semantic indexing, not a screen reader.
They separate edit work, post-edit queries, warm queries, direct index updates and cold child counts.
They also materialize 100, 10k and 100k model rows, preserve a retained row's identity through an
insert/delete pair, and require zero allocation for the same mutations after disconnect. These
include model-core persistent-index maintenance, not only the accessibility source's own work.
Guards check allocations and lines read, not host-specific microsecond thresholds. Allocations cover
ordinary C++ `new/new[]` on the measuring thread only, not `malloc`, native libraries or other threads.
The optional native burst interleaves full model enumerations and secondary-window queries, requires
every complete reply and reports client round-trip latency separately from native UI-thread profiling.
Its timing is informational, not a new CI threshold.

The separate cold-text mode prepares 10k/100k/1M-line editors without clients, text queries or focus,
then attaches a mock backend implementing AT-SPI's exact-edit preparation and focus-time semantic
queries. It measures the first edit at the beginning and near EOF, and first focus, against the next
warm operation. These timings include ordinary editor/focus work but exclude D-Bus/native transport.
Exact edit records and offsets must still be correct; the mode does not suppress unqueried text events.
Its Inspector regression selects and edits an editor through the real Inspector table, then verifies
that the first explicit offset query is still cold. That guard fails if Inspector requests unused
offsets, while the following warm query must allocate nothing. No timing threshold is enforced.

On the same Release host, five cold-text samples per implementation accepted without competing CPU
work measured the following before and after replacing per-line locks with a bulk read-lock scope:

| Lines | First edit at start (before → after) | First edit near EOF (before → after) | First focus (before → after) | Allocated bytes on first use (unchanged) |
| --- | --- | --- | --- | --- |
| 10k | 0.294 → 0.050 ms | 0.281 → 0.038 ms | 0.437 → 0.203 ms | about 80.5 kB |
| 100k | 2.850 → 0.381 ms | 2.850 → 0.391 ms | 2.847 → 0.396 ms | about 803 kB |
| 1M | 29.228 → 5.707 ms | 29.210 → 5.715 ms | 29.264 → 5.800 ms | about 8.03 MB |

Warm edits took 0.002–0.014 ms and warm focus took 0.026–0.036 ms, including ordinary editor/focus
work. These are mock-backend semantic-path medians, not real screen-reader/frame-time measurements.
The single lock scope removes the dominant synchronization cost, but initial index creation remains
O(lines) on the UI thread, even when the first edit is at the start. The AT-SPI message budget does
not bound ordinary document-notification work.
Exact events for unqueried text remain enabled: dropping them would change the contract for clients
that subscribe without fetching text first. A strictly bounded cold-start latency would need a
separate follow-up; the remaining scan is not hidden by the warmed-query benchmark.

On a Ryzen 9 3900X (24 logical CPUs, 62 GiB RAM, GCC 16.2.1), Release SDL3 without ASAN,
the five-sample semantic-text median was 0.10–0.13 µs per post-edit query at 10k/100k/1M lines,
and about 0.06–0.08 µs per warm query. A 100k-line index read exactly one line per ordinary edit;
100 direct updates and all measured queries allocated zero bytes. These are mock-backend indexing
measurements, not end-to-end screen-reader timings. A 4k-child cold count eliminated the temporary
442,368 bytes allocated by the old count-via-children implementation.
The dormant allocation measurements match the pre-change branch (`1e0ed08d2`) in every reported
scenario except first initialization: the manager/index-store and native profiler pointers add
16 bytes there, with no additional allocation. Widgets and documents do not grow. This is not a
fresh whole-branch `develop` comparison or proof of zero CPU overhead; it isolates this patch's
allocation changes. Native discovery/background allocations are outside the UI-thread counter.

A separately warmed 200-request/1k-row AT-SPI probe, with competing CPU activity checked and rejected,
measured maximum UI-thread dispatch batches of 13.39 ms with the old count-only scheduling policy
and 2.40 ms with the 2 ms budget. Both returned every reply; client wall times were about 202/197 ms.
That is one instrumented sample per policy, not a frame-time percentile or a throughput guarantee.
The finishing callback may overrun the budget, as intended. The count-only comparison used the same
optimized code with a nonbinding time budget, isolating scheduling from the other improvements.
Multi-window flood correctness is separately exercised by the benchmark's optional burst mode.

A final three-sample quiet-host run also measured the long-lived model cases. Materializing 100k
rows took about 14.5 ms and allocated 46.4 MB cumulatively; an insert/delete pair with those rows
retained took about 15.2 ms and allocated 53.1 MB cumulatively, including model-core handle
relocation and source re-keying. These byte counts are allocation volume, not retained memory/RSS.
After disconnect the same pair allocated zero bytes and took about 0.003 ms. This is a confirmed
active-path scalability limit, not fixed by dispatch scheduling or by the text-index patch.

### Native-host follow-up measurements

Do not infer NVDA/VoiceOver performance from Linux or Wine. On Windows, record 10k/100k/1M-line
typing, caret movement near EOF, selections and retained text ranges with NVDA and UIA automation.
On macOS, repeat with VoiceOver and AXUIElement clients. Measure per-method UI-thread tails, cache
misses, allocations/RSS and application frame times, distinguishing offset indexing from UTF-16 work.
Windows `textSnapshot()` still obtains full revision-cached source and CRLF-normalized buffers through
`textContents()` and scans code points to translate selection offsets. AppKit text/range accessors
still use the full revision-cached UTF-16 `textFor()` result. A caret-only query can therefore need a
full snapshot after an edit; a warm snapshot can still involve linear UTF-16 offset translation.
These paths remain deliberately unchanged pending real native-client measurements and Unicode/CRLF
conformance checks. UIA queue budgeting, listener-poll cadence and macOS's sticky active state likewise
need native-host evidence; queued UIA dispatch profiling is available for that investigation.

## Known limitations

- Enumerating all children of a model view is O(rows).
- Retaining many queried model rows makes insert/delete maintenance O(queried rows), including
  model-core persistent-handle relocation and source re-keying. The AT-SPI request budget does not
  bound those ordinary widget/model callbacks. Pruning retained row identities blindly would break
  clients; measure native-client retention before changing that identity contract.
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
- Text-index structural shifts and lazy rebuilds remain O(lines). Native UTF-16 snapshot cache
  misses remain O(document size), and the AT-SPI time budget cannot interrupt one expensive query.
- HTML content (`UIRichText`, headings, links, lists) is not exposed as semantic elements.
- Only one cell editor (the view's active editor) is exposed inside a model view.
