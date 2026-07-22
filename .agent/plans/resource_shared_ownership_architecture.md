# eepp shared-resource ownership architecture

Status: active implementation baseline; Stage 0 through Stage 5 complete, 2026-07-21.

This document freezes the contracts that must be true before the public texture API is changed. The
implementation may refine names and small mechanics, but changing an invariant below requires an
explicit architecture revision.

## 1. Objective

Replace raw manager ownership, global load side effects, manual deletion, and `ownIt` flags with
explicit shared ownership throughout eepp and all in-repository consumers.

The final model is:

- Consumers, immutable source objects, catalogs, and caches own resources with strong handles.
- A live registry observes resources weakly for diagnostics, accounting, and leak
  reporting. It is never searched for semantic names.
- Catalogs define names and persistence.
- Scopes define which catalogs and typed caches are visible.
- GPU resources remain graphics-thread-affine. A final owning release may happen on a worker, but
  the texture deleter only performs a thread-safe handoff to TextureFactory. Actual destruction runs
  through the graphics/display lifecycle.
- UI drawable resolution is layered over Graphics resource lookup; browser caching and navigation
  remain outside Graphics.
- A UISceneNode can own a scope and resolver, but neither texture lifetime nor pure Graphics usage
  requires a UISceneNode.

This is an intentional repository-wide API break. There will be no compatibility API, no `Shared`
suffixes, and no retained `ownIt` overloads.

## 2. Confirmed hazards in the current code

These are not hypothetical migration risks:

- `TextureAtlasLoader` queues `TextureFactory::loadFromFile()` and `loadFromPack()` calls, discards
  their results, then later resolves the textures globally by name. Immediately switching the
  factory to weak, unpinned retention would compile and destroy each loaded texture at the end of
  the lambda.
- `TextureLoader` stores and returns `Texture*`; its callback and unload behavior depend on the
  factory owning the object.
- `TextureRegion` stores `Texture*`, and constructors taking a texture ID resolve that raw pointer
  through the factory.
- `FrameBuffer` stores `Texture*` and deletes it directly in its destructor.
- `TextureAtlas`, glyph drawables, SVG icon raster caches, GIF loading, and font caches retain raw
  texture pointers.
- `Texture::~Texture()` performs GL deletion and reaches `Engine::instance()` and
  `TextureFactory::instance()`. Shared ownership must replace this singleton-dependent destruction
  with TextureFactory-controlled deferred release on the graphics thread.
- `TextureRegion::isStateful()` and `DrawableGroup::isStateful()` return false despite draw/update
  methods mutating destination size, position, or child state.
- `UIImage`, `StateListDrawable`, and `DrawableGroup` temporarily mutate drawable color, alpha,
  size, position, or children while drawing.
- `Models::Variant` copies both an owning flag and the same raw drawable pointer, allowing two
  copies to believe they exclusively own one allocation.
- `EE_MEMORY_MANAGER` requires allocation registration through `eeNew` and removal through
  `eeDelete`; default `shared_ptr` deletion of an `eeNew` allocation would leave tracking invalid.
- `Engine::~Engine()` destroys `Renderer` before `ShaderProgramManager`, while
  `ShaderProgram::~ShaderProgram()` calls `GLi->deleteProgram()` directly.
- The global HTTP pool is cleared after Graphics resources and managers, allowing asynchronous
  producers to outlive systems they can mutate.

## 3. Frozen ownership vocabulary

### 3.1 Public handle representation

eepp will expose `std::shared_ptr` and `std::weak_ptr` through consistent aliases:

```cpp
template <typename T> using ResourcePtr = std::shared_ptr<T>;
template <typename T> using ResourceWeakPtr = std::weak_ptr<T>;

using TexturePtr = ResourcePtr<Texture>;
using TextureWeakPtr = ResourceWeakPtr<Texture>;
```

This is an explicit API/ABI choice. eepp accepts that users can use standard shared-pointer
operations. The library nevertheless exposes no API for adopting an arbitrary resource raw pointer,
and resource constructors remain protected/private where practical.

Requirements:

- A resource allocation has exactly one control block.
- Owning APIs return handles. They do not return a raw pointer plus an ownership convention.
- Long-lived borrowed raw pointers are forbidden unless the field or API documents the owner that
  dominates the borrow. Local `.get()` views inside a call are allowed.
- Shared-library and static-library builds must test the chosen handle/deleter behavior.

### 3.2 Centralized creation and deletion

All ref-counted eepp resource allocations go through one internal creation path with an
eepp-compatible deleter:

```cpp
template <typename T> struct ResourceDeleter {
	void operator()( T* resource ) const noexcept { eeDelete( resource ); }
};

template <typename T, typename... Args>
ResourcePtr<T> makeResource( Args&&... args );
```

