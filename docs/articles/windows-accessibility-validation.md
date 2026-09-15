# Windows UI Automation backend validation

Validation date: 2026-09-15

This document records the native Windows validation performed for the eepp UI Automation (UIA)
backend. The automated evidence is produced by `eepp-windows-accessibility-tests.exe` and written
to `bin/unit_tests/output/windows_accessibility_results.json`.

## Test host

- Windows 10 Pro 22H2, build 19045.6466, AMD64, 24 logical processors
- Visual Studio Build Tools 18, MSVC 19.50.35727, toolset 14.50.35717
- Windows SDK 10.0.26100.0
- MinGW-w64 GCC 15.2.0
- Premake 5.0.0-beta7
- SDL 2.32.10 backend
- Windows SDK Inspect 7.2.0.0 and AccEvent 7.2.0.0 are installed
- Narrator 10.0.19041.1 is installed
- Accessibility Insights was not installed on this host

## Original implementation audit

The table describes the implementation at the start of this work, the native evidence that found
the problem, and the resulting correction.

| Area | Original implementation | Native result | Correction |
|---|---|---|---|
| Root exposure | Replaced the window procedure and compared a truncated object ID | Fragile subclass ownership and incorrect 64-bit `WM_GETOBJECT` handling | `SetWindowSubclass`, exact `LONG` object-ID comparison, lazy `UiaReturnRawElementProvider` |
| Fragment navigation | Queried the live manager directly and had incomplete fragment boundaries | Foreign-thread access and inconsistent root/host behavior | UI-thread dispatch, root-scoped navigation, root-only host provider |
| Provider identity | Provider wrappers and cache did not have complete detach semantics | Retained clients could outlive a node/window | Per-HWND scoped cache, stable runtime scope, explicit provider detach and `UIA_E_ELEMENTNOTAVAILABLE` |
| Invoke | Present | Action could wait for a later scene update and hit a two-second timeout | Private HWND dispatch message executes on the owning UI thread |
| Toggle | Present | Pattern/property/event semantics were incomplete | Role/action gated pattern and native toggle property event |
| Selection | Item side only | Clients could not query selection from a logical container | Added `ISelectionProvider` and correct selection container/sender |
| Value | Present | Threading, invalid writes, and text behavior were incomplete | Marshalled access, read-only/error checks, UTF-16 text integration |
| Range value | Present | Validation and role behavior were incomplete | Current min/max/step values and safe malformed/out-of-range rejection |
| Expand/collapse | Present | State event could be raised on an internal popup child | Current semantic state and combo/tree parent event routing |
| Scroll item | Present | Needed the same UI-thread guarantee as other actions | Action-gated marshalled `ScrollIntoView` |
| Text | Absent | `IValueProvider` could not represent document/selection ranges | Added `ITextProvider` and lifetime-safe `ITextRangeProvider` |
| Events | Partial mapping | Events were dropped before native client observation and could expose half-mutated state | Sticky native-client observation, next-update queue, property/text/selection/structure/focus mapping |

The old backend also risked a recursive `CallWindowProcW` chain, stored insufficient ownership state
for teardown, used non-atomic COM reference counts, and allowed live UI-tree access from UIA's MTA
client thread. The replacement keeps node references rather than raw widgets and does not introduce
a second accessibility tree.

## Implemented contract

Each live top-level eepp HWND owns a fragment-root context and provider cache. `WM_GETOBJECT` creates
the root lazily, preserving late discovery whether the client or application starts first. Window
destruction detaches externally retained providers, disconnects UIA provider state, cancels pending
dispatches, removes the subclass, and leaves other top-level window contexts intact.

Providers expose Invoke, Toggle, Selection/SelectionItem, Value, RangeValue, ExpandCollapse,
ScrollItem, and Text only when the shared accessibility node advertises the corresponding semantic
capability. Text offsets are explicitly represented as UTF-16 code units. The tests include CRLF,
a non-BMP emoji, and a combining character, and verify that protected fields expose no text pattern.

### Event mapping

| Shared event | UIA event/property |
|---|---|
| Name changed | `UIA_NamePropertyId` |
| Description changed | `UIA_HelpTextPropertyId` |
| Value changed | `UIA_ValueValuePropertyId` or `UIA_RangeValueValuePropertyId`; text changed where applicable |
| Enabled changed | `UIA_IsEnabledPropertyId` |
| Visibility changed | `UIA_IsOffscreenPropertyId` |
| Checkbox state | `UIA_ToggleToggleStatePropertyId` |
| Radio/selection state | `UIA_SelectionItemIsSelectedPropertyId` and element-selected event |
| Expanded state | `UIA_ExpandCollapseExpandCollapseStatePropertyId` |
| Created/destroyed | Child-added/child-removed structure event with runtime ID |
| Children/model changed | Children-invalidated structure event |
| Focus changed | Has-keyboard-focus property and focus automation event |
| Text selection changed | Text-selection-changed event |
| Other layout change | Layout-invalidated event |

