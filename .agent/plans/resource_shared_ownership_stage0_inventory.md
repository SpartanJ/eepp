# Shared-resource refactor Stage 0 inventory

Status: complete repository audit, lifetime contract revised 2026-07-12.

This document is the evidence and dependency inventory required by Stage 0 of
`resource_shared_ownership_architecture.md`. Searches covered `include/`, `src/eepp/`, in-tree
modules, tools, examples, ecode, and tests. Third-party implementation directories were excluded
except where eepp invokes their APIs.

Confirmed defects are tracked for implementation in
`resource_refactor_prerequisite_bugfixes.md`.

The inventory classifies stored relationships, side-effect loads, explicit deletion, callbacks,
GPU object namespaces, drawable mutation, asynchronous producers, and Engine teardown dependencies.
Line numbers will drift; paths and symbols are the stable references.

## 1. Stage 0 conclusions

The architecture contracts can proceed with these refinements:

1. eepp retains its existing graphics-thread-affine project contract. Shared ownership does not
   promise arbitrary-thread final destruction. Texture is deferred through TextureFactory and
   collected from `Window::display()` after batch flush; other self-contained GPU objects retain
   their direct graphics-thread destruction model.
2. Every queued renderer submission owns its dependencies. `BatchRenderer` currently stores a raw
   texture between `setTexture()` and a later `flush()`; the migrated batch must retain a
   `TexturePtr` until flushing or discarding its vertices.
3. Text layout is an owner/cache boundary. The global `TextLayout` LRU stores layouts containing
   `ShapedGlyph::font` raw pointers. In the final design layouts retain the required `FontPtr`
   values; the bounded global LRU is then an intentional cache owner and is explicitly clearable for
   test isolation.
4. Resource-producing work must have an explicit cancellation/lifetime owner independently of an
   arbitrary shared `ThreadPool`. UISceneNode pools can be shared with a host or application and
   cannot simply be destroyed by Engine. WebResourceCache and the owning scene/document services
   track their own operations and subscribers; no generic Graphics ResourceSystem is required.
5. Existing shutdown functions cannot only be reordered:
   - `Http::Pool::clear()` clears clients while holding the pool mutex; each client joins request
     callbacks, so a callback re-entering the global pool can deadlock.
   - When `Http::setThreadPool()` is active, queued lambdas capture raw `Http*`. `Http::~Http()`
     cancels requests but only joins privately created request threads; it does not join shared-pool
     work. Clearing the Pool can therefore destroy Http while a shared-pool task still uses it.
   - `TextureAtlasLoader` declares `ResourceLoader mRL` before the state used by its callbacks.
     Members are destroyed in reverse declaration order, so the loader is destroyed last and can
     run against already-destroyed state.
   These are prerequisite bugs and should be fixed independently before the ownership refactor.

   Status: the HTTP Pool lock-order defect and shared-ThreadPool operation lifetime defect are
   fixed. Pool clearing now establishes an operation barrier without owning the executor, including
   callback-initiated clearing and executor-discarded queued work. The TextureAtlasLoader lifetime
   defect is also fixed and covered by sanitizer-backed regression tests.
6. `isStateful()` is unusable as a shareability test. Every current Drawable inherits mutable color
   and position, and several classes reporting false mutate themselves or children during draw.
7. Stage 2 must migrate texture holders, loaders, ID-based construction, and queued batches in one
   cut. Factory-wide temporary retention may then be removed only in Stage 3 after catalogs/scopes
   replace global semantic lookup.

No unresolved architectural choice remains in Stage 0. The confirmed defects are tracked as Stage
0.5 bug fixes before texture lifetime scaffolding begins.

## 2. GPU and device-affinity inventory

### 2.1 Context topology

`Engine` owns a map of `Window*` and tracks one current window. SDL2 and SDL3 windows each own a
primary GL context and may own a second worker context when `SharedGLContext` is enabled:

- `src/eepp/window/engine.cpp`: `createWindow()`, `setCurrentWindow()`, `mWindows`.
- `src/eepp/window/backend/SDL2/windowsdl2.cpp`: `mGLContext`, `mGLContextThread`,
  `setGLContextThread()`.
- `src/eepp/window/backend/SDL3/windowsdl3.cpp`: equivalent context pair.
- `Texture` and `TextureLoader` currently acquire the current window's worker context directly.

The refactor needs a process-unique `ResourceId` for diagnostics. It does not introduce public
device epochs or share-group disposal identities. Existing context-current/shared-worker rules
remain authoritative. Tests may destroy and recreate the singleton Engine, but they must first
release all GPU resource handles and verify that factory/manager state is empty.

### 2.2 Device-affine object table

