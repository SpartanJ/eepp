# Resource-refactor prerequisite bug-fix execution plan

Status: active; work packages 1 and 2 completed, 2026-07-13.

This plan defines the bounded correctness work to complete before Stage 1 of the shared-resource
ownership refactor. It turns the findings in `resource_refactor_prerequisite_bugfixes.md` into an
ordered implementation sequence. The defect ledger remains the source of detailed evidence; this
document defines execution order, dependencies, validation, and the point at which prerequisite
work stops.

Related documents:

- `resource_refactor_prerequisite_bugfixes.md`
- `resource_shared_ownership_architecture.md`
- `resource_shared_ownership_stage0_inventory.md`

## 1. Objective and boundary

Fix current correctness defects that would make the ownership migration unsafe or unnecessarily
difficult, without introducing `ResourcePtr`, resource catalogs, scopes, deferred texture release,
or compatibility APIs.

This is not a general cleanup phase. A defect belongs here only when at least one of these is true:

- It can currently cause a deadlock, use-after-free, double deletion, stale cross-Engine work, or
  invalid GL access.
- It prevents deterministic destruction of current Engine-owned systems.
- It prevents loaders and asynchronous producers from being stopped safely before resource
  teardown.
- It is a small, independent correctness bug found during the audit and can be fixed without
  designing an API that Stage 1 or a later migration will immediately replace.

Once the work packages below satisfy their exit criteria, begin Stage 1. Do not delay Stage 1 for
raw resource ownership problems already assigned to Stages 2 through 7.

## 2. Landing rules

- Land one defect or one tightly coupled lifetime cluster per change.
- Add focused regression coverage before or with each fix.
- Preserve current TextureFactory ownership and current public resource APIs.
- Avoid temporary ownership abstractions that compete with the accepted final architecture.
- Do not invoke callbacks, destroy callback-visible objects, perform GL work, or join threads while
  holding a registry/pool mutex.
- Regenerate the build before compiling, format modified C++ sources, and run the relevant focused
  unit-test suite under `xvfb`.
- Use ASAN for lifetime/destruction changes and TSAN where concurrent state is changed.
- For Engine teardown changes, run repeated Engine creation/destruction in one process.

## 3. Work package 1: small independent correctness fixes

These fixes are low risk and do not depend on the larger lifetime changes.

### 3.1 TextureAtlasLoader texture-filter count

Current defect:

```cpp
size_t count = getTextureAtlas()->getTexturesCount() == 0;
```

The expression stores a boolean instead of the texture count. When textures exist, `count` becomes
zero and no filter is applied. When no textures exist, it becomes one and the loop may request
texture index zero.

Fix:

- Store the actual texture count.
- Apply the filter to every loaded atlas texture.
- Handle a null or not-yet-created atlas consistently with the surrounding loader API.

Validation:

- An atlas with multiple textures updates every texture.
- An empty/not-yet-loaded atlas performs no invalid access.

Status: implemented and covered by `ResourcePrerequisites` unit tests. Focused ASAN tests pass.

### 3.2 Unsigned Models::Variant type

Current defect:

`Variant(const unsigned int&)` stores the value in `asUint` but sets `mType` to `Type::Int`.

Fix:

- Set `mType` to `Type::Uint`.
- Add construction, copy, move, assignment, `is(Type::Uint)`, `asUint()`, and `toString()` coverage.
- Include a value greater than `INT_MAX` so signed reinterpretation cannot pass unnoticed.

Status: implemented and covered by `ResourcePrerequisites.unsignedVariantPreservesTypeAndValue`.
The focused test and existing `StringMapModel` tests pass under ASAN.

### 3.3 Texture copy-construction trap

`Texture` already inherits privately from `NonCopyable`, but it still implements a protected copy
constructor that copies the GL texture handle. This is dangerous if an internal/friend path ever
uses it.

Fix:

- Explicitly delete the Texture copy constructor and copy assignment in `texture.hpp`.
- Remove the copy-constructor implementation.
- Add compile-time non-copyability assertions.

This is defensive cleanup rather than a currently observed public copy path and must not block the
following packages if it exposes unrelated legacy code.

