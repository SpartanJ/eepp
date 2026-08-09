# UI Data Handling Modernization Plan

Status: Stage 1 implemented and under review; Stage 2 is next, 2026-08-09.

Baseline commit: `01d5614a7 ui: add scoped event and observable value bindings`

## Goal

Build a modern, predictable data-handling layer for eepp's retained-mode UI without imposing a
React-like component model, virtual DOM, immutable application state, or mandatory reactive
architecture.

The system must preserve direct widget manipulation and the existing model/view APIs while making
safe value synchronization, validation, derived state, commands, background delivery, forms, and
diagnostics available as composable C++ tools.

The target layering is:

```text
Application state                    UI-local state              Existing raw state
ObservableValue<T>                   UIProperty<T>               T*
        |                                  |                      |
UIValueBinding<T>                    UIDataBind<T> <--------------+
        |                                  |
        +------------- UIValueConverter<T> +
                           |
                        UIWidget

Collections -> Model adapters -> UIAbstractView
Commands ---------------------> buttons / menus / shortcuts
```

The current classes retain distinct purposes:

- `EventConnection`: scoped lifetime for a `Node` listener. It does not replace widget-level
  `Event::OnClose` lifecycle notification.
- `UIDataBind<T>`: low-level adaptation of an externally owned `T*`. The caller controls and must
  prove the value lifetime.
- `UIProperty<T>`: inexpensive owned UI-local value and bidirectional widget binding.
- `ObservableValue<T>`: UI-independent owned state whose observer types are unknown to the model.
- `UIValueBinding<T>`: scoped bidirectional adapter between an `ObservableValue<T>` and a widget.
- `UIValueConverter<T>`: conversion policy shared by raw and observable bindings.
- Existing `Model` / view classes: structured and potentially large collection presentation.

Do not collapse these classes merely to share implementation. Their ownership and coupling models
are intentionally different.

## Decisions Already Made

1. Typed conversion results and observable field error state were completed in Stage 1.
2. Layout-update batching is out of scope. eepp already queues/coalesces layout invalidation, so a
   generic observable transaction would add complexity without a demonstrated problem.
3. Scripting is out of scope for this roadmap. The C++ lifetime, validation, command, collection,
   and inspection foundations come first.
4. The system remains retained-mode. Reactive features update persistent widgets and models; they
   do not reconstruct a virtual widget tree.
5. All current event, observable, binding, and widget mutation remains single-threaded unless an
   explicit UI-thread delivery adapter is used.
6. New facilities must be opt-in. Existing event handlers, widget setters, and custom `Model`
   implementations remain valid and are often the clearest solution.
7. Every scoped observer or binding must be safe when either endpoint is destroyed first.
8. Conversion, validation, and binding errors must be inspectable. Silent failure is not an
   acceptable final design.

## Priority Order

1. Rich conversion and validation results.
2. Form/binding groups and aggregate validation.
3. Computed/derived observable values.
4. Explicit UI-thread observation and delivery.
5. Commands and reactive command state.
6. Observable collections and incremental model adapters.
7. Binding/observable inspection tooling.

This order is dependency-driven: form groups consume validation state; commands benefit from
computed values; inspection should understand every final primitive rather than being repeatedly
redesigned.

---

# Stage 1: Typed Conversion and Field Error State

Status: implemented locally; build and 924-test ASAN suite pass.

## Objective

Replace output-parameter converters and their bare `bool` result with typed results that either
contain an accepted value or identify a failure. Expose the current error state from both
`UIDataBind` and `UIValueBinding` without adding a second validator pass or a general-purpose
binding pipeline.

`UIValueConverter<T>` is intentionally the only input policy. Its `toValue()` callback performs
whatever parsing and field-local acceptance a use case needs, while `fromValue()` formats
authoritative model values. More specialized composition belongs in application code until a
repeated concrete use case justifies another shared abstraction.

Validation errors should be machine-readable first. UI code normally maps a stable numeric error
code to localized text; the optional string is a technical diagnostic for logs, tests, and
inspection rather than the default user-facing message. This stage must not force converters to
allocate an error string on success or on ordinary coded failures.

## Implemented result type

The public result and observable error state live in:

```text
include/eepp/ui/uivaluevalidation.hpp
```

Implemented result shape:

```cpp
struct UIValueValidationResult {
	using Code = Uint32;

	bool valid{ true };
	std::optional<Code> code;
	std::optional<std::string> debugMessage;

	static UIValueValidationResult success();
	static UIValueValidationResult error( Code code );
	static UIValueValidationResult error( Code code, std::string debugMessage );
	static UIValueValidationResult error( std::string debugMessage );
	explicit operator bool() const { return valid; }
};
```

`Code` is intentionally numeric and `0` is not overloaded to mean “no code”; the disengaged
`std::optional` represents absence. Codes are defined by the subsystem or application that owns
the acceptance rule. `UIValueValidationError` reserves documented values for built-in converter
failures. Codes are not otherwise globally unique or stable for serialization unless a later API
introduces an error domain.

An error may have only a code, only a diagnostic, or both. Coded errors are the normal application
path. Diagnostic-only errors remain useful for ad-hoc acceptance rules and converter migration, but UI
presentation must not depend on English diagnostic text.

The selected public name is `UIValueValidationResult`: converter failures can represent syntax,
range, or other field-local acceptance errors without introducing nearly identical result types.
Success and code-only errors are allocation-free; diagnostics allocate only when supplied.

## Converter migration

Change `UIValueConverter<T>` callbacks from:

```cpp
std::function<bool( const PropertyDefinition*, T&, const std::string& )>
std::function<bool( const PropertyDefinition*, std::string&, const T& )>
```

to:

```cpp
std::function<UIValueResult<T>( const PropertyDefinition*, const std::string& )>
std::function<UIValueResult<std::string>( const PropertyDefinition*, const T& )>
```

Returning values instead of mutating output parameters prevents failed converters from leaking
partial output and makes the binding's early-return behavior explicit.

This is intentionally source-breaking for custom converters. A legacy `return false` is ambiguous
once conversion must return a typed value and could accidentally become a successful boolean or
numeric value. Migrate custom converters explicitly and do not retain duplicate output-parameter
adapters. Infallible converters may return a raw `T`; fallible converters use
`UIValueResult<T>::error()`.

Default parse failures should use documented core error codes and may additionally produce useful
diagnostics containing the rejected text and expected type/category when practical. Do not
localize low-level converter diagnostics; preserve technical text suitable for logs and inspector
tooling. Applications translate the code (plus their binding/form context) through their own i18n
layer.

## Property conversion

Bindings call `UIValueConverter<T>` directly in both directions:

```text
model output: T -> widget property string
widget input: property string -> accepted T or error
```

Presentation-specific formatting such as currencies, percentages, and units belongs in
`UIValueConverter<T>`. More elaborate typed adaptation can be implemented by applications if a
concrete use case requires it.

## Field-local acceptance

Parsing and semantic acceptance are conceptually different, but both are part of the converter's
single widget-input operation:

```text
widget string -> UIValueConverter<T>::toValue() -> accepted T or error
```

Examples:

- `"abc"` cannot convert to an integer.
- `-1` parses as an integer but may still be rejected by the converter's acceptance rules.
- A path converts to a string but may not exist.
- A return date converts successfully but may precede the departure date.

Do not add a second validation pass to every binding. A custom converter can reuse parsing or
validation helpers internally when an application needs them. Do not put asynchronous validation
in Stage 1.

## Binding state and API

Both `UIDataBind<T>` and `UIValueBinding<T>` must expose their current validity without requiring
knowledge of the other class. The common state is:

```cpp
class UIValueValidationState {
  public:
	bool isValid() const;
	const std::optional<UIValueValidationResult::Code>& code() const;
	const std::optional<std::string>& debugMessage() const;
	Connection observe( Callback );
};
```

Validation observation reuses `ObservableValue` internally and allocates its observer storage only
when observed. `ObservableValue` does not depend on UI headers.

Required binding behavior:

1. Successful input acceptance updates the model value and clears the previous error.
2. Rejected input leaves the last accepted model value unchanged.
3. The originating widget may retain its invalid text so the user can correct it.
4. Other widgets bound to the same value must continue showing the last valid model value; invalid
   text must not propagate to them.
5. Converter acceptance applies to UI-originated proposals. Programmatic `set()` and external
   `ObservableValue` changes are authoritative model updates; formatting can still fail and be
   reported.
6. Model-to-widget conversion failure must not apply an empty or partial property string.
7. Repeated identical validation errors should not emit duplicate state-change notifications.
8. Widget destruction clears its validation contribution safely.

## Widget presentation policy