| Object/payload | Current creation and mutation | Current destruction | Current tracker/owner | Required migration |
|---|---|---|---|---|
| Texture GL handle | SOIL and texture upload paths in `texture.cpp` and `textureloader.cpp`; lock, unlock, replace, resize, reload and filter operations issue GL | `Texture::~Texture()` calls `glDeleteTextures` and changes worker context | TextureFactory owns raw Texture and binding/memory state | Final TexturePtr release queues the Texture in TextureFactory; `Window::display()` flushes batches then collects it under the current context; shutdown performs a final collection |
| Temporary readback FBO | GLES `Texture::iLock()` creates, attaches, reads and deletes a framebuffer | Deleted synchronously inside `iLock()` | Stack-local handle | Keep as a device-thread scoped command; never execute from arbitrary last-release thread |
| FrameBuffer FBO | `FrameBufferFBO::create()/resize()/reload()` creates framebuffer and depth/stencil/color renderbuffers | `FrameBufferFBO::~FrameBufferFBO()` directly deletes renderbuffers/FBO and may unbind | FrameBuffer self-registers in non-owning FrameBufferManager; SceneNode/UIWindow/TerminalDisplay own raw objects | Preserve graphics-thread destruction; migrate attachment ownership without a generic GPU disposal layer |
| FrameBuffer texture attachment | `FrameBufferFBO::create()` asks TextureFactory for empty texture | `FrameBuffer::~FrameBuffer()` directly deletes Texture | FrameBuffer exclusive raw ownership, factory also believes it owns the same texture | FrameBuffer stores TexturePtr; no direct deletion; factory registry remains weak |
| VBO/EBO/VAO | `VertexBufferVBO` creates/updates buffers and VAO | `VertexBufferVBO::clear()` directly deletes buffers/VAO; destructor calls clear | VertexBuffer self-registers in non-owning VertexBufferManager; consumer owns raw object | Preserve graphics-thread destruction; owning consumer eventually uses a handle or value owner |
| Renderer streaming VBO/VAO | `RendererGL3CP` owns eight VBOs and one VAO | `RendererGL3CP::~RendererGL3CP()` directly deletes them | Renderer | Preserve Renderer-owned direct destruction before context teardown |
| Shader object | `Shader::Init()/reload()` calls `GLi->createShader`, compiles source | `Shader::~Shader()` directly calls `GLi->deleteShader` | ShaderProgram manually owns raw Shader children | Preserve graphics-thread destruction; ShaderProgram eventually owns ShaderPtr/source handles |
| Program object | `ShaderProgram::init()/reload()` creates and links program | Destructor directly calls `GLi->deleteProgram`, deletes Shader children, self-removes from manager | ShaderProgramManager raw-owns programs; Renderer stores raw default/current program pointers | Preserve graphics-thread destruction and ensure manager precedes Renderer; ownership becomes explicit later |
| Primitive/UI geometry buffers | PrimitiveDrawable, UIBackgroundDrawable and UIBorderDrawable create VertexBuffer objects | Their destructors directly delete VertexBuffer | Per-drawable exclusive raw ownership | Per-consumer drawable owns VertexBuffer handle under the graphics-thread contract |
| Terminal geometry/FBO | TerminalDisplay owns FrameBuffer, background/foreground VBs and style VB vector | Explicit delete/recreate paths | TerminalDisplay | Strong handles; release before device gate closes |

Direct deletion sites found by the audit:

- `src/eepp/graphics/texture.cpp`: texture and temporary framebuffer deletion.
- `src/eepp/graphics/framebufferfbo.cpp`: framebuffer/renderbuffer deletion.
- `src/eepp/graphics/vertexbuffervbo.cpp`: buffer and vertex-array deletion.
- `src/eepp/graphics/renderer/renderergl3cp.cpp`: renderer VBO/VAO deletion.
- `src/eepp/graphics/shader.cpp`: shader deletion.
- `src/eepp/graphics/shaderprogram.cpp`: program deletion.

Renderer wrapper implementations in `src/eepp/graphics/renderer/renderer.cpp` expose the GL delete
entry points; they are dispatch, not independent owners.

### 2.3 Non-owning GPU registries and consumers

- `FrameBufferManager : Container<FrameBuffer>` and `VertexBufferManager : Container<VertexBuffer>`
  observe raw self-registering objects and do not delete them.
- FrameBuffer owners: SceneNode, UIWindow, TerminalDisplay, and direct application/test callers.
- VertexBuffer owners: PrimitiveDrawable, UIBackgroundDrawable, UIBorderDrawable, TerminalDisplay,
  renderer internals, and direct application/test callers.