Status: implemented. The obsolete implementation was removed and compile-time non-copyability
checks cover both construction and assignment.

### 3.4 Empty ResourceLoader progress

Current defect:

`ResourceLoader::getProgress()` divides by `mTasks.size()` without handling an empty loader.

Fix:

- Define empty-loader progress explicitly. Prefer `100%` when an empty load is considered complete;
  otherwise use `0%` consistently with `isLoaded()` semantics.
- Add focused coverage for empty, partially completed, and completed loaders.

Status: implemented. An empty loader reports `0%` before loading and `100%` after completing an
empty load. Focused unit coverage verifies both states.

### 3.5 MemoryManager concurrent bootstrap

Current defect found during Work Package 2 TSAN validation:

`MemoryManager::addPointer()` conditionally skips `sAllocMutex` until a process-global `sHasInit`
flag is set. Concurrent first tracked allocations race on that flag and can enter the allocation
map and accounting counters without mutual exclusion.

Fix:

- Replace translation-unit bootstrap globals with one thread-safe function-local state.
- Keep that tracking state alive through process-static destruction so late `eeDelete()` calls do
  not depend on static destruction order.
- Always lock allocation-map and accounting access, including metric getters.
- Return the biggest-allocation snapshot by value instead of exposing an unlocked mutable record.

Status: implemented. The focused TSAN suite initially reproduced the race in
`MemoryManager::addPointer()`. After the fix, all six `ResourcePrerequisites` tests pass under
TSAN without suppressions. The ASAN build and focused tests also pass.

Work-package exit criteria:

- Each fix has an isolated regression test.
- No resource ownership API has changed.

## 4. Work package 2: ResourceLoader and TextureAtlasLoader lifetime

This package establishes reliable loader destruction before changing texture ownership.

### 4.1 ResourceLoader synchronization audit

Current worker and caller threads read and write `mLoaded`, `mLoading`, and `mTotalLoaded`. Treat
these accesses as shared state rather than relying on timing.

Fix requirements:

- Synchronize status and progress state with atomics or a narrowly scoped mutex.
- Define which thread invokes completion callbacks. Preserve current behavior unless deliberately
  changing it with documented caller migration.
- Never invoke completion callbacks while holding the loader-state mutex.
- Ensure task and callback containers cannot be modified while worker execution reads them.

### 4.2 ResourceLoader destruction contract

`ResourceLoader` owns and joins its worker from its destructor. There is no consumer-facing need
for a separate terminal shutdown state or public shutdown operation.

Contract:

- The destructor waits for the runner and its internal ThreadPool work before clearing tasks and
  callbacks.
- A loader is not destroyed from one of its own tasks or completion callbacks.
- Owners whose callbacks access sibling members must encode a destruction order that destroys the
  loader before those sibling members.

### 4.3 TextureAtlasLoader member order

Current member order destroys callback-visible atlas state before `mRL`, whose destructor performs
the join.

Fix:

- Declare `mRL` as the final data member so it is destroyed first and joins before callback-visible
  atlas state is destroyed.
- Document the required order beside the member.
- Keep a destruction regression test protecting the invariant.

Validation:

- Destroy a loader immediately after queuing several tasks.
- Verify under ASAN that no task or callback accesses destroyed loader members.
- Run synchronization coverage under TSAN where available.

Work-package exit criteria:

- TextureAtlasLoader's required destruction order is documented and covered by a regression test.
- ResourceLoader status/progress reads are data-race-free.
- No callbacks execute under internal synchronization locks.

Status: implemented. Status/progress counters are atomic, task and callback container access is
synchronized, and callbacks execute after releasing internal locks. `ResourceLoader` joins its
worker in its destructor without exposing a terminal shutdown API. `TextureAtlasLoader` declares
`mRL` last, documents the order invariant, and publishes asynchronous status through atomics.
Focused ASAN coverage verifies atlas destruction while tasks and a completion callback are pending.
All six `ResourcePrerequisites` tests pass under both ASAN and unsuppressed TSAN.

## 5. Work package 3: shared ThreadPool HTTP operation lifetime

This follows the already fixed `Http::Pool::clear()` lock-order defect.

Current defect:

When `Http::setThreadPool()` is active, queued lambdas capture raw `Http*`. `Http` tracks and joins
only its privately created `AsyncRequest` threads, so a shared-pool operation may begin or continue
after its Http object has been destroyed.

Fix requirements:

- Register every asynchronous operation before publishing it to an executor.
- Give queued/running operations lifetime and cancellation state independent of raw `Http*`.
- `Http::~Http()` rejects new work, cancels all registered operations, and waits until no operation
  can dereference the object.
- Handle destruction initiated from an operation callback without joining/waiting on the same
  operation.
- Do not destroy or drain an externally owned shared ThreadPool.
- Do not wait while holding the global Http Pool mutex, operation-map mutex, or any lock visible to
  callbacks.
- Preserve cancellation callback behavior deliberately and document it.
- Cover all three async forms: response in memory, external IOStream, and output path.

Validation:

- Queue a request behind blocked shared-pool work, destroy/clear its Http owner, then unblock it.
- Destroy while a request is running.
- Re-enter `Http::Pool` from a callback during concurrent pool clearing.
- Initiate final-owner release from a callback and verify no self-deadlock.
- Run under ASAN and TSAN.

Work-package exit criteria:

- No shared-pool lambda depends on an untracked raw Http lifetime.
- Http destruction provides a complete operation barrier without taking ownership of the executor.

## 6. Work package 4: deterministic Engine shutdown

Reorder shutdown only after HTTP and loader barriers are reliable.

### 6.1 Required dependency order

The exact implementation may group calls differently, but it must preserve this dependency graph:

```text
mark Engine shutting down / reject new deliveries
    -> stop and join HTTP and resource-producing work
    -> invalidate and purge UI main-thread deliveries
    -> destroy scenes
    -> flush or discard GlobalBatchRenderer submissions
    -> clear TextLayout and shaped-font caches
    -> destroy UI/global drawable providers and resource managers
    -> destroy fonts, atlases and textures in dependency order
    -> destroy shader programs and shaders
    -> destroy framebuffer and vertex-buffer registries/owners
    -> destroy Renderer
    -> destroy windows and GL contexts
    -> destroy process utilities and backend state
```

Concrete corrections required:

- Stop `Network::Http::Pool` and Engine-owned resource producers before Graphics consumers.
- Destroy `SceneManager` before `GlobalBatchRenderer` and `NinePatchManager` resources scenes can
  reference.
- Flush or explicitly discard pending batches before releasing their borrowed dependencies.
- Call `TextLayout::clearLayoutCache()` before `FontManager::destroySingleton()`.
- Destroy `ShaderProgramManager` before `Renderer` while `GLi` and a valid context still exist.
- Keep windows/context state alive through every GPU-object destruction step.

### 6.2 Validation

- Engine destruction with a live scene using fonts, nine-patches, textures, batches, shaders, FBOs,
  and vertex buffers.
- Engine destruction with pending HTTP and decode/resource-loader work.
- Multiple Engine create/destroy cycles in one test process.
- Assert no destructor recreates an Engine or manager singleton.
- ASAN/LSAN clean teardown in supported configurations.

Work-package exit criteria:

- All asynchronous producers are behind a shutdown barrier before their consumers are destroyed.
- Every GPU-owning manager is destroyed while its required Renderer/context services remain valid.
- Repeated Engine lifecycle tests pass.

## 7. Work package 5: UISceneNode async delivery lifecycle

Current behavior:

Async resource deliveries are stored in a process-static queue. Generation/alive checks prevent
many stale callbacks from mutating a dead scene, but queued closures and their captures can survive
until an unrelated future scene update and cross an Engine test boundary.

Fix requirements:

- Add explicit accept/reject/drain/purge lifecycle operations for the queue.
- Reject new deliveries once UI/Engine shutdown begins.
- Invalidate scene generation/alive state before purging queued closures.
- Release captures on the main/update thread, following the existing project destruction contract.
- Re-open/reset the delivery mechanism deliberately for a recreated test Engine.
- Do not allow work queued by Engine lifecycle A to execute during lifecycle B.

Validation:

- Queue immediate and delayed deliveries, destroy the scene before update, and verify neither runs.
- Destroy and recreate Engine, then update a new UISceneNode and verify no old closure executes.
- Race worker submission with queue shutdown under TSAN.

Work-package exit criteria:

- The static queue has an explicit Engine lifecycle boundary.
- Purging releases all stale captures deterministically.

## 8. Work package 6: FrameBufferFBO recreation correctness

First determine the intended callers and semantics of `FrameBufferFBO::reload()`.

Required distinction:

- Live-context recreation must release the previous framebuffer/renderbuffer objects before
  replacing their handles.
- Context-loss recreation must forget names from the lost namespace without issuing invalid delete
  calls against the replacement context.

Additional create-path audit:

- Restore prior framebuffer/renderbuffer bindings on every failure return.
- Release partially created objects on live-context failure.
- Leave the object in a destructible, clearly invalid state after failure.
- Do not change texture-attachment ownership in this package; that belongs to TexturePtr Stage 2.

Validation:

- Repeated live-context recreation does not grow tracked GL object counts.
- Context-loss recreation performs no deletion in the lost namespace.
- Forced create failures restore previous bindings and do not leak partial objects.

Work-package exit criteria:

- `reload()` has an explicit live/lost-context contract.
- Replacement and failure paths have deterministic GL-handle cleanup.

## 9. Work package 7: TextureLoader callback registry safety

Current behavior:

`TextureLoader::sCbs` is process-static and unsynchronized. Loading may notify from a worker while
UITextureViewer or another caller registers/removes callbacks.

Fix requirements:

- Synchronize callback registration and removal.
- Copy/snapshot callbacks under the lock and invoke the snapshot after unlocking.
- Define removal-during-notification behavior.
- Add an explicit test/Engine lifecycle reset operation if the registry remains process-static.
- Ensure callbacks from an old Engine lifecycle cannot target UI state in a recreated Engine.

Validation:

- Concurrent registration, removal, and notification under TSAN.
- Callback re-entry into registration/removal does not deadlock.
- Repeated Engine lifecycle does not inherit callback subscriptions.

This package may be omitted only if Stage 1 immediately replaces this registry with the accepted
weak live-texture diagnostics mechanism. The omission must be an explicit Stage 1 scope decision,
not an assumption.

## 10. Deferred findings: do not solve in this plan

The following defects remain recorded but should normally be resolved by their owning migration
stage because a raw-pointer workaround would be short-lived or semantically incomplete:

- `Variant` copying an owning Drawable pointer: resolve with the Stage 4 handle/`std::variant`
  redesign unless a current reproducer requires an emergency restriction.
- `UISkin::clone()` and `StateListDrawable` duplicating ownership maps: resolve with Stage 4
  source/instance semantics unless a current owning-child clone path is demonstrated.
- FrameBuffer texture attachment direct/factory ownership: resolve in the complete TexturePtr
  holder cut in Stage 2.
- TextureRegion, TextureAtlas, fonts, glyphs, sprites, particles, and batches borrowing textures:
  resolve together in Stage 2.
- Mutable Drawable sharing and incorrect `isStateful()` classifications: resolve in Stage 4.
- TextureFactory lock scope, weak diagnostics, semantic lookup, metrics, and deferred release:
  these are Stage 1 through Stage 3 architecture work, not prerequisite patches.

## 11. Final prerequisite gate

Stage 1 may begin when all mandatory packages satisfy these invariants:

- HTTP and ResourceLoader work cannot outlive the objects they dereference.
- Engine shutdown rejects and joins asynchronous resource producers before destroying scenes or
  Graphics systems.
- UISceneNode queued delivery cannot cross an Engine lifecycle boundary.
- Scenes and caches are destroyed before the resources they borrow.
- Shader, font-layout, Renderer, and context destruction order is valid.
- TextureAtlasLoader destruction is safe with active work.
- FrameBuffer recreation cannot leak or delete objects from the wrong context namespace.
- Focused ASAN tests and repeated Engine create/destroy tests pass.

After this gate, start Stage 1 immediately. Any newly discovered issue is added to this prerequisite
track only if it violates one of these invariants; otherwise it is assigned to its resource-family
migration stage.