The fragment root implements `IRawElementProviderAdviseEvents`. Subscription counts are tracked,
while successful native discovery is sticky for the context so tree availability never depends on
event subscription state. Property events intentionally use an empty old-value variant when the old
snapshot is unavailable.

## Threading and teardown paths

All manager/tree reads and actions happen on the owning UI thread. Providers invoked on that thread
execute directly. A foreign provider call creates a bounded dispatch record, posts the registered
`eepp.UIAutomation.Dispatch` HWND message, and waits for at most two seconds. The UI thread drains
the request directly from the window procedure. Window shutdown marks the context unavailable and
completes pending records with `UIA_E_ELEMENTNOTAVAILABLE`.

The main paths are:

1. Property/navigation/text query: UIA MTA -> provider -> HWND dispatch -> manager query -> result.
2. Pattern action: UIA MTA -> provider -> HWND dispatch -> manager action -> result.
3. Hit test: UIA MTA -> root -> HWND dispatch -> deepest semantic node -> cached provider.
4. eepp event: UI thread mutation -> small pending event -> next manager update -> UIA raise call.
5. HWND destruction: UI thread -> disconnect/cache detach -> cancel dispatches -> remove subclass.

The provider/cache mutex is never held while calling the manager, user actions, another window
procedure, or UIA event APIs. Event emission is delayed until the next update so it cannot reenter a
half-mutated widget operation.

## Automated results

The MSVC x64 release harness passes all 16 groups in five consecutive expanded runs, including:

- late discovery and separate roots for multiple HWNDs;
- stable live runtime IDs and complete bidirectional raw navigation;
- representative properties and every exposed control pattern;
- UTF-16 document and selection ranges and protected-text exclusion;
- property, focus, text, selection, automation, and structure events;
- exact-PID HWND discovery and point hit testing;
- retained-provider invalidation after HWND destruction;
- 100 repeated dynamic-window create/query/destroy cycles with unique identities;
- closing a secondary window without invalidating the primary window;
- clean child-process and COM shutdown.

In the latest run, the 100-window lifecycle test completed in 8.44 seconds. Two hundred individual
name queries measured p50 0.269 ms, p95 0.534 ms, p99 0.915 ms, and maximum 2.387 ms. The event test
spends about two seconds attempting foreground activation because the validation session is a
non-interactive desktop; semantic focus state and its native property event still pass.

The complete release unit suite has five pre-existing failures on this checkout (two `eterm`
timing/drain tests, one text-format autodetection test that encounters a clangd cache binary, and
two image-diff view tests). The accessibility unit test and native Windows harness pass. No new
failure was observed.

### Performance

Nine interleaved optimized-release runs each performed 1,000,000 notification-heavy text
mutations. Disabled accessibility had a median of 891.587 ms (range 878.675--964.301 ms); a dormant
UIA backend had a median of 888.166 ms (range 871.460--941.320 ms). The measured median difference
was -0.38%, so dormant overhead is below the 1% target and indistinguishable from host noise.

With an active UIA client, normal individual queries remain below a small single-digit millisecond
budget at p99 in the automated sample. The test creates providers lazily and invalidates each
dynamic-window cache on teardown; the repeated identity/lifecycle run found no stale provider or
unbounded cache behavior.

## Compiler validation

- MSVC x64 release generation and build pass.
- The backend translation unit compiles successfully with MinGW-w64 GCC 15.2.0.
- Premake generation with `--windows-mingw-build` passes once its SDL MinGW developer package is
  present. A complete native MinGW target build was attempted but not accepted: this native build
  tree shares library names between MSVC and MinGW and also exceeds the Windows command-line limit
  in generated archive/link commands. Further MinGW build-system work was explicitly deferred.
  These build-layout failures are separate from the cleanly compiled UIA translation unit.

## Deliberate limitations and manual acceptance

Grid, GridItem, Table, TableItem, Scroll-container, Window, and `ITextProvider2` are not advertised.
The shared eepp model does not yet provide complete semantics for those contracts; returning a
pattern that only partly works would be worse than leaving it unsupported. The minimum complete
text contract is `ITextProvider`/`ITextRangeProvider` with character and document movement.

Per-monitor DPI and negative-screen-coordinate geometry need an interactive multi-monitor pass.
The automated test validates current-window physical bounds and deepest-element hit testing on the
available display. Foreground activation is conditional because Windows correctly denies it from
some non-interactive sessions.

Accessibility Insights/FastPass, visual Inspect and AccEvent review, Narrator keyboard use, mixed
DPI/negative-coordinate monitors, Visual Studio memory diagnostics, and Application Verifier were
not run by the non-interactive automation session. These remain the manual release-acceptance
checklist; their installed-tool status is recorded above rather than claimed as passing.