Factories use an equivalent private helper for protected constructors. `std::make_shared` is not
used for tracked eepp resources unless the memory manager is redesigned to understand its combined
allocation. No second control block may be created from `handle.get()`.

Texture is the deliberate exception to immediate `eeDelete`: its factory-controlled deleter may
queue the final raw object from any thread. `TextureFactory::collectReleasedTextures()` performs the
eventual `eeDelete` on the graphics thread after queued rendering has been flushed. This is the same
deferred destruction contract used by scene nodes; it is not a general GPU disposal system.

### 3.3 Identity, keys, and labels

These concepts are distinct:

- `ResourceId` is immutable and process-unique across Engine recreation in tests. For textures it
  is the value returned by `Texture::getTextureId()`; there is no separate factory-internal texture
  ID.
- `ResourceKey` is the immutable canonical semantic lookup key. Equality compares the complete key,
  never only a hash.
- `displayName` is diagnostic text and may change without changing identity or catalog indexes.
- Aliases are catalog entries, not mutable fields used as registry indexes.

A process-wide monotonic ID source must not reset when an Engine singleton is recreated by tests.

## 4. Graphics ownership layers

```text
Engine
├── Renderer and contexts
├── TextureFactory
│   ├── weak live-texture registry
│   └── deferred released-texture queue
├── GlobalResourceCatalog (intentional strong persistence)
└── Default ResourceScope (local catalog + explicit imports)

Application/scene ResourceScope
├── local ResourceCatalog
├── explicitly imported catalogs
└── typed caches

UI::DrawableResolver
├── CSS/image/icon/glyph parsing
├── node and UISceneNode context
└── delegates texture/source lookup to Graphics::ResourceScope

UI/Network::WebResourceCache
├── request/cache partitioning
├── in-flight request coalescing
├── TTL/LRU/byte-budget retention
└── per-document leases and subscribers
```

Engine coordinates the Graphics lifetime roots directly. TextureFactory coordinates texture
creation, weak observation, and deferred destruction, but does not provide semantic lookup or
normal strong retention. No Graphics class depends on UI.

### 4.1 LiveResourceRegistry

The registry observes every instantiated texture weakly:

```cpp
struct TextureRecord {
	ResourceId id;
	ResourceKey creationKey;
	std::string displayName;
	TextureWeakPtr texture;
	std::size_t memoryBytes;
	ResourceFlags flags;
};
```

It has no strong resource field, no `ownerScope`, no semantic name resolution, and no public
`unregister()` operation.

Primary operations are snapshots, expiration purging, and live iteration. Diagnostic snapshots
return metadata plus weak handles, not a vector of owning texture handles:

```cpp
TextureRegistrySnapshot snapshotTextures() const;
void purgeExpired();
```

Opening `UITextureViewer` must not retain all textures. The viewer may lock a weak handle for one
render operation and may strongly retain only a user-selected texture.

Destruction, callbacks, and GL operations never occur while a registry lock is held.

Snapshot metadata, including texture memory usage, is copied from the texture while the registry
temporarily locks its weak handle. Texture keeps its existing memory size as the single source of
truth; accounting does not require separate shared state or callbacks into the factory.

### 4.2 ResourceCatalog

A catalog maps complete canonical keys/aliases to strong handles. It provides semantic lookup and
intentional persistence:

```cpp
class ResourceCatalog {
  public:
	void publish( ResourceKey key, TexturePtr texture );
	TexturePtr findTexture( const ResourceKey& key ) const;
	bool erase( const ResourceKey& key );
	void clear();
};
```

Publishing is the final global pin model. The low-level TextureFactory has no `ResourcePin` argument
and no factory-owned strong pin. Independent catalogs/caches naturally provide independent pins.
If temporary explicit pinning is needed, `ResourceCatalog::pin()` returns an independent RAII token;
there is no shared boolean or one global `strong` field.

A higher-level scope convenience may accept a retention option and publish into that scope's catalog,
but texture creation and decoding remain unpinned operations.

### 4.3 ResourceScope

`Graphics::ResourceScope` performs Graphics-only lookup and loading:

```cpp
class ResourceScope {
  public:
	TexturePtr findTexture( const ResourceKey& key ) const;
	TexturePtr loadTexture( const TextureRequest& request );
	void publishLocal( ResourceKey key, TexturePtr texture );
	void importCatalog( ResourceCatalogPtr catalog );
};
```

Frozen lookup rules:

- Search the local catalog, then explicitly imported catalogs in deterministic order.
- Never search the live registry.
- Never implicitly search a parent, host scene, sibling scene, or every live resource.
- The default Graphics scope imports the global catalog explicitly.
- A UI/application scene receives only the catalogs deliberately imported into it.
- A Web document does not inherit host/global resources unless the host exports and imports them
  intentionally.