Stage 1 deliberately does not hard-code an error class, tooltip, or localized message into the
binding core. Numeric codes and optional diagnostics remain independently observable. Stage 2 must
decide how a form maps field and cross-field errors to localized presentation after auditing the
existing invalid/error widget states and theme conventions.

## API compatibility audit

Audit and migrate:

- all `UIValueConverter` construction;
- `UIDataBind<T>::converterDefault/String/Bool()` compatibility forwarders;
- ecode's `ProjectOutputParserTypes` converter;
- `UIProperty` constructor defaults;
- all unit tests and examples;
- any downstream-style public callback signatures exposed in headers.

Document source-breaking changes clearly because `UIDataBind::Converter` was public before this
roadmap.

## Stage 1 verification

Implemented focused coverage includes:

- successful default conversion;
- conversion failure with a code and no diagnostic allocation;
- diagnostic-only and code-plus-diagnostic failures;
- field-local acceptance failure after parsing;
- model-originated values remain authoritative even when equivalent widget input would be rejected;
- invalid widget text does not change the model;
- invalid widget text does not propagate to sibling widgets;
- later valid input clears the error and updates every widget;
- failed model-to-widget conversion does not apply a property;
- programmatic set validation behavior;
- repeated identical error suppression, comparing validity, code, and diagnostic;
- widget-first, binding-first, and value-first destruction while invalid;
- custom converter parsing, formatting, and acceptance;
- formatted currency-style values in both directions.

The project builds with the ASAN debug configuration, all focused binding tests pass, and the full
suite passes 924/924. Presentation/localization and cross-field behavior remain Stage 2 concerns.
Use Flight Booker as a design reference for that phase, but do not migrate it until the resulting
form API is clearly shorter and more expressive than its current explicit validation function.

---

# Stage 2: Form and Binding Groups

## Objective

Provide ownership and aggregate validation for related bindings without turning every form into a
new framework.

The group must solve two separate concerns:

1. Stable ownership of heterogeneous bindings.
2. Aggregate state such as valid, dirty, commit, reset, and first error.

## Proposed API direction

Explore extending or replacing the narrowly typed `UIDataBindHolder` classes with a type-erased
scoped binding interface:

```cpp
class UIValueBindingBase {
  public:
	virtual ~UIValueBindingBase() = default;
	virtual void disconnect() = 0;
	virtual bool isConnected() const = 0;
	virtual bool isValid() const = 0;
	virtual const std::optional<UIValueValidationResult::Code>& validationCode() const = 0;
	virtual const std::optional<std::string>& validationDebugMessage() const = 0;
};
```

Avoid virtual dispatch if a small type-erased value holder can provide the same ownership cleanly.
Measure complexity before choosing.

Candidate user API:

```cpp
UIBindingGroup form;
form += bindValue( config.name, nameInput, stringConverter, "text", Event::OnTextChanged );
form += bindValue( config.path, pathInput, stringConverter, "text", Event::OnTextChanged );

saveButton->setEnabled( form.isValid() && form.isDirty() );
form.onValidationChange( ... );
```

## Required semantics

- Destruction or `clear()` disconnects every binding.
- A group may contain `UIValueBinding`, `UIDataBind`, and optionally non-binding validators.
- Aggregate validity updates when a child binding changes validity or disappears.
- The group exposes every child's code, optional diagnostic, and originating binding/widget, plus
  the first invalid widget for focus/navigation. The group must preserve the context needed to map
  application-defined codes to localized messages.
- Dirty state compares against an explicit baseline, not merely “received an event.”
- `markClean()` establishes a new baseline after save.
- `reset()` restores the baseline where values are copyable.
- Commit/rollback must be opt-in; not every live configuration form uses temporary state.
- A group must not own widgets or observable model values.

## Tests

- heterogeneous binding ownership;
- aggregate validity transitions;
- first invalid widget behavior;
- widget destruction while invalid;
- dirty/clean baseline behavior;
- reset and mark-clean;
- group destruction before and after endpoints;
- no duplicate aggregate notification when state is unchanged.

---

# Stage 3: Computed and Derived Observable Values

## Objective

Represent read-only state derived from one or more observables while preserving synchronous,
deterministic retained-mode updates.

## Design constraints

- Do not implement implicit dependency tracking by executing arbitrary lambdas and recording reads.
- Dependencies must be explicit in the initial implementation.
- Computed values are read-only to consumers.
- Dependency connections are scoped and expire safely.
- Equality suppression should match `ObservableValue` behavior.
- Reentrant updates and cycles must have defined behavior before merging.