- Renderer shader arrays (`RendererGL3`, `RendererGL3CP`, `RendererGLES2`) are raw views of programs
  currently owned by ShaderProgramManager.
- `GlobalBatchRenderer` is CPU storage but retains a borrowed Texture pointer until a later flush.
  It is a real lifetime owner in the new model whenever `mNumVertex != 0`.

### 2.4 Device-operation rule

Creation, upload, mutation, context reload, final ownership release and deletion are graphics-
affine. Async CPU decode may run elsewhere; GPU work uses the main context or an API that explicitly
acquires the existing shared worker context. The refactor does not broaden this contract. Debug
assertions and tests should detect wrong-thread release rather than adding generic device scheduling.

## 3. Texture ownership and lookup inventory

### 3.1 Persistent raw texture holders

| Holder | Field/API | Current lifetime assumption | Target classification |
|---|---|---|---|
| TextureFactory | `mTextures: id -> Texture*` | Sole global owner, semantic lookup, diagnostics and deletion | Weak LiveResourceRegistry records; no semantic lookup or ownership |
| TextureLoader | `mTexture`, `getTexture()`, `OnTextureLoaded(Uint32, Texture*)` | Factory owns after loader returns | TexturePtr result/state/callback; operation owns during load |
| TextureRegion | `mTexture`, ID constructors and `setTextureId()` | Factory keeps texture alive | Immutable region source stores TexturePtr; per-consumer region drawable stores source |
| TextureAtlas | `mTextures` and ResourceManager-owned TextureRegion children | Factory owns textures; atlas owns regions | Atlas/source catalog owns TexturePtr and region-source handles |
| TextureAtlasLoader | `mTexturesLoaded` plus ignored queued loads | Relies on factory side effects and later global name lookup | Operation retains TexturePtr results directly and passes them into atlas construction |
| FrameBuffer | `mTexture` | FrameBuffer deletes attachment although factory also registers it | FrameBuffer stores TexturePtr |
| FontTrueType::Page | `texture` and raw GlyphDrawable cache | Page explicitly removes texture from factory | Page stores TexturePtr; glyph source records retain page texture |
| FontBMFont::Page | `texture` and raw GlyphDrawable cache | Same | Same |
| FontSprite::Page | `texture` and raw GlyphDrawable cache | Same | Same |
| GlyphDrawable | `mTexture` | Font page/factory assumed to outlive glyph | GlyphSource stores TexturePtr; render state is external/per consumer |
| UISVGIcon | `mSVGs: size -> Texture*` | Factory owns raster cache | Icon/source cache stores TexturePtr with explicit cache policy |
| ParticleSystem | `const Texture* mTexture` resolved by ID | Factory owns | ParticleSystem stores TexturePtr or immutable texture-source handle |
| maps::TileMap | `mTileTex` | Factory owns generated/named blank-tile texture | TileMap stores TexturePtr |
| BatchRenderer | `const Texture* mTexture` | Caller/resource survives until deferred flush | Strong TexturePtr while queued; reset on flush/discard |
| Tests/test harness | vectors, arrays and locals in `src/tests/test_all` and unit tests | Factory teardown cleans up | Test-local handles/catalog fixtures |

Cursor APIs accept a Texture pointer but immediately lock and copy pixels into an Image. They are
synchronous borrowed parameters, not persistent texture holders. They should accept `const
TexturePtr&` or a documented borrowed `Texture&` depending on the final lock API.

UIColorPicker texture-returning helpers, image viewer, diff view, examples, and tool code mostly
return/use local pointers but must receive TexturePtr because the result crosses a call boundary.

### 3.2 Texture-to-region-to-drawable chains

Raw texture lifetime is also hidden behind raw TextureRegion relationships:

- Sprite frame vectors store `TextureRegion*`, copy them shallowly, mutate frame region size/offset,
  and optionally delete regions/textures according to sprite flags.
- NinePatch exclusively deletes nine generated TextureRegion children, each borrowing one Texture.
- TextureAtlas raw-owns regions through ResourceManager.
- GlobalTextureAtlas owns regions created by ID-based Sprite paths.
- ScrollParallax, UITextureRegion, UISprite, maps GameObjectVirtual/GameObjectTextureRegion, map
  editor state, and UI editor image maps retain TextureRegion pointers.
- TextureAtlasManager returns raw regions and vectors by name/pattern to Sprite and search callers.

Stage 2 therefore removes texture-ID construction and migrates all these region edges. Region IDs
remain stable metadata if useful; they are not a lifetime acquisition mechanism.

### 3.3 Global semantic lookup sites

TextureFactory semantic lookup currently serves unrelated scopes:

- DrawableSearcher searches TextureAtlasManager, NinePatchManager and TextureFactory globally.
- TextureAtlasLoader locates queued results by path after loading.
- UIImage and UINodeDrawable use URL/path names as global cache keys.
- Sprite, TextureRegion, NinePatch and ParticleSystem resolve numeric texture IDs.
- Font page destructors remove by texture ID.
- maps::TileMap resolves a generated blank-tile name.
- ecode settings and uieditor resolve/remove application textures globally.
- Tests rely on `getByName()`, `getTexture()`, and `getTextures()`.

All semantic name/path/URL acquisition moves to ResourceCatalog/ResourceScope. Numeric ResourceId
lookup in LiveResourceRegistry is diagnostic/administrative and returns a weak handle; it is not a
substitute for a catalog.

### 3.4 Loads relying on global side effects

Confirmed ignored or indirect load results:

- `TextureAtlasLoader` queued `loadFromPack()` and `loadFromFile()` calls; later lookup by path.
- `src/tests/test_all/test.cpp` queues/discards pack loads and later resolves globally.
- Theme directory loading directly embeds load results into new TextureRegion/NinePatch/Sprite
  graphs; those destination objects must retain handles in the same cut.
- Font pages assign factory results to raw page fields and explicitly remove them later.
- `Texture::loadGif()` returns a vector of raw frames; Sprite assumes ownership flags/global factory.

Other creation paths return a local pointer and immediately pass it to a current raw holder:

- FrameBufferFBO attachment creation.
- UIImage/UINodeDrawable remote placeholders.
- UISVGIcon and UISVG rasterization.
- FontTrueType, FontBMFont and FontSprite page creation.
- DrawableSearcher file/data/HTTP paths.
- UIColorPicker, UIImageViewer, UIDiffView, uieditor and sprite examples.

The Stage 2 compile cut changes every one to retain or propagate TexturePtr. The temporary factory
retention map is removed only after an audit asserts no ignored result is semantically required.

### 3.5 Explicit deletion and unload semantics

Current deletion paths that must disappear:

- `TextureFactory::remove(id)`, `remove(Texture*)`, `unloadTextures()` and `removeReference()`.
- `TextureLoader::unload()`.
- FontTrueType/FontBMFont/FontSprite Page destructors removing texture IDs.
- FrameBuffer directly deleting its attachment.
- uieditor explicitly removing textures loaded for its image map.
- Sprite cleanup flags that can delete factory textures/regions.

Final equivalents are handle reset, catalog erase, cache eviction, and operation cancellation. None
invalidates another consumer's resource.

### 3.6 Texture callbacks and global state

- TextureLoader has a process-static callback map with `Texture*` payloads. It is not synchronized,
  is not reset with Engine, and UITextureViewer is its only current subscriber.
- DrawableResource Change/Unload callbacks use raw resource pointers and integer IDs.
- UITextureViewer stores Texture pointer -> callback ID and expects Unload to remove rows.
- TextureFactory memory accounting and Texture mutation call each other through the singleton.
- TextureFactory performs reload/grab/ungrab while holding its registry lock and invokes texture/GL
  operations under that lock.

Target:

- LiveResourceRegistry emits diagnostic record changes or supplies snapshots; viewer uses weak
  handles.
- Source mutation signals use RAII connections and weak subscriber tokens.
- ResourceMetrics is captured state, independent of factory lifetime.
- Registry locks protect records only; device work and callbacks occur after unlocking.

## 4. Drawable mutation and ownership inventory

### 4.1 Base-class result

`Drawable` itself stores mutable `mColor` and `mPosition`, and exposes setColor, setAlpha and
setPosition. Consequently no existing subclass is generally shareable merely because its
`isStateful()` returns false.

The target source/instance split remains valid, with one performance qualification: high-frequency
glyph and text rendering should use immutable glyph sources plus external draw parameters instead of
allocating a mutable drawable instance per rendered glyph.

### 4.2 Class classification