- Scopes import catalogs, not arbitrary scopes. This avoids recursive lookup and import cycles.

Pure `EE::Graphics` users may use TextureFactory for unpinned creation or Engine's default Graphics
scope/catalog for named persistent resources. No UISceneNode is involved.

### 4.4 TextureFactory final API role

TextureFactory creates, decodes, uploads, and updates textures. Creation names return `TexturePtr`
directly and do not retain it:

```cpp
TexturePtr createEmptyTexture( ... );
TexturePtr loadFromPixels( ... );
TexturePtr loadFromPack( ... );
TexturePtr loadFromMemory( ... );
TexturePtr loadFromStream( ... );
TexturePtr loadFromFile( ... );
```

The factory registers every result with its weak live registry. It has no semantic `getByName()` or
`getByHash()` API, no public deletion/removal API, no public registry detachment, and no owning
`getTextures()` API. Named lookup belongs to a catalog/scope; diagnostics use registry snapshots.

The `TexturePtr` control block uses a factory-controlled deleter. Final release appends the raw
texture to TextureFactory's released queue; it does not execute `Texture::~Texture()` immediately.
`Window::display()` flushes pending batches and then calls
`TextureFactory::collectReleasedTextures()` while the active context is current. Engine shutdown
performs the same collection explicitly because no later display is guaranteed.

## 5. GPU lifetime and threading

### 5.1 Graphics-thread lifetime contract

GPU operations remain graphics-context-affine and follow the existing graphics/update or explicitly
shared-context rules. Releasing the final `TexturePtr` is different: it performs no GPU operation and
may enqueue the raw texture from any thread. Only collection and actual destruction require the
graphics thread and a current context.

Debug builds assert the collection boundary. The design does not add a generic device state, epoch,
or disposal mechanism for other GPU resource families; TextureFactory's small deferred-release queue
is the texture-specific lifetime boundary already required by batched rendering.

### 5.2 Texture deferred destruction

Final `TexturePtr` release queues the Texture object in TextureFactory. It remains allocated until a
safe collection point:

```cpp
void Window::display( bool clear ) {
	GlobalBatchRenderer::instance()->draw();
	TextureFactory::instance()->collectReleasedTextures();
	swapBuffers();
	// ...
}
```

Batch flushing precedes collection because the current renderer still keeps borrowed texture state.
Long-lived render queues must eventually retain TexturePtr themselves. The released queue is also
drained during Engine shutdown after consumers are released and before TextureFactory, Renderer or
contexts are destroyed.

Other self-contained GPU classes retain their direct, graphics-thread destruction model. They are
not routed through TextureFactory and do not acquire generic lifetime machinery unless a later
ownership migration demonstrates a concrete need.

### 5.3 Resource mutation and loading threads

- Decode and network work may run off-thread.
- GPU create/upload/reload/mutation and final ownership release obey the graphics-thread/shared-
  context contract.
- Registries and caches are thread-safe at their boundaries.
- UI mutation executes on the scene/main thread and remains protected by scene generation tokens.
- A generation token controls whether a subscriber may mutate a scene; it does not own a resource or
  a shared request.
- No callback, resource destruction, or device command executes while a registry/cache mutex is held.

## 6. Engine shutdown and restart contract

Engine teardown must be reordered around producer shutdown, consumer release and valid GL contexts:

1. Mark Engine and Web cache/resource delivery services as shutting down; reject new work.
2. Invalidate scene/document async subscribers and stop accepting main-thread resource deliveries.
3. Cancel/stop and join resource/network/decode producers that can create resources or callbacks.
4. Destroy scenes, documents, UI resolvers, document leases, scene scopes, and application caches.
5. Clear application/global catalogs and remaining manager-owned handles.
6. Flush/discard pending rendering submissions, then collect TextureFactory's released textures.
7. Inspect the weak texture registry. Debug/tests assert that no unexpected strong TexturePtr
   remains; a defensive shutdown sweep may release the GPU payload of reported survivors.
8. Destroy device-dependent managers in audited order while Renderer and contexts remain valid.
9. Destroy TextureFactory, Renderer, windows/contexts and backend state in that order.

The exact manager list will be produced by the Stage 1 GPU audit, but these order constraints are
fixed:

- HTTP/decode producers stop before resource consumers and GPU systems are dismantled.
- `ShaderProgramManager` releases programs before Renderer/GL dispatch is destroyed.
- TextureFactory's deferred released queue is empty before its context disappears.
- A TexturePtr surviving Engine destruction is a project-contract violation, reported by debug
  builds and tests rather than supported through a second device-lifetime architecture.
