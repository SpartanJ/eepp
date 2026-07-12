# Resource-refactor prerequisite bug fixes

Status: active defect track, 2026-07-12.

This document isolates correctness defects discovered during the shared-resource ownership audit.
They should be fixed before the public resource API refactor wherever practical. Fixes in this track
must preserve current ownership APIs unless the defect cannot be corrected safely without the later
structural migration.

Related documents:

- `resource_shared_ownership_architecture.md`
- `resource_shared_ownership_stage0_inventory.md`

## 1. Landing rules

- One defect or tightly coupled lifetime defect per change.
- Add focused regression coverage before or with the fix.
- Do not introduce ResourcePtr, catalogs, scopes or compatibility APIs in this track.
- Preserve current TextureFactory ownership until the Stage 2 holder cut.
- Run the relevant focused suite plus repeated Engine create/destroy coverage for teardown changes.
- Run ASAN for UAF/double-delete defects and TSAN for shared callback/queue synchronization defects.

## 2. Priority A: shutdown and asynchronous lifetime

### A1. HTTP Pool destruction while holding its mutex

Current behavior:

`Http::Pool::clear()` clears `mHttps` while holding `mMutex`. Destroying an Http joins local async
requests. A callback that re-enters the global Pool then waits for the same mutex, while `clear()`
waits for the callback to finish.

Fix:

- Swap the client map into a local container under the mutex.
- Release the mutex.
- Destroy/join clients from the local container.

Regression coverage:

- An async callback re-enters `Pool::get()` while another thread calls `Pool::clear()`.
- The test completes under a bounded timeout without deadlock.

Status: implemented with `Http.poolClearAllowsCallbackReentry`; focused ASAN suite passes.

### A2. Shared ThreadPool tasks capture a raw Http

Current behavior:

When `Http::setThreadPool()` is configured, async lambdas capture raw `this`. Http tracks only its
privately created AsyncRequest threads for joining. Pool clear can destroy Http while a queued or
running shared-pool lambda still dereferences it.

Fix requirements:

- Every scheduled operation has lifetime state independent of raw Http.
- Http destruction can cancel and wait for all operations using that Http, regardless of executor.
- Waiting never occurs while holding Pool, request-map or callback-visible locks.
- Do not destroy an externally owned ThreadPool as part of Http shutdown.

Regression coverage:

- Queue a request behind blocked shared-pool work, clear the Http Pool, then release the blocker.
- Running and queued variants complete/cancel without UAF under ASAN.
- Callback re-entry does not deadlock.

### A3. Engine stops HTTP/resource producers too late

Current behavior:

Engine clears the global HTTP Pool after textures, Renderer, shaders, framebuffers and vertex buffers
have been destroyed. Callbacks may still mutate placeholders, create textures or queue UI delivery.

Fix requirements:

- Reject new Engine-owned resource deliveries first.
- Cancel/join HTTP/resource-producing operations before scenes and Graphics managers are destroyed.
- Preserve callback lock ordering established by A1/A2.
- Shared application ThreadPools remain externally owned, but no task may retain Engine-owned state
  beyond the shutdown barrier.

Regression coverage:

- Destroy Engine with pending HTTP and decode work.
- Repeat Engine creation/destruction in the same test process.
- Assert no callback touches the destroyed scene/factory and no singleton is recreated.

### A4. UISceneNode static delivery queue lacks shutdown semantics

Current behavior:

Worker/HTTP paths can append main-thread scene deliveries to process-static state. Normal scene
updates drain it, but Engine teardown has no explicit reject/purge boundary.

Fix requirements:

- Define close/reject/purge operations owned by UI lifecycle state.
- Invalidate scene generations before purging captured work.
- Release captured resources on the main/update thread according to project contract.

Regression coverage:

- Queue delivery, destroy scene/Engine before update, recreate Engine, and verify old delivery never
  executes against new state.

## 3. Priority B: deterministic destruction order

### B1. Renderer destroyed before ShaderProgramManager

Current behavior:

Renderer destruction clears `GLi`; ShaderProgram and Shader destructors subsequently call GL delete
through it.

Fix:

- Destroy ShaderProgramManager before Renderer while a valid context is current.
- Audit Renderer-owned raw program views so manager destruction cannot trigger Renderer callbacks.

Regression coverage:

- Create/link programs, destroy Engine, and repeat under ASAN.

### B2. TextLayout cache destroyed after FontManager

Current behavior:

Cached shaped glyphs retain raw FontTrueType pointers. The global TextLayout cache is currently
cleared after FontManager destruction.

Fix:

- Clear TextLayout and related shaped-font caches before FontManager.

Regression coverage:

- Populate shaped-layout cache, destroy Engine, and verify repeated Engine lifecycle under ASAN.

### B3. Scene/global resource manager order

Current behavior:

GlobalBatchRenderer and NinePatchManager are destroyed before SceneManager even though scenes can
retain or submit their resources.

