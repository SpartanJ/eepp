# Dense CSS Property and Shorthand IDs / Bitset Migration Plan

Status: implementation-ready plan, 2026-08-05.

## Goal

Replace hash-valued CSS `PropertyId` and `ShorthandId` identifiers with compact, contiguous IDs,
and replace the heap-allocating `PropertyIdSet` hash set with a fixed-capacity bitset. Preserve
string/hash lookup at parsing and public name-based entry points, while making all resolved-property
identity, set algebra, and state-change traversal allocation-free.

The implementation must also remove the existing accidental dependency between CSS property
application order and replaced-image sizing. A bitset has a deterministic numeric iteration order,
which differs from the current `UnorderedSet` bucket order. The migration is not allowed to encode
the old bucket order into enum values or otherwise depend on a lucky order.

This plan includes a separate dense `ShorthandId` namespace and name map. Shorthand IDs must never
be cast to `PropertyId`, even when a shorthand and a longhand share a spelling such as `transition`,
`flex`, `grid`, or `gap`.

## Decisions Already Made

The implementer must use these choices; none are left open:

1. `PropertyId` uses `Uint16`, not `Uint8`. eepp currently registers 307 longhand definitions and
   the enum currently contains 316 names. Even before custom properties, that cannot fit in an
   8-bit ID. Splitting widget-specific longhands into unrelated ID spaces would prevent one
   `PropertyIdSet` and make dispatch substantially more complex, so it is explicitly rejected.
2. `PropertyId::Invalid` is zero. Built-ins are contiguous from one through the last built-in.
   `PropertyId::NumDefinedIds` is the exclusive end of built-ins, and
   `PropertyId::FirstCustomId` has the same numeric value.
3. `PropertyId::MaxNumIds` is 512 and is an exclusive upper bound. Valid IDs are `[1, 511]`.
   The initial tree therefore has room for at least 195 runtime-registered properties. Exhaustion
   logs an error and registration fails without aliasing or wrapping an ID.
4. `ShorthandId` uses `Uint8`. `Invalid` is zero, built-ins are contiguous, `NumDefinedIds` and
   `FirstCustomId` mark the exclusive built-in end, and `MaxNumIds` is 255 (exclusive). Valid IDs
   are `[1, 254]`. The 37 current shorthand registrations leave ample custom capacity.
5. Numeric IDs are process-local implementation identifiers. They are not persisted, serialized,
   sent over IPC, or exposed as stable values. Names are the stable external representation.
6. Built-in IDs are explicitly supplied at registration. Custom IDs are assigned monotonically by
   the corresponding name map after all built-ins have been registered. Runtime unregistration and
   ID reuse are not supported.
7. Existing name hashes remain where they serve a different purpose: `StyleSheetProperties`,
   animation action tags, CSS variables, selectors, and data properties remain keyed by
   `String::HashType`. `PropertyDefinition::getId()` and `ShorthandDefinition::getId()` continue to
   return the canonical name hash for source compatibility. Dense identity is returned only by
   `getPropertyId()` and `getShorthandId()`.
8. Unknown CSS names and arbitrary `data-*` names do not consume dense IDs. Only successfully
   registered property and shorthand definitions receive IDs. CSS variables remain entirely
   outside both ID spaces.
9. `PropertyIdSet` stores exactly `std::bitset<512>`. It accepts and yields `PropertyId`, never raw
   hashes or `Uint32`. Its iteration order is ascending dense ID order.
10. Property application must be correct for any traversal order. The pre-migration sizing fix in
    Stage 1 is mandatory and must land before the set implementation changes.

## Confirmed Iteration-Order Bug

The prior `SmallVector` experiment did not expose a WebP decoder or test-CWD issue. It exposed a
real ordering dependency in style application:

- `UIStyle::onStateChange()` builds a set of changed properties and iterates it while an attributes
  transaction is active.
- `UIWidget::beginAttributesTransaction()` / `endAttributesTransaction()` only coalesce layout
  notifications. They do not defer property setters or make the property update atomic.
