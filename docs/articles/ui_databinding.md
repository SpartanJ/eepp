# UI Data Binding

## Introduction

eepp data binding connects application state to UI widget properties without making the state
depend on `UIWidget`. It is designed for small, explicit data flows: a model value changes, its
bound widgets update, and valid widget input can update the model again.

The API is available through `<eepp/ui.hpp>`. Individual headers live in
`<eepp/ui/databinding/>`, while the core observable containers live in `<eepp/core/>`.

Data binding is optional. Direct widget callbacks remain the clearest solution for isolated
interactions. These helpers become useful when state has multiple consumers, input needs typed
conversion or validation, several fields form one logical form, or an action is exposed through
both a widget and a keyboard shortcut.

## Choosing a type

| Need | Type |
| --- | --- |
| UI-independent observable state | `ObservableValue<T>` |
| A value calculated from observable dependencies | `ComputedValue` |
| Concise UI-local state owned together with its binding | `UIProperty<T>` |
| Two-way model-to-widget synchronization | `UIValueBinding<T>` |
| One-way synchronization from a read-only or calculated source | `UIReadOnlyValueBinding<T>` |
| Typed parsing, formatting, and input validation | `UIValueConverter<T>` |
| Aggregate validation, dirty state, reset, and error inspection | `UIBindingGroup` |
| One action shared by buttons and keyboard shortcuts | `UICommand` |
| A live vector exposed as a one-column model | `ObservableVector<T>` and `ObservableListModel<T>` |
| Delivery of worker-produced observable changes on the UI thread | `UIThreadObservation<T>` |
| Binding an existing externally owned value | `UIDataBind<T>` |

Bindings and observer connections are scoped objects. Keep the returned object alive for as long
as synchronization is required. Destroying it disconnects the relationship.

## Observable model values

`ObservableValue<T>` owns a value and synchronously notifies observers after distinct changes:

```cpp
ObservableValue<std::string> project( "eepp" );

auto connection = project.observe( []( const std::string& value ) {
    Log::info( "Project changed to: %s", value );
} );

project = "ecode";
```

The model remains independent from the UI. A model object can publish state without including UI
headers, and UI code can attach or disappear later.

Notifications run on the thread that changes the value. `ObservableValue` and its connections are
single-threaded; synchronize producer access when using worker threads.

Observer changes use snapshot semantics. Adding or disconnecting an observer during notification
takes effect on the next notification. Reentrant assignments are queued and the latest pending
value is delivered after the current observer pass.

## Binding a value to a widget

`bindValue()` creates a two-way `UIValueBinding<T>`. The current model value is immediately applied
to the widget. Later widget value changes are converted back into the model:

```cpp
ObservableValue<std::string> name( "Ada" );
auto nameBinding = bindValue( name, nameInput );

name = "Grace"; // Updates nameInput.
// Editing nameInput updates name.
```

Input widgets conventionally expose the `value` property and emit `Event::OnValueChange`, so the
default overload is normally enough. A different property can be selected explicitly:

```cpp
ObservableValue<bool> enabled( true );
auto enabledBinding = bindValue( enabled, saveButton, "enabled" );
```

Use `bindReadOnlyValue()` for calculated values or any source that should not be changed by the
widget:

```cpp
auto summary = computedValue( name, []( const std::string& value ) {
    return "Hello " + value;
} );

auto summaryBinding = bindReadOnlyValue( summary, summaryLabel );
```

Both binding types observe widget destruction and disconnect safely. They do not retain widgets.

## UI-local properties

`UIProperty<T>` owns a value and synchronizes it with one or more widgets. It is useful when state
belongs entirely to one UI screen and declaring a separate model value would add ceremony:

```cpp
UIProperty<std::string> filter( filterInput );

auto filterConnection = filter.observe( [&]( const std::string& prefix ) {
    updateFilter( prefix );
} );

filter = "Smith"; // Updates the widget and observers.
```

`UIProperty` implements the same observable-source interface as `ObservableValue`: `ValueType`,
`get()`, and `observe()`. It can therefore be used directly by `ComputedValue`, `UICommand`, and
`UIBindingGroup`.

Its value lives in retained shared storage. Notification publishes a lightweight change revision
instead of copying `T`, so a large `std::string` is not cloned for every observer. The retained
state also keeps the value alive if an observer destroys the property during notification.