- Destructors cannot recreate singletons.
- Tests release all resource handles before recreating Engine state from zero.

## 7. Drawable model

Shared lifetime and shareable instance state are separate concerns. `isStateful()` is not a sharing
contract and will not be used as one.

### 7.1 Source/instance split

Resource resolution caches immutable source data. UI consumers own per-consumer drawable instances:

```cpp
using DrawablePtr = ResourcePtr<Drawable>;

DrawablePtr Drawable::clone() const;
DrawablePtr DrawableResolver::createDrawable( const DrawableRequest& request );
```

Stage 4 established this contract without introducing a parallel `DrawableSource` class hierarchy.
Existing drawable resource types serve as source prototypes while retained by an atlas, theme,
icon, catalog, or resolver. A prototype is never handed directly to an unrelated consumer:
`clone()` returns independently mutable presentation state while sharing underlying
texture/resource handles. This is simpler than duplicating every drawable type into source and
instance classes and remains compatible with introducing immutable source-only types later when a
concrete resource requires one.

eepp continues to use `Drawable::Type` for runtime drawable dispatch. Generic handle conversion
checks that tag and then uses `static_pointer_cast`; cloning code for a statically known concrete
type also uses `static_pointer_cast`. The ownership migration does not introduce RTTI casts.

Representative split:

- `Texture` is shared GPU/resource data, not a globally shared mutable drawable instance.
- `TextureRegion` prototypes and instances retain a TexturePtr; instances copy rectangle, offset,
  intrinsic size, destination size, tint, and position.
- `NinePatch` instances clone their nine mutable region children while sharing the textures.
- `TextureDrawable` holds per-consumer destination size, tint, alpha, and position while retaining
  the shared TexturePtr.
- `StateListDrawable`, `DrawableGroup`, and `Sprite` are per-consumer state machines/instances that
  refer to source handles or private child instances.

`DrawableImageParser::createDrawable()` always returns a fresh consumer instance for CSS-generated
or resolved content, even when its immutable source came from a cache.

`UIIcon`, `UIGlyphIcon`, and `UISVGIcon` expose the split directly:

```cpp
const DrawablePtr& UIIcon::getSource( int size ) const;
DrawablePtr UIIcon::createDrawable( int size ) const;
```

`getSource()` supports lookup, measurement, and immediate rendering without cloning an existing
prototype. Glyph and SVG icons may materialize and cache a missing size source once.
`createDrawable()` is the explicit consumer-instance boundary for callers that retain the drawable
or need persistent independent state.

Immediate, single-threaded render paths may borrow an icon source and temporarily change
presentation state when they restore every changed value before returning and never retain the raw
pointer. Retained widget, menu, model, animated, or otherwise independently stateful consumers must
create and own an instance. Shared child mutation remains forbidden where drawing can be reentrant
or where the complete state cannot be restored locally.

No rendering callback may call `clone()`, `UIIcon::createDrawable()`, or an API that performs either
operation internally. It must render either a previously retained instance or a borrowed source
under the temporary-state contract above. The Stage 4 call-site audit classifies all remaining
direct `clone()` calls as:

- implementations recursively cloning their private child state;
- constructors and setters adopting a private region/sprite/map instance;
- theme, skin, icon, CSS, and name-resolution source-to-instance boundaries;
- widget deserialization and one-time assignment; or
- focused ownership tests.

The code editor lock icon and ecode debugger, linter, LSP breadcrumb, and autocomplete icon paths
borrow their per-size icon sources at the point of immediate rendering and restore temporary color
changes before returning. Icons assigned to widgets, menus, or models still use owned instances.

### 7.2 Consumer API

Consumers store a strong per-consumer instance:

```cpp
UIImage* UIImage::setDrawable( DrawablePtr drawable );
DrawablePtr UIImage::getDrawable() const;
```

The same rule applies to CSS layers, buttons, skins, icon instances, state lists, and groups. All
`ownIt` flags and manual drawable deletion paths are removed in the same drawable migration stage.

`Models::Variant` will be structurally migrated to `std::variant` (or an equivalently safe
non-union representation) with `DrawablePtr` as a normal non-trivial member. Its old owning flag and
raw drawable alternative are removed.

### 7.3 Resource notifications

`DrawableResource::Unload` is removed as a consumer lifetime mechanism. A strong owner cannot be
notified that its object vanished, and destructor callbacks into a partially destroyed most-derived
object are unsafe.

Mutable/reloadable source data uses a typed change/invalidation signal with RAII connection tokens.
Callbacks capture weak lifetime tokens rather than raw consumer `this` pointers. Registry expiration
is observed through weak handles and snapshots, not an object destructor callback.