## Candidate API

```cpp
auto fullName = computedValue(
	firstName, lastName,
	[]( const std::string& first, const std::string& last ) {
		return first + " " + last;
	} );

auto canSave = computedValue(
	formValid, formDirty,
	[]( bool valid, bool dirty ) { return valid && dirty; } );
```

The returned type should expose the read/observe subset of `ObservableValue`, not `set()`.

## Reentrancy decision required

The current synchronous `ObservableValue` allows an observer to set the same value during
notification. Before computed values are implemented, define and test one policy:

1. Nested immediate notifications.
2. Queue the latest value until the current notification completes.
3. Reject/assert reentrant mutation.

Recommended direction: queue the latest distinct value and drain synchronously after the current
observer snapshot completes. This avoids observers seeing later values twice during an older
notification while retaining synchronous completion before `set()` returns.

Cycle detection must report a clear diagnostic in debug builds rather than recurse indefinitely.

## Tests

- one and multiple dependencies;
- registration-order observation;
- equality suppression;
- dependency destruction;
- computed destruction;
- chained computed values;
- diamond dependency graph behavior;
- reentrant source updates;
- direct and indirect cycles;
- binding a computed value one-way to a widget.

---

# Stage 4: Explicit UI-Thread Delivery

## Objective

Allow state produced on worker threads to be delivered safely to the UI without making
`ObservableValue`, `EventConnection`, or widgets internally thread-safe.

## Proposed direction

Use existing `Node::runOnMainThread()` / `ensureMainThread()` infrastructure. Candidate APIs:

```cpp
auto connection = observeOnUIThread( value, widget, callback );
```

or:

```cpp
auto uiValue = deliverOnUIThread( source, uiSceneOrNode );
```

The adapter must capture only lifetime-safe handles. It must not queue a raw widget pointer that can
die before execution.

## Required semantics

- Source observation may occur on the source's owning thread.
- Widget mutation occurs only on the UI thread.
- Destruction before queued delivery makes the delivery a no-op.
- Define whether every value is delivered or only the latest pending value. Provide explicit names
  if both modes are needed.
- Preserve order for non-coalesced delivery.
- No blocking cross-thread calls.
- Clearly document that base `ObservableValue` remains single-threaded; a producer must serialize
  mutation or use a separate synchronized source adapter.

## Tests

- worker-to-UI delivery;
- endpoint destruction before execution;
- connection destruction before execution;
- ordered delivery;
- latest-value coalescing, if supported;
- UI-thread immediate fast path;
- sanitizer coverage where available.

---

# Stage 5: Commands

## Objective

Represent user actions separately from values and bind one action consistently to buttons, menus,
keyboard shortcuts, toolbars, and command palettes.

## Candidate API

```cpp
Command save{
	[&] { saveProject(); },
	canSave
};

auto buttonBinding = bindCommand( save, saveButton );
auto menuBinding = bindCommand( save, saveMenuItem );
```

## Command state

Explore:

- enabled;
- checked/toggled;
- visible, only if a real use case requires it;
- label and icon metadata;
- shortcut metadata;
- execution callback;
- optional parameter type for reusable commands.

Prefer observable/computed state inputs over command-owned ad hoc listener APIs.

## Required behavior

- Disabled commands cannot execute through any bound endpoint.
- Multiple UI representations stay synchronized.
- Endpoint and command destruction are safe in either order.
- Reentrant execution policy is explicit.
- Asynchronous command progress/cancellation is deferred unless a concrete ecode workflow requires
  it during implementation.

## Tests and candidate migrations

- bind one command to button and menu item;
- enabled and checked propagation;
- shortcut execution;
- endpoint destruction;
- command destruction;
- duplicate execution prevention;
- consider ecode actions already represented in menus/toolbars as the primary real-world audit;
- Circle Drawer undo/redo is a useful small example for enabled-state command binding.

---

# Stage 6: Observable Collections and Model Adapters

## Objective

Bridge ordinary application collections to eepp model/view components with incremental updates,
without replacing custom models such as Cells or forcing every collection to be observable.

## Research first

Audit existing `Model` invalidation/update flags and selection preservation before defining new
collection notifications. Reuse established model vocabulary where possible.

## Candidate change vocabulary