Choose `ObservableValue` when state belongs to the model or must remain UI-independent. Choose
`UIProperty` when the value and its widgets naturally share one UI lifetime.

## Conversion and validation

Widget properties are strings. `UIValueConverter<T>` defines both directions:

- `toValue` parses widget text into `T` and may reject invalid input.
- `fromValue` formats an authoritative model value for the widget.

The common validation-only form supplies custom parsing and reuses the default formatter:

```cpp
auto validPort = UIValueConverter<int>(
    []( const CSS::PropertyDefinition*,
        const std::string& text ) -> UIValueResult<int> {
        int port = 0;
        if ( !String::fromString( port, text ) || port < 1 || port > 65535 )
            return UIValueResult<int>::error(
                101, "port must be between 1 and 65535" );
        return port;
    } );

ObservableValue<int> port( 8080 );
auto portBinding = bindValue( port, portInput, validPort );
```

Invalid widget input does not replace the model value. The binding exposes its current
`UIValueValidationState`, including an optional numeric code and diagnostic message.

Error codes are preferable to using diagnostic strings as application logic. Codes can be mapped
to localized user-facing messages, while `debugMessage` remains useful for tests and inspection.

When presentation requires custom formatting too, provide both converter functions:

```cpp
using Date = std::optional<std::time_t>;
UIValueConverter<Date> dateConverter(
    parseDateFromWidget,
    formatDateForWidget );
```

## Forms with UIBindingGroup

`UIBindingGroup` owns heterogeneous value bindings or tracks `UIProperty` objects. It aggregates:

- Current validity, ignoring disabled fields.
- Dirty state relative to the last clean baseline.
- Invalid widgets and their validation results.
- Reset and `markClean()` behavior.

```cpp
ObservableValue<std::string> project( "eepp" );
ObservableValue<std::string> host( "localhost" );
ObservableValue<int> port( 8080 );

UIBindingGroup form;
form += bindValue( project, projectInput, requiredText );
form += bindValue( host, hostInput, requiredText );
form += bindValue( port, portInput, validPort );
```

The group can drive validation styling and error messages from one callback:

```cpp
form.onChange( [&] {
    for ( auto widget : form.widgets() )
        widget->removeClass( "field-error" );

    for ( const auto& error : form.errors() )
        if ( error.widget )
            error.widget->addClass( "field-error" );
} );
```

The returned widget and error collections use inline storage for ordinary small forms.

`validValue()` and `dirtyValue()` are observable booleans. They compose naturally into derived
state:

```cpp
auto canSave = computedValue(
    form.validValue(), form.dirtyValue(),
    []( bool valid, bool dirty ) { return valid && dirty; } );
```

After a successful save, establish a new baseline:

```cpp
form.markClean();
```

`form.reset()` restores every field to the baseline recorded when it was added or most recently
marked clean.

## Computed values

`computedValue()` creates a read-only observable derived from explicit dependencies:

```cpp
auto endpoint = computedValue(
    host, port,
    []( const std::string& host, int port ) {
        return host + ":" + String::toString( port );
    } );

auto endpointBinding = bindReadOnlyValue( endpoint, endpointLabel );
```

Dependencies are cached and observed in argument order. Recalculation is synchronous. Equal
results are suppressed by the calculated output's `ObservableValue`.

Computed values do not own their dependencies. Keep dependencies alive while further updates are
expected.

## Commands and shortcuts

`UICommand` represents one action with observable enabled state. It is valuable when the same
action has multiple endpoints, such as a button and a keyboard shortcut. For a single button, an
ordinary `onClick()` remains simpler.

The concise binding overload creates and owns the command plus both endpoints:

```cpp
auto saveCommand = bindCommand(
    [&] {
        saveSettings();
        form.markClean();
    },
    canSave,
    *saveButton,
    *uiScene,
    { KEY_S, KeyMod::getDefaultModifier() } );
```

For an always-enabled command, omit the enabled source:

```cpp
auto refreshCommand = bindCommand(
    [&] { refresh(); },
    *refreshButton,
    *uiScene,
    { KEY_R, KeyMod::getDefaultModifier() } );
```