Fix requirements:

- Stop submissions and destroy scenes before global drawable/resource providers they can reference.
- Explicitly flush or discard pending batch state before deleting referenced resources.

Regression coverage:

- Destroy an Engine with live scene widgets using nine-patches and a non-empty batch.

## 4. Priority C: loader and callback lifetime

### C1. TextureAtlasLoader member destruction order

Current behavior:

`ResourceLoader mRL` is declared before callback-visible loader state. C++ destroys members in
reverse declaration order, so that state dies before mRL joins its work.

Fix:

- Add an explicit destructor shutdown/join before any callback-visible member is destroyed, or move
  async operation state into a lifetime object that outlives execution.
- Do not rely only on member declaration order without an explicit invariant comment/test.

Regression coverage:

- Destroy a loader immediately with queued texture tasks and completion callbacks.

### C2. TextureLoader static callback registry is unsynchronized and process-persistent

Current behavior:

TextureLoader callback state is process-static, can be touched by asynchronous loading, and has no
clear test/Engine lifecycle boundary.

Fix requirements:

- Synchronize registration/removal/invocation or constrain all access with an asserted thread
  contract.
- Define reset behavior for repeated Engine tests.
- Never invoke callbacks while holding the callback registry lock.

Regression coverage:

- Concurrent registration/removal/invocation under TSAN.
- Engine recreation does not inherit callbacks from a previous fixture.

## 5. Priority D: existing ownership and GL-handle defects

### D1. Models::Variant copies raw drawable ownership

Current behavior:

Variant copying duplicates the same Drawable pointer and its owning flag. Two Variants can therefore
believe they exclusively own one allocation.

Near-term options:

- Make owning Drawable Variants non-copyable until Stage 4, or deep-clone where a correct clone
  contract exists.
- Never silently convert the second copy to a borrow without documenting its dominating owner.

Final resolution:

- Stage 4 replaces the manual union/owner flag with `std::variant` and DrawablePtr.

Regression coverage:

- Copy/move/reset/destruct every supported drawable Variant ownership mode under ASAN.

### D2. UISkin/StateListDrawable shallow ownership copying

Current behavior:

StateListDrawable stores raw children plus a separate ownership map. UISkin cloning can shallow-copy
child pointers and ownership claims, creating double-delete or shared-mutation behavior.

Near-term fix:

- Prevent ownership duplication during clone and define whether children are deep-cloned or borrowed
  from an explicitly dominant theme owner.

Final resolution:

- Stage 4 uses per-consumer instances and shared immutable source handles.

Regression coverage:

- Clone and destroy skins/state lists in both orders under ASAN.

### D3. Texture copy constructor copies the GL handle

Current behavior:

The protected Texture copy constructor copies `mTexture`. If exercised, two Texture objects can
delete or mutate the same GL handle while otherwise presenting value-copy semantics.

Fix:

- Delete Texture copy construction/assignment unless a real GPU deep-copy operation is explicitly
  required.

Regression coverage:

- Compile-time non-copyability checks.

### D4. FrameBufferFBO reload replaces handles without releasing old objects

Current behavior:

`FrameBufferFBO::reload()` calls `create()` again. `create()` assigns new framebuffer/renderbuffer
handles without an explicit release of the previous objects. Determine whether this is exclusively a
context-loss path where old names are already invalid; if it is callable with a live context, it
leaks GPU objects.

Fix requirements:

- Distinguish context-loss recreation from live-context recreation.
- Release existing live handles before replacement, but never delete names from a lost namespace.

Regression coverage:

- Repeated live-context reload does not increase tracked GL objects.
- Context-loss reload does not attempt invalid deletion.

## 6. Deferred/refactor-bound findings

These are real hazards but are intentionally resolved in their owning migration stage:

- FrameBuffer attachment has conflicting direct/factory ownership: resolved in TexturePtr Stage 2.
- TextureRegion and TextureAtlas depend on factory lifetime: resolved in the complete Stage 2 holder
  cut, not piecemeal.
- Drawable draw-time mutation and false `isStateful()` classifications: resolved by the Stage 4
  source/instance split.
- Global semantic lookup collisions/isolation: resolved by catalogs/scopes in Stage 3.
- Web request partitioning, document leases and coalescing: resolved by WebResourceCache Stage 6.

## 7. Suggested execution order

1. A1 HTTP Pool lock fix and regression test.
2. A2 shared ThreadPool Http lifetime.
3. C1 TextureAtlasLoader destruction order.
4. B1/B2/B3 Engine teardown ordering with lifecycle tests.
5. A3/A4 producer and delivery shutdown barriers.
6. C2 static TextureLoader callbacks.
7. D1/D2/D3/D4 isolated ownership/handle defects.
8. Re-run the Stage 0 source audit, then begin Stage 1 TextureFactory lifetime scaffolding.