- `UIHTMLImage::onSizeChange()` and `onSizePolicyChange()` call `autoSizeImage()` immediately.
- `UIWidget::setMaxWidthEq()` calls the virtual `onSizeChange()` immediately.
- `autoSizeImage()` reads the current width policy, height policy, `mMaxWidthEq`, `mMaxHeightEq`,
  padding, drawable dimensions, and current size. During the loop these values may represent a
  partially applied style.
- The regression fixture combines HTML `width="2560" height="1436"` with CSS `height:auto` and
  `max-width:100%`. A changed iteration order can therefore calculate from an intermediate fixed
  width/height state and leave zero or stale geometry. The outer transaction does not currently
  perform a final sizing reconciliation.

The correct fix is a final-state reconciliation at the outermost transaction boundary. Preserving
the old unordered bucket order, insertion order, or a hand-selected enum order is forbidden: those
approaches leave the same bug available for stylesheet changes, aliases, custom registrations, and
different standard-library hash layouts.

## Target Types and APIs

### `PropertyId` and `ShorthandId`

Move both enum declarations into a new public header:

`include/eepp/ui/css/propertyids.hpp`

Use the following shape (with all built-in enumerators listed explicitly between the markers):

```cpp
enum class PropertyId : Uint16 {
	Invalid = 0,
	Id,
	Class,
	// Every registered longhand, exactly once.
	Defer,
	NumDefinedIds,
	FirstCustomId = NumDefinedIds,
	MaxNumIds = 512,
};

enum class ShorthandId : Uint8 {
	Invalid = 0,
	Margin,
	// Every registered shorthand, exactly once.
	PlaceContent,
	NumDefinedIds,
	FirstCustomId = NumDefinedIds,
	MaxNumIds = 255,
};
```

Keep the current `PropertyId` declaration order to minimize switch/source churn, but reconcile it
against `StyleSheetSpecification::registerDefaultProperties()` before assigning the final list:

- add an enum member for every registered longhand missing from the current enum;
- remove from `PropertyId` any name that is only a shorthand and has no longhand registration;
- keep names that are both a registered longhand and a shorthand in both enums;
- update every switch/caller for any member moved out of `PropertyId`;
- add compile-time checks that `NumDefinedIds < MaxNumIds` for both enums.

The built-in `ShorthandId` list must exactly match these 37 current registrations, including names
missing from today's hash enum:

`Margin`, `LayoutMargin`, `LayoutMarginUnderscore`, `Padding`, `Background`, `Foreground`,
`BoxMargin`, `BackgroundPosition`, `ForegroundPosition`, `BorderColor`, `BorderWidth`,
`BorderStyle`, `BorderRadius`, `ForegroundRadius`, `RotationOriginPoint`, `RotateOriginPoint`,
`ScaleOriginPoint`, `MinSize`, `MaxSize`, `Border`, `TextShadow`, `HintShadow`, `BorderLeft`,
`BorderRight`, `BorderTop`, `BorderBottom`, `ListStyle`, `Font`, `VerticalAlign`, `FlexFlow`, `Flex`,
`Gap`, `GridTemplate`, `Grid`, `PlaceItems`, `PlaceSelf`, and `PlaceContent`.

Use `ListStyle` as the corrected C++ spelling; remove the current `ListStye` typo and update its
callers. `layout-margin` and `layout_margin`, and `rotation-origin-point` and
`rotate-origin-point`, remain distinct built-in shorthand IDs because they are independently
registered definitions today.

### Generic `IdNameMap`

Add the reusable header:

`include/eepp/ui/css/idnamemap.hpp`

Implement `template <typename Id, std::size_t MaxIds> class IdNameMap`. It owns:

- `std::vector<std::string> mNames`, indexed by the underlying numeric ID;
- `UnorderedMap<std::string, Id> mIdsByName`, using exact normalized canonical/alias names;
- the next custom ID, initialized to `Id::FirstCustomId` after built-in registration is finalized.

Required operations and behavior:

```cpp
bool addBuiltin( Id id, const std::string& canonicalName );
bool addAlias( const std::string& alias, Id target );
bool finalizeBuiltins();
Id getId( std::string_view name ) const;
const std::string& getName( Id id ) const;
Id getOrCreateId( const std::string& canonicalName );
bool contains( Id id ) const;
bool contains( std::string_view name ) const;
```