| Current class/family | Current mutation/ownership behavior | Target form |
|---|---|---|
| Texture | Mutable color/position from Drawable plus mutable GPU data, filters, clamp, local cache and name | Shared Texture resource only; drawing uses TextureDrawable instance or external draw params |
| TextureRegion | Mutates destination size inside `draw(position,size)`; mutable source rect, offset, pixel cache, color/position | Immutable TextureRegionSource retaining TexturePtr; TextureRegionDrawable instance/presentation state |
| GlyphDrawable | Cached and shared by font pages but has color, position, draw mode, italic flag, offset, size and advance; UICodeEditor temporarily changes draw mode | Immutable GlyphSource; text/editor pass draw mode, color, position and size as parameters |
| NinePatch | Owns nine mutable regions; draw updates own size/position and every child; propagates color/alpha | Immutable NinePatchSource plus private per-consumer NinePatchDrawable layout state |
| Sprite | Animation state, callbacks, transforms, current frame, repetitions and shallow-copied region vectors; mutates region size/offset | Per-consumer Sprite instance retaining immutable frame sources |
| PrimitiveDrawable and Rectangle/Triangle/Arc/Circle/ConvexShape | Mutable geometry, color, position, fill/blend/line state and owned VertexBuffer cache | Per-consumer drawable instance; optional immutable geometry source only if later useful |
| Linear/RadialGradientDrawable | Mutable stops, angle/shape/center/extent, size, color and position | Parsed immutable gradient source plus per-consumer instance, or fresh instance directly from CSS parser |
| DrawableGroup | Optional global child-owner bool; draw/update mutates own size/position and child positions/alpha | Per-consumer composite owning private mutable child instances/source handles |
| StateListDrawable | Raw state map plus pointer->bool ownership; mutable current state; draw temporarily changes child alpha; state color mutates child | Per-consumer state machine owning instances or source factories; no child mutation shared with another list |
| UISkin | StateListDrawable; `clone()` shallow-copies pointers and ownership map, duplicating ownership claims | Skin source/definition in theme catalog; create independent skin/state-list instances |
| RichText | Mutable layout/selection; stores raw inline/background/border drawables and temporarily recolors backgrounds during draw | Per-consumer RichText; retained source/instance handles; external color draw parameters |
| UINodeDrawable | Node-owned layer map, geometry/cache state and nested background drawable | Per-node/per-consumer instance |
| UINodeDrawable::LayerDrawable | Manual `mOwnsDrawable`; mutable repeat/clip/origin/size/offset; draw mutates child alpha/color; async placeholder | Per-node layer owning DrawablePtr instance created by UI::DrawableResolver |
| UIBackgroundDrawable/UIBorderDrawable | Owner-node pointer, mutable geometry/radii/colors/position/size, owned VertexBuffer | Per-node instance; VertexBuffer handle |
| DrawableResource/StatefulDrawable | Name/ID plus Change/Unload callback lifetime protocol | Source identity plus typed invalidation signal; no destructor Unload callbacks |

### 4.3 Other drawable holders requiring migration

- UIImage: raw drawable plus `mDrawableOwner`; temporarily recolors it during draw.
- UIPushButton icon delegates the same ownership flag to UIImage.
- UINode background/foreground APIs expose `ownIt`.
- DrawableImageParser returns `Drawable*` plus `bool& ownIt` for gradients, shapes, URLs and icons.
- UIIcon stores size -> Drawable raw pointers; UIGlyphIcon borrows FontTrueType and font-owned glyph
  drawables; UISVGIcon separately caches textures.
- UITheme owns UISkin objects through ResourceManagerMulti; UIIconTheme manually owns UIIcon
  objects; skins point into global atlas/nine-patch resources.
- Models::Variant stores Drawable in a C-style union and copies both pointer and owner flag.
- RichText inline boxes/fragments store backgroundColorDrawable/backgroundDrawable/borderDrawable
  raw pointers.
- UICodeEditor stores fold/unfold drawables and temporarily changes GlyphDrawable draw mode.
- ClippingMask stores temporary borrowed Drawable pointers and calls draw later; its operation must
  remain bounded by the owner or retain instance handles while queued.
- ecode/tool/plugin configuration and tab splitter structures store icon Drawable pointers.

### 4.4 Complete manual ownership-flag surface

The audit found ownership flags in:

- DrawableGroup (`mDrawableOwner`, `setDrawableOwner()`).
- StateListDrawable (`mDrawablesOwnership`, per-state `ownIt`).
- UISkin clone copying StateListDrawable ownership state.
- UIImage (`mDrawableOwner`, `safeDeleteDrawable()`, `setDrawable(..., ownIt)`).
- UINodeDrawable::LayerDrawable (`mOwnsDrawable`).
- UINode background/foreground APIs.
- UIPushButton icon API.
- DrawableImageParser function and return protocol.
- Models::Variant (`mOwnsObject` for Drawable).

All are removed in Stage 4. No compatibility overload remains.

## 5. Asynchronous producer inventory