## 8. UI resolution layering

`DrawableSearcher` is replaced, but not by putting all of its behavior into Graphics::ResourceScope.

`UI::DrawableResolver` owns UI-specific interpretation:

- CSS gradients and functions
- `url(...)` and scene-relative URI resolution
- icons, glyphs, sprites, and generated drawable instances
- node/scene context and UI source-to-instance creation

It delegates texture/source lookup and creation to the scene's `Graphics::ResourceScope`. A default
UI resolver can use the default Graphics scope for UI applications without custom scenes, but pure
Graphics does not depend on it.

Browser request policy, cookies, navigation, and cache leases are not responsibilities of either
DrawableResolver or Graphics::ResourceScope.

## 9. Web document ownership and shared cache

### 9.1 Topology

Each UIWebView document has a distinct `DocumentSessionId`, document scope, and cache lease. Multiple
documents may use one shared WebResourceCache:

```text
Document scope/session A ─┐
                          ├── shared WebResourceCache partition
Document scope/session B ─┘
```

Navigation changes/releases only that document's lease. It never directly purges entries required
by another document. Widgets retain their currently displayed resource/source through ordinary
strong handles independently of cache retention.

### 9.2 Cache key and isolation

```cpp
struct OriginKey {
	std::string scheme;
	std::string normalizedHost;
	Uint16 effectivePort;
};

struct WebResourceKey {
	CachePartitionId partition;
	CanonicalURI uri;
	ResourceKind kind;
	DecodeOptions decode;
	RequestVariant requestVariant;
};
```

`CachePartitionId` represents the intentionally shared HTTP/cookie/authentication context. Different
cookie jars or credential contexts do not share entries merely because a URI matches. Request
variants account for content-affecting headers until full HTTP `Vary` support exists.

Canonicalization rules are fixed at the cache boundary:

- Same-origin means normalized scheme, host, and effective port all match.
- URI fragments are removed; query strings remain part of the key.
- Redirect metadata records both request URI and canonical final URI without merging partitions.
- File paths use platform-aware canonicalization with explicit symlink/case behavior.
- Data URIs use a content hash plus decode options and enforce resource-size limits.
- Hashes accelerate lookup but full keys determine equality.

### 9.3 Entry state and request coalescing

```cpp
enum class LoadState { Empty, Loading, Ready, Failed, Cancelled };

struct WebCacheEntry {
	WebResourceKey key;
	LoadState state;
	TexturePtr retainedResource;
	TextureWeakPtr liveResource;
	MonotonicTime lastUsed;
	MonotonicTime expiresAt;
	std::size_t retainedBytes;
	UnorderedSet<DocumentSessionId> activeLeases;
	std::vector<WeakSubscriber> subscribers;
};
```

Concurrent requests for one key share one fetch/decode/upload operation. Each subscriber has its own
document session and scene generation. A stale subscriber is removed without cancelling delivery to
other current subscribers. Cancellation of the shared operation occurs only when policy permits and
no subscriber/cache requirement remains.

Retention uses monotonic TTL, LRU information, and per-partition/global byte budgets. Same-origin
navigation may renew a document lease; cross-origin navigation releases that document's old-origin
lease. External consumer handles remain valid regardless of cache eviction.

Failure entries define retry/backoff and do not become permanent accidental cache hits.

## 10. Migration strategy

No externally released intermediate state is required. Temporary duplicated retention is allowed on
the feature branch to keep behavior valid while the repository-wide API break is assembled.

### Stage 0: contract freeze and inventories

Status: complete. Its accepted conclusions are incorporated into this architecture baseline.

Deliverables:

- This architecture document accepted or amended.
- Complete inventory of GPU resource classes and direct GL deletion sites.
- Complete inventory of stored raw `Texture*`, texture-ID lookup, ignored texture-load returns,
  loader callback signatures, and factory deletion calls.
- Complete drawable mutation/shareability inventory.
- Build-matrix decision for shared/static libraries and `EE_MEMORY_MANAGER`.
- Concrete shutdown dependency graph for Engine-owned producers, consumers, managers, Renderer, and
  contexts.

Exit criterion: no unresolved ownership, lookup, last-release-thread, Engine restart, scope import,
or drawable-sharing contract blocks substrate implementation.

### Stage 0.5: prerequisite bug fixes

Status: complete, 2026-07-14. Concrete defects discovered by the ownership audit were fixed with
focused regression coverage while preserving current raw factory ownership:

- HTTP Pool clears clients outside its mutex so joined callbacks can re-enter without deadlock.
- Externally executed HTTP tasks cannot retain a dangling raw Http after Pool destruction.
- TextureAtlasLoader joins/stops ResourceLoader work before callback-visible loader state is
  destroyed.