Implementation invariants:

- construct index zero as `Invalid` with an empty name;
- `addBuiltin()` is valid only before custom allocation begins, rejects `Invalid`, rejects IDs at
  or beyond `MaxIds`, rejects duplicate names, and rejects assigning a different name to an
  occupied ID;
- it resizes `mNames` to `id + 1`, so explicitly registered built-ins may be checked even if a
  mistake introduces a gap;
- `addAlias()` adds only the reverse entry and does not occupy an ID or replace the canonical name;
- `finalizeBuiltins()` verifies every slot `[1, FirstCustomId)` is populated, sets the next custom
  ID to `FirstCustomId`, and permanently rejects later `addBuiltin()` calls;
- `getOrCreateId()` is rejected until `finalizeBuiltins()` succeeds;
- `getId()` returns `Invalid` for an unknown name;
- `getName()` returns the empty invalid name for invalid/out-of-range/unassigned IDs;
- `getOrCreateId()` returns an existing canonical/alias target when present; otherwise it assigns
  `mNextCustomId`, increments it, and appends the canonical name;
- reaching `MaxIds` returns `Invalid` and logs a clear capacity error containing the rejected name;
- all names passed into the map are already lowercase and trimmed by the caller. The map performs
  no hidden normalization and no hash-only equality.

Do not copy RmlUi's source verbatim. Reimplement this behavior using eepp containers, types,
logging, and formatting conventions.

### Definition and specification ownership

Change constructors to receive dense identity:

```cpp
PropertyDefinition( PropertyId propertyId, const std::string& name,
					const std::string& defaultValue, bool inherited = false );
ShorthandDefinition( ShorthandId shorthandId, const std::string& name,
					 const std::vector<std::string>& properties,
					 const std::string& shorthandFuncName );
```

Each definition stores both identities:

- `mPropertyId` / `mShorthandId`: dense dispatch identity;
- `mId`: existing canonical name hash used by name-keyed structures and action tags.

`StyleSheetProperty::getId()` remains the key for `StyleSheetProperties`: return the canonical
definition hash for a resolved property or shorthand, and return `mNameHash` for an unresolved
name. This preserves canonical alias coalescing while ensuring distinct unknown/`data-*` names do
not collapse onto `Invalid`/zero. Add explicit `getPropertyId()` and `getShorthandId()` accessors
that return their respective dense IDs or `Invalid`; callers must use the accessor matching the
definition kind.

`PropertySpecification` owns:

- `IdNameMap<PropertyId, 512> mPropertyIds`;
- `IdNameMap<ShorthandId, 255> mShorthandIds`;
- `std::vector<std::shared_ptr<PropertyDefinition>> mPropertiesById` indexed by dense ID;
- `std::vector<std::shared_ptr<ShorthandDefinition>> mShorthandsById` indexed by dense ID;
- `PropertyIdSet mInheritableProperties` instead of `SmallVector<PropertyId>`.

Resize the definition vectors on registration to `id + 1`; do not preconstruct 512 strings or 512
`shared_ptr`s. Name lookup first resolves the name through the appropriate `IdNameMap`, then indexes
the definition vector. ID lookup indexes directly after validating range.

Expose these exact overloads through both `PropertySpecification` and
`StyleSheetSpecification`:

```cpp
PropertyDefinition& registerProperty( PropertyId id, const std::string& name,
									  const std::string& defaultValue,
									  bool inherited = false );
PropertyDefinition* registerProperty( const std::string& name,
									  const std::string& defaultValue,
									  bool inherited = false );
const PropertyDefinition* getProperty( PropertyId id ) const;
const PropertyDefinition* getProperty( const std::string& name ) const;

ShorthandDefinition& registerShorthand( ShorthandId id, const std::string& name,
										const std::vector<std::string>& properties,
										const std::string& parserName );
ShorthandDefinition* registerShorthand( const std::string& name,
										const std::vector<std::string>& properties,
										const std::string& parserName );
const ShorthandDefinition* getShorthand( ShorthandId id ) const;
const ShorthandDefinition* getShorthand( const std::string& name ) const;
```