| Producer | Work and current captures | Current stop behavior | Required contract |
|---|---|---|---|
| Http global Pool/Http AsyncRequest | UIWebView documents, UIImage/UINodeDrawable placeholders, DrawableSearcher, font faces; callbacks can mutate scenes/textures or queue main-thread work | Pool clear erases shared clients under mutex; Http destructor joins private request threads, but optional global-ThreadPool tasks capture raw Http and are not joined | Pool/service close rejects requests, swaps clients out under lock, unlocks, then cancels/joins all tracked operations regardless of executor; operation subscribers use weak session tokens |
| UISceneNode ThreadPool | File texture load/upload, SVG/image decode, deferred fonts/styles; pool may be shared with host/application | Scene invalidates generation and deletes children; shared pool may outlive scene; ThreadPool destructor drains queued work by default | Owning scene/document service tracks operations, captures no raw scene, and is joinable; Engine does not destroy arbitrary external pools |
| Static UISceneNode async-main queue | Lambdas queued by HTTP/thread workers with resource state/generation | Drained only during UISceneNode scheduled update; no Engine clear | UI-owned delivery queue has close/reject/invalidate/purge semantics before scene destruction |
| TextureLoader | Decode and optional direct GL upload on calling/worker thread; static global callbacks | Stack loader; no service-level shutdown; callbacks unsynchronized | TextureLoadOperation owns TexturePtr/result; GPU upload follows explicit current/shared-context rules; typed observers are synchronized |
| ResourceLoader | Internal Thread plus temporary ThreadPool drains all tasks before destructor returns | Destructor waits, but cannot cancel running work | Close/cancel token and join; callbacks never use destructed owner state |
| TextureAtlasLoader | ResourceLoader tasks call factory, completion callback accesses loader and managers | ResourceLoader member is destroyed last due declaration order | Loader operation/state shared independently; join before state destruction; retain texture results directly |
| UIWebView navigation | HTTP callbacks use weak NavigationLoadState and generation, then main-thread document replacement | Good stale-delivery guard, but request is global and cache-unaware | Per-document subscriber/session over shared request; stale tab does not cancel other subscribers |
| UIImage/UINodeDrawable remote paths | Capture raw placeholder Texture and raw `this`, partially guarded by alive atomics/generation | Callback may still release/mutate on HTTP thread; factory owns placeholder | Capture TexturePtr and weak consumer token; decode off-thread, upload/device mutation scheduled, scene update generation-guarded |
| UISVG and image tools | Shared scene pool tasks often capture raw `this` and later runOnMainThread | Per-widget tags/alive handling varies | Convert resource-producing paths to operation/subscriber tokens; application-only CPU tasks remain app responsibility |

ThreadPool itself waits for all queued work unless `terminateOnClose` is set. That behavior is useful
but is not a global shutdown mechanism because ownership is distributed. Each UI/Web/cache service
tracks operations that access its state even when execution uses a provided shared pool.

## 6. Current Engine teardown dependency audit

Current order in `Engine::~Engine()`:

```text
GlobalBatchRenderer
NinePatchManager
SceneManager
StyleSheetSpecification / SyntaxDefinitionManager
FontManager
TextureAtlasManager
TextureFactory
Renderer
ShaderProgramManager
PackManager
FrameBufferManager / VertexBufferManager
VFS
SSL end
HTTP Pool clear
Windows / contexts
backend and process caches
TextLayout cache
SystemFontResolver
```

This list records the pre-prerequisite order found by the audit. On 2026-07-13 the deterministic
portion was corrected: pool-owned HTTP clients stop first; the selected context is made current;
scenes precede GlobalBatchRenderer and global resource managers; TextLayout precedes FontManager;
ShaderProgramManager precedes Renderer; and windows/contexts remain until all Graphics singleton
teardown is complete. Complete shared-executor and static UI-delivery barriers remain assigned to
their prerequisite work packages.

Concrete violations:

| Current edge/order | Violation |
|---|---|
| NinePatchManager before SceneManager | Scenes/widgets can still hold raw global nine-patch/region pointers; relies on Unload callbacks during teardown |
| GlobalBatchRenderer destroyed first | Pending submissions are discarded without an explicit dependency release contract; future strong queued texture handles need explicit discard |
| TextLayout cache after FontManager | Cached ShapedGlyph objects contain raw FontTrueType pointers after fonts are deleted |
| Renderer before ShaderProgramManager | Renderer destruction sets global `GLi = nullptr`; ShaderProgram and Shader destructors then call through GLi |
| TextureFactory before external FrameBuffer/VBO/program handles | External resources can outlive managers and their destructors can recreate manager/factory singletons or touch dead GL state |
| FrameBufferManager/VertexBufferManager after Renderer | Managers are non-owning today, but any live registered object's deletion needs Renderer/context; order provides no guarantee |
| HTTP Pool after all Graphics resources | Callbacks can mutate placeholders, create resources, enqueue scene work, or release final handles after consumers/device systems are gone |
| HTTP Pool clear under its own mutex | Http destruction joins callbacks; callback re-entry to global Pool can deadlock |
| HTTP using `sGlobalThreadPool` | Queued task captures raw Http; Pool clear can delete Http because its destructor cannot join externally executed work |
| Windows last without per-context drain | Direct deletion happens against whichever context happens to be current, not necessarily the object's namespace |
| Log before late static cache cleanup | Future abandoned-resource diagnostics would lose logging if any resource/cache remains |