- Engine destroys ShaderProgramManager before Renderer and clears TextLayout before FontManager.
- Engine stops asynchronous resource producers before resource consumers and GPU managers.
- UISceneNode's static async delivery queue has an explicit shutdown purge/rejection boundary.
- Obsolete FrameBuffer context-loss reload APIs were removed.

TextureLoader's static callback registry is deliberately removed with Stage 1 live observation.
Drawable ownership defects remain assigned to their structural Stage 4 replacement.

### Stage 1: texture lifetime scaffolding, with old factory retention still active

Status: complete, 2026-07-15. Stable process-wide ResourceId, eepp-compatible resource
aliases/deleter, and TextureFactory's weak live-texture registry are
implemented.
`Texture::getTextureId()` now returns that ResourceId directly, and every identity-based texture
API and stored consumer uses ResourceId; the only other texture identifier is the OpenGL handle.
The factory now retains the single TexturePtr control block internally while public texture APIs
still return raw pointers, preserving its old strong-retention behavior until Stage 2. Texture
memory updates no longer reach the factory singleton, and texture destruction no longer unregisters
itself through a singleton callback. Final handle release now queues Texture destruction in the
factory; `Window::display()` collects only after batch flush, and Engine shutdown performs a final
collection while its context remains valid. Debug assertions enforce graphics-thread release and
collection, while shutdown diagnostics report and defensively release GPU payloads from surviving
external handles. TextureLoader's static callback registry is removed. UITextureViewer reconciles
weak snapshots only when the atomic live-registry generation changes and strongly retains only the
currently enlarged texture.

Implement stable ResourceId, centralized eepp-compatible handle creation, the weak TextureFactory
live registry, TextureFactory's deferred released-texture queue, shutdown diagnostics,
and graphics-thread assertions. Integrate collection into Window::display() after batch flush and
into Engine shutdown before Renderer/context destruction. Reorder Engine teardown using the audited
dependency graph. Do not generalize this substrate to self-contained GPU resource classes.

Remove TextureLoader's static callback registry in the same change and migrate UITextureViewer to
the weak live registry. Do not add an intermediate synchronization/reset contract to the old API.

The old raw factory ownership remains temporarily so this internal stage cannot make resources
disappear. Public texture APIs have not switched yet; the deferred shared-pointer deleter becomes
active in the complete Stage 2 TexturePtr cut.

Exit tests:

- Released textures are deleted only at display/final shutdown collection points.
- Pending batches flush before texture collection.
- Engine teardown leaves no pending released textures or unexpected live registry entries.
- Repeated test-only Engine create/destroy cycles start with empty resource state.
- Worker-thread final release only queues the texture; it does not run GL or destruction work.
- `EE_MEMORY_MANAGER` accurately removes texture allocations through the factory-controlled deleter.

### Stage 2: one complete TexturePtr ownership cut

Status: complete, 2026-07-19. TextureFactory creation and acquisition APIs now return TexturePtr,
and TextureLoader exposes handle-based state with `reset()` replacing destructive `unload()`
semantics. TextureRegion, atlases/loaders, framebuffers, font pages and glyphs, nine-patches,
sprites, particle systems, SVG caches, UI image/background paths, maps, tools, tests, ecode, and
eeiv now retain texture handles. Atlas worker loads store their returned handles directly instead
of depending on later global lookup. BatchRenderer retains handle-aware submissions until flush;
its raw overload is limited to Texture's immediate draw path, whose queued object lifetime is
protected by display-time deferred destruction. Sprite's obsolete texture-owner flag and public
factory texture-removal APIs are removed. Factory-wide strong retention remains only as the
planned temporary bridge to Stage 3.

Change creation/acquisition APIs to return TexturePtr and migrate every required holder in the same
repository-wide cut. During conversion, TextureFactory temporarily retains strong handles so an
unclassified ignored result cannot silently expire.

At minimum migrate:

- TextureLoader state, return types, callbacks, unload behavior, and async captures
- Texture GIF frame ownership
- TextureRegion and region-source texture ownership
- TextureAtlas, TextureAtlasLoader, and texture vectors
- FrameBuffer attachment ownership
- fonts and glyph texture caches
- nine-patches and region chains
- SVG raster caches and UI icons
- sprites and particle systems that retain textures/regions
- UIImage and UINodeDrawable texture paths
- debug texture viewer and live-resource diagnostics
- eepp, ecode, modules, examples, tools, and tests

Every stored raw Texture pointer is classified as strong, weak, or a short borrow dominated by a
documented owner. Add a source audit/clang-tidy check where practical.

Exit criteria:

- Regions, atlases, framebuffers, fonts, glyphs, UI consumers, and async work retain dependencies.
- No ignored factory load result is relied upon for later global lookup.
- No public texture delete/remove API remains.
- Diagnostic snapshots do not pin resources.
- Temporary factory retention can be removed without failing ownership tests.

### Stage 3: catalog and scope ownership cutover

Status: complete, 2026-07-19. Engine now owns the global catalog and default Graphics scope;
UISceneNode owns an isolated scope that can be shared explicitly. TextureFactory is an unpinned
creator and weak live registry with no semantic name/hash lookup. Atlas, map, UI image/background,
DrawableSearcher, ecode, tests, and other name-based consumers publish to and resolve through their
explicit scope. Catalog aliases and imports provide intentional persistence and deterministic
sharing. Worker-thread final TexturePtr release is handed to the factory's thread-safe queue and
actual deletion remains display/shutdown-bound. The full cut also corrected TextureLoader's decoder
pixel allocator provenance, which asynchronous scoped loading exposed.

Implement the global catalog, default Graphics scope, application/scene catalogs, explicit imports,
immutable keys, and aliases. Move intended persistent resources from temporary factory retention into
catalogs/caches. Remove factory-wide strong retention and activate final unpinned creation.

Remove semantic TextureFactory name/hash lookup and migrate every lookup to a scope/catalog.

Exit criteria:

- Dropping the last real consumer/catalog/cache handle expires a texture.
- Duplicate names in unrelated scopes resolve independently.
- Sibling/document resources are invisible without explicit catalog import.
- Global resources persist only because the global catalog owns them.
- Live diagnostic records cannot be resolved semantically.
- Scope destruction does not invalidate externally retained resources.

### Stage 4: drawable source/instance conversion

Status: complete, 2026-07-20. Drawable ownership
now uses `DrawablePtr`; textures create private
`TextureDrawable` wrappers; mutable prototypes implement `clone()`; sprites, state lists,
skins, groups, nine-patches, regions, glyphs, gradients, and primitive drawables clone their
presentation state. UIImage, UINodeDrawable, menus/icons/themes, parsers, editor/tool consumers,
maps, physics, ecode, and eeiv were migrated in the same API cut.

`DrawableResource::Unload` and callback IDs were replaced by Change-only RAII connections.
Callback state is allocated lazily on the first connection, so ordinary drawable resources carry
no callback allocation. Callback storage and notification snapshots use small inline buffers;
snapshotting preserves safe self-disconnection and reentrant mutation during notification without
allocating in the common case.
`Variant` stores DrawablePtr outside its scalar union. UITextureRegion and ScrollParallax render
with local geometry rather than temporarily resizing shared source regions; region-based map
objects retain private instances. `DrawableSearcher` already returns fresh instances as a safe
bridge, but its replacement by the layered UI resolver remains Stage 5.

`UIIcon::getSource()` now returns a cached source/prototype for lookup, measurement, and immediate
single-threaded rendering under the temporary-state restoration contract, while
`UIIcon::createDrawable()` explicitly creates one private consumer instance. `UIGlyphIcon` and
`UISVGIcon` cache their lazily materialized sources under the same contract. The complete `clone()`
call-site audit found no remaining render-loop cloning. The code editor, debugger, linter, LSP
breadcrumb, and autocomplete draw-only paths borrow sources directly; retained widget and menu
icons continue to own instances.

Introduce source types and per-consumer instances, remove shared draw-state mutation, replace manual
ownership with DrawablePtr, remove Unload lifetime callbacks, add RAII change connections, and
migrate Variant's storage.

Convert UIImage, UINodeDrawable layers, StateListDrawable, DrawableGroup, Sprite, UIPushButton,
UISkin, UIIcon, themes, and DrawableImageParser in one coherent cut.

Exit criteria:

- Two consumers using one source have independent tint, alpha, size, position, state, and animation.
- Nested/reentrant drawing leaves no shared source or child modified.
- Drawable groups do not reposition shared child instances.
- Variant copying cannot duplicate exclusive ownership.
- No drawable `ownIt` or manual child deletion remains.

### Stage 5: layered UI resolution

Status: complete, 2026-07-21. `DrawableSearcher` was removed from Graphics. `ResourceScope` now
provides scoped drawable lookup for textures, atlas regions, nine-patches, and sprites, preserving a
pure Graphics entry point with no UI dependency. Each `UISceneNode` owns an allocation-free
`UI::DrawableResolver` that reads the scene's current scope and referer when resolving file, data,
HTTP, and named drawable references. UI images, sprites, menus, CSS parsing, and tests use the scene
resolver; callers without a scene explicitly construct one over `defaultResourceScope()`.