Remove public `getProperty(Uint32)`, `getShorthand(Uint32)`, and `isShorthand(Uint32)` overloads so
a name hash cannot accidentally bind to a dense-ID lookup. Callers that possess both a cached hash
and the full name must use the full-name overload; do not add a hash-only fast path to `IdNameMap`.

Built-in registration returns references and treats a mismatch as a programming error: log an
error and assert in debug if an explicit ID is already bound to another name, a name is bound to
another ID, or a built-in slot is skipped. Runtime registration returns a pointer so exhaustion can
return `nullptr`. Preserve current duplicate semantics: a duplicate non-prefixed name warns and
returns the existing definition; a duplicate name beginning with `-` replaces its definition in
the same dense slot (without allocating another ID). Do not duplicate its inheritable-set entry.

`PropertyDefinition::addAlias()` must call `PropertySpecification::addPropertyAlias(alias,
mPropertyId)`. Preserve the definition's existing alias strings/hashes for `isAlias()` callers,
but make the specification's full-string reverse map authoritative. Apply the equivalent behavior
to `ShorthandDefinition::addAlias()` if shorthand aliases are added later; no `ShorthandIdMap`
class separate from the generic `IdNameMap` is needed.

### Built-in registration

Convert every call in `StyleSheetSpecification::registerDefaultProperties()` from name-only form
to explicit form:

```cpp
registerProperty( PropertyId::Width, "width", "" )...;
registerShorthand( ShorthandId::Margin, "margin", {...}, "box" );
```

The enum spelling, canonical string, and registration call form one audited table even though they
remain written as enum declarations plus registration code. Add a debug-only final validation at
the end of default registration that:

- every numeric built-in slot `[1, NumDefinedIds)` has exactly one definition;
- every definition's dense ID round-trips through its canonical name;
- every canonical name round-trips to the same definition;
- the first custom ID equals `NumDefinedIds`;
- the count of registrations equals `NumDefinedIds - 1` independently for properties and
  shorthands.

This validation prevents a newly added enum or registration from silently shifting custom IDs or
leaving an unregistered bit.

### `PropertyIdSet`

Rewrite `include/eepp/ui/css/propertyidset.hpp` around `std::bitset<512>`. Keep it header-only.
The public API is:

```cpp
void insert( PropertyId id );
void clear();
void erase( PropertyId id );
bool empty() const;
bool contains( PropertyId id ) const;
std::size_t size() const;
PropertyIdSet& operator|=( const PropertyIdSet& other );
PropertyIdSet operator|( const PropertyIdSet& other ) const;
PropertyIdSet& operator&=( const PropertyIdSet& other );
PropertyIdSet operator&( const PropertyIdSet& other ) const;
bool operator==( const PropertyIdSet& other ) const;
bool operator!=( const PropertyIdSet& other ) const;
PropertyIdSetIterator begin() const;
PropertyIdSetIterator end() const;
PropertyIdSetIterator erase( PropertyIdSetIterator it );
```

`Invalid` insertion/erasure is a no-op, `contains(Invalid)` is false, and out-of-capacity values
assert in debug while returning safely in release. The iterator stores the owning set and a
`std::size_t` bit index. Construction and increment scan forward to the next set bit; dereference
returns `PropertyId`. `erase(iterator)` clears the current bit and returns an iterator positioned at
the next set bit. There is no custom-ID side container and no heap fallback.

Add:

```cpp
static_assert( sizeof( PropertyIdSet ) == 64 );
```

on the supported standard libraries/builds. If visibility or padding makes this fail on a supported
toolchain, assert `sizeof(std::bitset<512>) == 64` and `sizeof(PropertyIdSet) <= 72` instead; this is
the only permitted toolchain adaptation.

Change all `PropertyIdSet` loops and calls to `PropertyId`. In particular, remove the
`static_cast<PropertyId>(prop)` conversions in `UIStyle::onStateChange()` and reject raw name hashes
at compile time.

## Implementation Stages

### Stage 1 - Make property application order-independent

Complete this stage and run its focused tests before changing any ID or set representation.

1. Add a protected virtual `UIWidget::onAttributesTransactionEnd()` with a default empty
   implementation.