## 7. Target shutdown dependency graph

### 7.1 Dependency graph

```text
HTTP / decode / atlas / scene-pool executors
                    │ produce
                    ▼
Owning UI/document operation state + WebResourceCache requests
                    │ deliver through generation/session tokens
                    ▼
Scenes / UI documents / drawable instances / app caches
                    │ retain
                    ├──────────────► Fonts ─► glyph sources ─► Textures
                    ├──────────────► Atlases ─► region sources ─► Textures
                    ├──────────────► Nine-patch/sprite sources ─► region sources
                    ├──────────────► FrameBuffers ─► attachment Textures
                    └──────────────► VertexBuffers / queued batches

Global/default catalogs ───────────► any published resource/source
TextLayout LRU ────────────────────► Fonts used by cached shaped runs
Renderer ──────────────────────────► default Programs + streaming VBO/VAO

TexturePtr final release ─► TextureFactory released queue ─► Window::display collection
Other GPU objects ────────────────────────────────────────► graphics-thread destruction
Both paths ───────────────────────────────► Renderer/GL dispatch ─► Window context
```

An arrow means the left side must stop producing or release its dependency before the right side is
detached/destroyed. Shared handles make sibling release order less fragile, but graphics-thread and
context order remain strict.

### 7.2 Concrete Engine shutdown sequence

1. **Enter shutdown.** Mark Engine, WebResourceCache and resource delivery queues as closing. Reject
   new resource, cache, HTTP-for-resource, upload, reload and main-thread delivery operations.
2. **Invalidate subscribers.** Invalidate all document sessions, UISceneNode async generations,
   widget subscribers, navigation states, and cache leases. UI objects still exist, so cancellation
   callbacks that must observe them can do so through checked weak tokens.
3. **Stop producers without holding service locks.** Swap global HTTP clients/request sets and
   tracked operations into local containers under their locks; unlock; cancel and join them. Close
   and join TextureAtlas/Texture load operations and Web cache fetch/decode operations. A provided
   external ThreadPool remains alive, but no tracked task may still access Engine/UI resource state
   after this barrier.
4. **Purge delivery queues.** Remove pending UIScene/resource main-thread deliveries and release
   their captured handles. No queue can accept new entries after step 1.
5. **Stop rendering submissions.** Discard pending GlobalBatchRenderer vertices and release its
   TexturePtr; ensure no window/scene draw is active. Do not attempt a cosmetic final render.
6. **Destroy scenes/documents.** Destroy SceneManager and all child UISceneNodes/widgets. This
   releases document scopes, UI resolvers, UI themes/icons/skins, drawables, scene/app cache leases,
   SceneNode/UIWindow/Terminal framebuffers, primitive/UI/terminal vertex buffers, and scene-owned
   fonts. Scenes precede global source/catalog teardown.
7. **Clear CPU caches retaining resources.** Clear TextLayout LRU and any font/glyph/drawable lookup
   caches. Clear application and global ResourceCatalogs/default scopes. Destroy StyleSheet and
   syntax/UI resolver specification state after scenes no longer use it.
8. **Release high-level Graphics owners.** Release NinePatch, atlas/region, font/glyph and remaining
   theme/icon manager/catalog handles in dependency order. Clear font fallback/style links before
   releasing fonts. TextureFactory/LiveResourceRegistry remain present as weak observers only.
9. **Collect released textures.** Make the active context current, flush/discard pending batch
   submissions, and call `TextureFactory::collectReleasedTextures()`. Purge expired weak records and
   report/assert any unexpected externally owned TexturePtr. Tests treat every survivor as a fixture
   teardown failure; a defensive GPU-payload release may make production shutdown safe to continue.
10. **Release renderer-owned resources.** Destroy ShaderProgramManager and framebuffer/vertex
    owners before Renderer. Renderer then releases its default programs and streaming VBO/VAO while
    the context is valid. Assert TextureFactory has no pending released objects before it is destroyed.
11. **Destroy Graphics roots.** Destroy TextureFactory's weak registry and CPU state, Renderer/GL
    dispatch, and non-owning manager shells. A TexturePtr surviving this point violates the project
    contract; it is not supported through a second device lifetime.
12. **Destroy loading infrastructure.** Destroy PackManager and VFS after all tracked resource work
    and reload-capable internal owners are gone. End SSL after HTTP clients have joined.
13. **Destroy windows and backend.** Destroy each worker/primary GL context and window, then platform,
    display and backend state.