```cpp
inserted( index, count );
removed( index, count );
moved( from, to, count );
changed( index, count );
reset();
```

Candidate types:

```cpp
ObservableVector<T>
ObservableCollection<T>
CollectionModelAdapter<T>
```

Avoid exposing mutable container references that bypass notifications. Mutation should happen
through explicit operations or an edit guard that emits one well-defined change.

## Required behavior

- Incremental model updates preserve unaffected indexes and selection.
- Removal invalidates only affected indexes according to existing model contracts.
- Batch operations emit one range update where possible.
- Collection and view/model adapter destruction are safe in either order.
- Large collections do not copy their contents for every notification.
- Thread behavior is explicit and uses Stage 4 adapters when needed.

## Candidate audits

- CRUD's people list for basic insertion/removal/filtering.
- Application configuration lists in ecode.
- Do not migrate Cells: it has a specialized formula dependency graph and custom model semantics.

## Tests

- insert/remove/move/change/reset;
- selection preservation;
- filtering/sorting proxy interaction;
- adapter destruction;
- large range changes;
- mutation during model notification;
- stable identity where rows represent long-lived objects.

---

# Stage 7: Inspection and Diagnostics

## Objective

Make invisible data flow understandable in the existing widget inspector and debug tooling.

This stage follows the functional primitives so it can expose one coherent model.

## Inspectable information

For a widget:

- active event connections by type and registration ID;
- active `UIDataBind` / `UIValueBinding` property bindings;
- binding direction and converter type/name where available;
- current model value in a safe string representation;
- last widget value received;
- validity, numeric validation code, and optional diagnostic message;
- connected/expired endpoint status;
- owning binding/form group;
- last propagation timestamp or sequence number in debug builds;
- command bindings;
- model/collection adapter information.

For an observable:

- observer count;
- computed dependencies and dependents;
- current notification/reentrancy state;
- thread-affinity owner in debug builds;
- last validation or delivery error.

## Instrumentation constraints

- Release builds should not pay for names, timestamps, graph edges, or stack traces unless an
  existing debug/inspection flag enables them.
- Do not expose raw pointers as stable identities in user-facing output.
- Inspector observation must not change lifetime or keep endpoints alive.
- Diagnostics must not recursively trigger the binding being inspected.

## Tests

- inspection does not retain endpoints;
- disconnected and expired state visibility;
- validation code and optional diagnostic visibility;
- command and computed dependency display;
- debug instrumentation compiled out or minimized in release configuration.

---

# Explicitly Deferred Work

## Generic observable transactions

Deferred because eepp already coalesces layout invalidation. Reconsider only with profiling evidence
of expensive non-layout observers repeatedly recomputing during bulk configuration changes.

## Scripting

Deferred until the C++ APIs and inspection model stabilize. A future scripting bridge should expose
the same primitives rather than inventing a separate lifetime system:

- `EventConnection`;
- observable values and computed values;
- UI bindings and validation;
- commands;
- models/collection adapters.

The scripting design must explicitly solve VM/context destruction, callback disconnection, dynamic
type conversion, error reporting, and UI-thread delivery.

## Virtual DOM / mandatory declarative components

Not planned. Users may build a React-like layer on these primitives, but eepp's core remains a
retained widget tree with direct, predictable C++ control.

---

# Cross-Stage Quality Requirements

Every stage must:

1. Preserve current direct widget/event APIs.
2. Document ownership, thread affinity, notification order, and destruction behavior.
3. Use scoped connections for all callbacks that capture object addresses.
4. Avoid per-update allocation on successful common paths where practical.
5. Preserve registration-order dispatch.
6. Define reentrancy before exposing APIs that can form cycles.
7. Add focused destruction-order and mutation-during-notification tests.
8. Run clang-format on modified C/C++ files.
9. Run `git diff --check`.
10. Build with the project's ASAN debug configuration.
11. Run focused tests first, followed by the full unit-test suite before each stage is considered
    complete.
12. Audit at least one real eepp or ecode workflow before accepting a new abstraction.

# Immediate Next Action

Review and commit Stage 1, then design Stage 2 around the implementation that now exists. Audit
`UIDataBindHolder`, existing form-like screens, invalid/error widget styling, and theme conventions.
The Stage 2 design must keep field-local converter errors separate from cross-field/form errors,
aggregate observable `UIValueValidationState` instances without retaining widgets, and avoid
adding machinery to ordinary bindings solely for form use cases.