2. In `UIWidget::endAttributesTransaction()`, assert that the transaction count is positive before
   decrementing it. When the count reaches zero, call `onAttributesTransactionEnd()` before
   emitting the accumulated self/parent layout notifications. Changes triggered by reconciliation
   are therefore folded into the same pending flags.
3. Override the hook in `UIHTMLWidget`. Call `updateCSSContentBoxFixedSize()` there so final width,
   height, padding/border, and `box-sizing` state is reconciled independent of setter order.
4. Override the hook in `UIHTMLImage`. Call `UIHTMLWidget::onAttributesTransactionEnd()` first and
   `autoSizeImage()` second so the final width/height policies, CSS content-box size, min/max
   equations, padding, and drawable ratio are all present before replaced sizing is finalized.
5. Keep existing immediate setter behavior outside transactions. Do not globally suppress
   `onSizeChange()` or `onSizePolicyChange()`; native/direct setters rely on it.
6. Add a test helper that applies the same image properties inside an attribute transaction in
   these orders: `width,height,max-width`; `max-width,height,width`; `height,width,max-width`; and
   reverse dense-ID order. Each case must end with identical non-zero size and the 1436/2560 aspect
   ratio under the same containing block.
7. Keep and run `UIHTML.ImageMaxWidthConstrainsWebpWithHeightAuto`. Add a second transition test
   that starts with fixed HTML dimensions, applies `height:auto;max-width:100%` through a style
   state change, then removes and reapplies the rule. Assert identical final geometry each time.

The acceptance criterion is explicit: randomizing or reversing changed-property traversal in a
test-only helper must not alter final widget geometry. Only after this passes may Stage 5 switch to
ascending bitset iteration.

### Stage 2 - Introduce dense enum declarations and maps

1. Add `propertyids.hpp`; move both enum declarations out of `propertydefinition.hpp` and
   `shorthanddefinition.hpp`; include the new header from both.
2. Reconcile enum members with the actual default registrations as specified above.
3. Add `idnamemap.hpp` and focused unit tests before integrating it.
4. Add dense members to both definition classes while retaining their canonical name hashes.
5. Replace the two hash-to-definition ownership maps in `PropertySpecification` with the two name
   maps and two dense definition vectors.
6. Convert string constructors/lookups in `StyleSheetProperty` to full-name lookup. Continue to
   compute/cache `mNameHash`; never use a hash alone to select a definition.
7. Convert attribute-selector lookup in `stylesheetselectorrule.cpp` and every remaining
   hash-only property/shorthand lookup to either a known dense ID or full string lookup.
8. Verify aliases resolve to the canonical definition and dense ID while preserving the alias text
   in the parsed `StyleSheetProperty` where current diagnostics/serialization expect it.

### Stage 3 - Explicitly register all built-ins

1. Convert all longhand calls in `stylesheetspecification.cpp` to explicit `PropertyId` overloads.
2. Convert all shorthand calls to explicit `ShorthandId` overloads.
3. Add the final registration validation.
4. Search the entire repository for `registerProperty` and `registerShorthand`. Keep external/tool
   registrations on the runtime overload and add null handling for capacity failure.
5. Search for casts between hashes, `Uint32`, `PropertyId`, and `ShorthandId`; eliminate every cast
   that treats one identity domain as another.

### Stage 4 - Migrate resolved-property APIs to dense IDs

Keep `StyleSheetProperties` name-hash keyed. Change APIs that semantically address a registered
property to accept `PropertyId`:

- `ElementDefinition::getProperty(PropertyId)`;
- `StyleSheetStyle::hasProperty(PropertyId)` and a new `getProperty(PropertyId)` helper;
- `UIStyle::getProperty`, `hasProperty`, `hasLocalProperty`, `getLocalProperty`, and
  `getResolvedLocalProperty` dense overloads;
- transition/animation definition maps whose keys represent registered properties;
- all widget dispatch and `getPropertyString` paths.

Implement a dense lookup into a hash-keyed `StyleSheetProperties` by retrieving the
`PropertyDefinition` from `PropertySpecification`, then finding `definition->getId()` in the hash
map. Do not add a 512-entry pointer array to every `ElementDefinition`; that would trade one small
lookup for several kilobytes per cached style definition.