CSS icon resolution now uses the requesting node's scene instead of the process-global scene.
Texture names remain isolated by local/imported catalogs and become visible across scenes only when
their scopes or catalogs are shared intentionally. Atlas, nine-patch, and sprite manager migration
to scoped catalogs remains Stage 7 work for those resource families.

Implement UI::DrawableResolver and replace DrawableSearcher. Scene resolvers delegate Graphics work
to their explicit scope. Keep CSS/icon/glyph interpretation in UI and cookie/navigation concerns in
Web services.

Exit criteria:

- Pure Graphics works without UI.
- UI resolves through its scene/application scope.
- Embedded documents cannot see sibling resources accidentally.
- Host/application assets require explicit export/import.

### Stage 6: WebResourceCache and document leases

Implement cache partitions, canonical keys/origins, per-document sessions and leases, in-flight
coalescing, per-subscriber generation guards, retries, TTL/LRU, and byte budgets. Integrate WebView
navigation at its existing document replacement boundary.

Exit criteria:

- Shared tabs reuse eligible resources without sharing document ownership.
- Navigation in one tab cannot evict another tab's active lease.
- Different cookie/auth partitions cannot reuse credential-dependent responses.
- Stale subscribers do not block current subscribers.
- Same-origin retention and cross-origin lease release follow policy.
- Cache memory remains within configured budgets.

### Stage 7: remaining resource families

Migrate fonts, font faces/fallback caches, themes, shader programs/shaders, nine-patch catalogs,
atlas managers, and every remaining raw-owning ResourceManager subclass one family at a time. Their
self-contained GPU objects retain the established graphics-thread destruction contract unless a
concrete migration requires otherwise.

Remove raw-owning `ResourceManager<T>` only when no subclass or consumer depends on it.

## 11. Required validation matrix

### Ownership and registry

- Unpinned texture expires after its last real owner releases it.
- Region/source retains its texture; atlas retains its sources/textures; framebuffer retains its
  attachment.
- Catalog publication retains; erasing one catalog entry does not invalidate other owners.
- Independent catalog/pin leases do not interfere.
- Registry snapshot and UITextureViewer do not retain all resources.
- Registry creation, snapshot, expiration, and purging are race-safe.

### GPU/thread lifetime

- Final texture release on the graphics thread queues rather than immediately deleting.
- Worker-thread final release safely queues; display performs the destruction with a current context.
- Display flushes batches before collecting released textures under the current context.
- Engine shutdown performs a final collection before TextureFactory/Renderer/context destruction.
- No deletion/callback occurs while registry/cache locks are held.
- Repeated test-only Engine creation starts with no prior texture handles or registry state.
- Relevant suites run under TSAN as well as ASAN/LSAN.

### Scope isolation

- Identical keys in sibling scopes resolve independently.
- Explicit imported catalog resolves; missing import does not fall back.
- Global catalog is visible only to scopes importing it.
- Registry records are never semantically resolvable.
- Scope/catalog destruction leaves externally retained resources alive.
- Catalog import order is deterministic and graph cycles are structurally impossible.

### Drawable independence

- One source displayed by two widgets with different tint, alpha, and size.
- Two region instances draw at different sizes without shared mutation.
- State-list and sprite instances maintain independent state/time.
- Nested/reentrant drawing restores nothing because shared objects were not mutated.
- DrawableGroup owns/private-instantiates mutable children.
- Variant copy/move/reset is safe with DrawablePtr.

### Web cache

- Same-origin navigation reuses resources.
- Cross-origin navigation releases only the navigating document lease.
- Two tabs share eligible entries and survive independent navigation.
- Different cookie/auth partitions do not share.
- URI plus different decode/request options produces distinct entries.
- Concurrent requests coalesce while subscribers retain independent generation guards.
- Failed requests retry according to explicit policy.
- TTL uses a monotonic clock; byte-budget eviction works independently.

### Teardown and repeatable tests

- Pending HTTP, decode, and upload work during scene and Engine destruction.
- No destructor recreates Engine, Renderer, TextureFactory, or another manager.
- Repeated Engine create/destroy cycles in one process.
- Each unit test releases its scopes, catalogs, leases, and resources independently.
- Test teardown reports unintended catalog pins and live resource provenance.
- `EE_MEMORY_MANAGER`, supported debug/release configurations, static builds, and shared-library
  builds.

## 12. Next implementation deliverable

Stage 2 is complete with the repository-wide TexturePtr API and holder migration while temporary
factory retention preserves existing global lookup behavior. The next coding deliverable is Stage
3: introduce catalogs and Graphics scopes, migrate semantic name lookup out of TextureFactory, move
intentional persistence into catalogs/caches, and then remove the factory's temporary strong
retention.