The returned binding must remain alive. It synchronizes the widget's enabled state, dispatches
clicks and shortcuts through the same action, prevents reentrant execution, and restores a
previous shortcut mapping when disconnected.

## Existing values with UIDataBind

`UIDataBind<T>` adapts a value that already exists outside the observable model types. The raw
pointer form is intentionally lightweight:

```cpp
bool showDetails = false;
auto binding = UIDataBind<bool>::New(
    &showDetails, detailsCheckBox,
    UIValueConverter<bool>::converterBool() );
```

The raw value must outlive the binding and every synchronous callback delivery. Direct writes
through the pointer are not observable; call `binding->set()` when a model-originated update must
reach widgets and observers.

Use the shared form when callbacks may destroy the original owner or otherwise require retained
storage:

```cpp
auto query = std::make_shared<std::string>();
auto binding = UIDataBind<std::string>::New( query, queryInput );
```

Shared bindings retain the original value through callback delivery without cloning `T`.
`UIProperty` uses this retained form internally.

## Observable collections

`ObservableVector<T>` is intended for collections that remain live while a view is attached. Its
explicit mutations emit incremental before/after notifications:

```cpp
ObservableVector<std::string> tasks( {
    "Build eepp",
    "Run unit tests",
} );

auto model = ObservableListModel<std::string>::create( tasks );
taskList->setModel( model );

tasks.pushBack( "Package release" );
tasks.set( 0, "Build release" );
tasks.erase( 1 );
```

Unfiltered models preserve incremental model notifications, allowing unaffected selection and
persistent indexes to survive. A formatter supports domain objects without coupling them to
`Variant`:

```cpp
auto model = ObservableListModel<Person>::create(
    people,
    []( const Person& person, ModelRole role ) {
        return role == ModelRole::Display
            ? Variant( person.surname + ", " + person.name )
            : Variant{};
    } );
```

Filters create a visible projection:

```cpp
model->setFilter( [prefix]( const Person& person ) {
    return String::istartsWith( person.surname, prefix );
} );
```

Use `sourceRow()` before mutating a filtered source from a view selection. Immutable option lists
and collections already managed by specialized models do not benefit from `ObservableVector`.

## Delivering worker changes to the UI thread

`ObservableValue` invokes callbacks on the thread that changes it. UI widgets must be touched only
from their owning UI thread. `observeOnUIThread()` bridges a worker-produced observable to a widget:

```cpp
auto progressObservation = observeOnUIThread(
    progress,
    *uiScene,
    *progressBar,
    []( UIWidget& widget, const float& value ) {
        widget.asType<UIProgressBar>()->setProgress( value );
    } );
```

The observation queues delivery through the scene scheduler and checks that the endpoint still
exists. Queued work becomes a no-op after the widget closes or the observation disconnects.

This helper does not make the source thread-safe. Construct and disconnect the observation only
while the producer is stopped or otherwise synchronized, and do not race observer-list mutation
with source mutation. If the source already changes on the UI thread, normal observation or value
binding is simpler.

## Lifetime checklist

- Retain bindings, commands, observations, and observer connections while they are needed.
- Model values must outlive bindings that refer to them.
- Widgets are not retained; bindings disconnect when widgets close.
- `ComputedValue` does not own its dependencies.
- Raw `UIDataBind` values must survive complete callback delivery.
- Shared `UIDataBind` and `UIProperty` retain their value during callbacks.
- UI operations and ordinary bindings belong on the widgets' UI thread.
- Synchronize worker-owned observable sources explicitly.

## Complete examples

- [`ui_data_handling`](https://github.com/SpartanJ/eepp/blob/develop/src/examples/ui_data_handling/ui_data_handling.cpp)
  demonstrates conversion, validation, form state, computed summaries, commands, and shortcuts.
- [`ui_data_collections`](https://github.com/SpartanJ/eepp/blob/develop/src/examples/ui_data_collections/ui_data_collections.cpp)
  demonstrates a live observable collection and incremental list model.
- [`7guis/flight_booker`](https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/flight_booker/flight_booker.cpp)
  demonstrates reactive cross-field validation with `UIProperty`.
- [`7guis/crud`](https://github.com/SpartanJ/eepp/blob/develop/src/examples/7guis/crud/crud.cpp)
  demonstrates UI-local properties, filtering, selection state, and editable observable rows.