Keep explicit name/hash APIs only where they address arbitrary names (`data-*`, CSS variables,
selectors, serialization, or animation action tags). Rename ambiguous helpers such as
`getPropertyById(Uint32)` to `getPropertyByNameHash(String::HashType)` when they truly remain
hash-based.

When `ElementDefinition::refresh()` builds its ID set, insert
`property.getPropertyDefinition()->getPropertyId()` only when a definition exists. Preserve the
current separate handling of unknown and `data-*` properties; `StyleSheetProperty::getId()` must
key each by its own `mNameHash`, while neither `PropertyIdSet` nor either `IdNameMap` receives it.

### Stage 5 - Replace `PropertyIdSet` and remove the render-time allocation workaround

1. Land the bitset implementation and its unit tests.
2. Convert `ElementDefinition`, `UIStyle`, inheritance propagation, and all other set users to
   typed `PropertyId` operations.
3. Change `PropertySpecification::mInheritableProperties` and its getter to `PropertyIdSet`.
4. Remove `UIStyle::mChangedProperties` and its `std::optional<PropertyIdSet>` cache. Restore
   `PropertyIdSet changedProperties;` as a stack local in `onStateChange()`; the 64-byte bitset has
   no constructor allocation and does not retain per-widget heap capacity.
5. Keep ascending numeric iteration. Do not add insertion-order storage, sorting, or a second
   vector.
6. Re-run the Stage 1 permutation tests with the real bitset traversal and the WebP regression.

### Stage 6 - Cleanup and documentation

1. Remove obsolete raw-`Uint32` overloads, hash casts, unordered-set iterator machinery, and
   comments describing hash values as property IDs.
2. Document in `propertyids.hpp` that enum numeric values are unstable internal values and names
   must be used for persistence/plugins crossing binary boundaries.
3. Document runtime capacity and registration-before-parsing requirements on both registration
   APIs.
4. Add a short comment at the transaction-end reconciliation hook explaining that the hook makes
   interdependent property setters observe the complete style and protects set-order independence.
5. Update Doxygen for `getId()`, `getPropertyId()`, `getShorthandId()`, and renamed name-hash APIs so
   the three identity domains cannot be confused.

## Required Tests

Add focused tests under `src/tests/unit_tests/` and register them with the existing unit-test build:

### `IdNameMap`

- zero is invalid and returns the empty name;
- explicit built-ins round-trip by ID and full string;
- aliases resolve to the target without consuming an ID;
- duplicate ID/name and conflicting aliases are rejected;
- the first custom ID is exactly `FirstCustomId`;
- repeated custom lookup returns the same ID;
- IDs increase contiguously;
- the final valid slot succeeds and the next registration returns `Invalid`;
- two different strings are never equated only because a hash collides.

### Definition registration

- every built-in property and shorthand slot is populated;
- canonical name and dense ID round-trip for every slot;
- all automatically generated dash/underscore-free property aliases resolve to the same ID;
- explicit aliases such as `bgcolor`, `align`, `layout_width`, `lw`, and `rotate` resolve correctly;
- a runtime property receives `FirstCustomId`, can be parsed/applied/dispatched, and appears in a
  `PropertyIdSet`;
- a runtime shorthand receives `ShorthandId::FirstCustomId`, expands to registered longhands, and
  never collides with an equal-valued `PropertyId`;
- duplicate and `-`-prefixed replacement behavior remains as specified;
- capacity failure is safe and observable.

### `PropertyIdSet`

- default empty state performs no heap allocation under the debug memory manager;
- insert, duplicate insert, contains, erase-by-ID, clear, size, equality, union, intersection, and
  self-union/intersection;
- `Invalid` behavior and upper boundary behavior;
- custom IDs work identically to built-ins;
- iteration yields only present IDs in ascending numeric order;
- iterator erase returns the next present ID and supports erasing every element in one loop;
- copy/move operations preserve bits and allocate nothing;
- exact/maximum object-size assertion described above.

### Ordering and integration