14. **Destroy process caches and logging last.** Clear SystemFontResolver/FreeType resolver state,
    parser/regex caches, and finally Log. MemoryManager reporting happens after test/application
    handles and catalog fixtures have been released.

### 7.3 Window destruction outside Engine teardown

`Engine::destroyWindow()` can remove one context while Engine and other windows continue. It needs
the same per-context mini-sequence:

1. Reject new operations targeting that window/context.
2. Cancel/join its tracked uploads and release window/scene-owned resources.
3. Flush pending submissions and collect textures releasable under that current context.
4. Release renderer/context-local payloads.
5. Destroy the contexts/window.

The existing multi-window/shared-context contract determines which texture objects are valid under
the remaining contexts. This refactor does not introduce a second context ownership model.

## 8. Test-isolation and build matrix decision

### 8.1 Required build configurations

The repository enables `EE_MEMORY_MANAGER` in debug configurations and builds both eepp static and
shared libraries. Stage 1/2 validation therefore requires at least:

| Configuration | Purpose |
|---|---|
| Linux debug static, EE_MEMORY_MANAGER | Primary unit-test and tracked deleter correctness |
| Linux debug shared, EE_MEMORY_MANAGER | ResourcePtr/custom-deleter behavior across library boundary |
| Linux release static | Behavior without MemoryManager macros and optimized lifetime paths |
| Linux release shared | Public handle ABI and cross-DSO destruction without debug tracking |
| Debug static + AddressSanitizer/LeakSanitizer | UAF, double control block, leaks and late callbacks |
| Debug static + ThreadSanitizer | Registry/cache/async operation races and contract violations |

Existing Premake options include `with-static-eepp`, `address-sanitizer`, and `thread-sanitizer`.
Platform CI can expand after Linux substrate tests are stable; Windows/macOS context implementations
must pass deferred texture collection and teardown-order tests before the public refactor is complete.

### 8.2 Per-test fixture contract

Each resource test owns an explicit fixture containing Engine/window if needed, catalogs/scopes, Web
cache/session objects, and resource handles. Teardown order is:

1. invalidate subscribers and stop tracked operations;
2. release test widgets/scenes/caches/catalogs/handles;
3. flush batches, collect released textures and destroy Engine;
4. purge expired registry records and static callbacks/caches;
5. assert no unintended catalog entries, operation records, pending deliveries, released textures,
   or live texture registry entries;
6. assert no resource handle survives Engine destruction;
7. compare MemoryManager/registry diagnostics to fixture baseline.

No test may depend on a previous test's global TextureFactory name entry, TextureLoader callback,
TextLayout cache, font fallback cache, or Engine ID counter reset.

## 9. Source-audit gates

Stage 2 and Stage 4 should preserve repeatable repository checks. The exact implementation may use
clang-tidy, but these searches define the initial gates:

```sh
rg '\b(?:const\s+)?Texture\s*\*' include src --glob '!src/thirdparty/**'
rg 'TextureFactory::instance\(\)->(getByName|getByHash|getTexture|remove)' include src
rg 'TextureFactory::instance\(\)->(loadFrom|createEmptyTexture|pushTexture)' include src
rg '(ownIt|mOwnsDrawable|mDrawableOwner|mDrawablesOwnership|setDrawableOwner)' include src
rg '(glDelete[A-Za-z]*|GLi->delete[A-Za-z]*)' include/eepp src/eepp
```

The goal is not zero raw pointers everywhere. Allowed remaining matches must be one of:

- a local borrowed `.get()` view whose owning handle is in the same lexical object/call;
- a synchronous reference parameter with documented lifetime;
- low-level GL dispatch declarations;
- a diagnostic weak-lock result used within the lock's strong-handle scope.

Every stored raw resource field requires an explicit code-review annotation and should normally be
rejected.

## 10. Stage 0 exit assessment

Stage 0 deliverables are satisfied:

- GPU resource classes/direct deletion sites: inventoried.
- Graphics-thread and TextureFactory deferred-release requirements: frozen.
- Raw Texture holders, ID/name lookup, ignored loads, callbacks and deletion calls: inventoried.
- Drawable classes, mutation and manual ownership surfaces: inventoried and classified.
- Resource-related asynchronous producers and stop semantics: inventoried.
- Current and target Engine shutdown dependency graph: documented.
- Shared/static/MemoryManager/sanitizer build matrix: frozen.
- Unit-test isolation contract: frozen.

Stage 0.5 fixes the concrete defects listed by this audit before ownership work resumes. Stage 1
then adds TextureFactory-specific lifetime scaffolding without changing public Texture ownership;
current factory retention remains active until the complete Stage 2 holder migration.