- all Stage 1 image property permutations produce identical final geometry;
- `UIHTML.ImageMaxWidthConstrainsWebpWithHeightAuto` passes;
- the state-transition variant passes;
- shorthand parsing for every built-in shorthand still produces the same longhand names/values;
- style specificity, indexed properties, aliases, inheritance, transitions, animations, CSS
  variables, custom registered properties, and `data-*` properties retain coverage;
- add a regression proving two definitions with the same shorthand/property spelling occupy their
  separate ID spaces and resolve through the correct map.

## Build and Verification Sequence

After each stage that changes C++:

1. From the repository root, regenerate using the current debug command from
   `.agent/rules/build-project.md`. Use `premake4` when installed, include `--with-mold-linker` when
   `mold` is installed, and otherwise use the documented fallback.
2. Format all changed C/C++ files with the repository `clang-format` command.
3. Build with `make -C make/linux -j$(nproc)` exactly.
4. Run focused tests through:

   `projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="<filter>"`

Before completion:

1. Run the complete unit-test suite through
   `projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug`.
2. Build the normal eepp targets and ecode, not only the unit-test target, because public CSS APIs
   and plugin-facing registration signatures change.
3. Run `git diff --check` and inspect the full diff for accidental generated/build artifacts.

## Performance and Allocation Acceptance Criteria

- Constructing, clearing, copying, unioning, intersecting, and iterating `PropertyIdSet` performs
  zero heap allocations.
- `UIStyle::onStateChange()` has no allocation attributable to the changed-property set, including
  its first call on a widget.
- No per-`ElementDefinition` 512-entry lookup table is introduced.
- Full-string name lookup and custom ID assignment occur during registration/parsing, not during
  property-set iteration or widget dispatch.
- Dense property lookup is vector indexing; set membership/algebra is fixed-size bitwise work.
- The transaction-end reconciliation adds no heap allocation. It runs only when the outermost
  transaction closes and must avoid duplicate layout messages by executing before pending flags
  are flushed.
- Perform the repository-required allocation audit over every touched constructor, container
  insertion, string copy, lambda, and transition/animation path.

## Compatibility and Failure Policy

- This is an ABI break for enum values and for APIs that previously accepted raw `Uint32` IDs.
  Treat it as an intentional eepp library ABI change; do not provide implicit integer overloads
  that reintroduce hash/dense ambiguity.
- Source code comparing `getId()` to name hashes continues to work. Source code casting
  `PropertyId`/`ShorthandId` to persisted integers must migrate to names.
- Existing external custom registrations continue through the name-only runtime overload but must
  handle its nullable return on capacity exhaustion.
- Registration must finish before styles using custom names are parsed. Registering a definition
  does not retroactively repair already parsed unknown properties.
- Capacity exhaustion is never allowed to wrap, reuse `Invalid`, overwrite another definition, or
  silently drop a built-in. Built-in mismatch is a debug assertion/programming error; runtime
  exhaustion is a logged recoverable failure.
- Hash collisions in existing name-keyed `StyleSheetProperties` remain a pre-existing concern
  outside this refactor. The new ID maps themselves must use full-string equality and must not add
  any new collision-based identity.

## Completion Checklist

- [ ] Stage 1 proves style application order-independent before bitset migration.
- [ ] `PropertyId` is dense `Uint16`, complete, and below 512.
- [ ] `ShorthandId` is dense `Uint8`, complete, and below 255.
- [ ] Separate property and shorthand `IdNameMap` instances own full-string reverse lookup.
- [ ] Every built-in registration supplies an explicit enum ID and passes round-trip validation.
- [ ] Runtime custom property and shorthand registration has deterministic IDs and exhaustion
      handling.
- [ ] Hash identity and dense identity have unambiguous API names/types.
- [ ] `PropertyIdSet` is a fixed 512-bit allocation-free set with typed ascending iteration.
- [ ] `UIStyle` no longer retains `mChangedProperties` merely to avoid set allocation.
- [ ] Image sizing, shorthand expansion, inheritance, animations/transitions, variables, aliases,
      data properties, and custom properties pass focused coverage.
- [ ] Debug eepp, unit tests, ecode, and the full wrapped suite build/pass using `-j$(nproc)`.
- [ ] Formatting, diff check, allocation audit, and final self-review are complete.
