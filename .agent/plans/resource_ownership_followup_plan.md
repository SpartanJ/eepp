# Resource ownership follow-up plan

Status: proposed follow-up after Stage 7 completion, 2026-07-24.

The shared-resource ownership migration is complete. This plan is intentionally limited to
validation, documentation, diagnostics, and compatibility-era naming cleanup. It must not reopen
the established `ResourceScope` / `ResourceCatalog` ownership model without a concrete defect.

## 1. Scene lifetime coverage

- Add focused tests proving resources published only into a scene scope are released when its
  `UISceneNode` and `ResourceScope` are destroyed.
- Cover textures, drawables, fonts, themes, icons, and shader programs where practical.
- Verify explicitly imported catalogs remain alive only through their actual external owners.
- Assert against registry/catalog contents and retained handles, not only destructor side effects.

## 2. Public ownership documentation

- Audit public resource APIs and consistently document whether parameters and return values are:
  owning, retaining, borrowing, or observing.
- Document the required lifetime for borrowed raw pointers returned by UI and GPU APIs.
- Keep hot-path raw pointers where ownership is established elsewhere; do not imply ownership by
  converting those APIs to shared handles.
- Add short ownership examples to `ResourceScope`, `ResourceCatalog`, and family-specific services.

## 3. Compatibility-era naming cleanup

- Rename non-owning `ShaderProgramManager`, `VertexBufferManager`, and `FrameBufferManager`
  concepts to `ShaderProgramRegistry`, `VertexBufferRegistry`, and `FrameBufferRegistry` throughout
  filenames, includes, build files, and documentation.
- Review other `*Manager` names only when their current role is genuinely a registry or service.
- Keep this as an isolated public API cleanup so downstream include breakage is easy to review.

## 4. GPU borrowed-lifetime diagnostics

- Add debug-only validation that borrowed frame buffers, vertex buffers, shaders, and programs are
  not used after their owning OpenGL context or renderer has been destroyed.
- Prefer cheap generation/context identity checks at API boundaries over reference counting in hot
  rendering paths.
- Do not add OpenGL context-loss recreation support; current supported platforms do not require it.

## 5. Static initialization audit

- Build a Clang diagnostic configuration with `-Wglobal-constructors` and
  `-Wexit-time-destructors` to identify C++ work performed before `main()` and after its return.
- Produce a linker-level inventory of `.init_array` entries for representative executables to find
  constructors hidden in libraries or translation units excluded from Clang diagnostics.
- Prioritize globals that allocate memory, register callbacks/resources, depend on singleton order,
  or retain graphics objects. Constant-initialized POD data is not a migration target.
- Move executable state into `main()` scopes and callback lambdas. Replace necessary library
  globals with function-local statics only when process lifetime is intentional and documented.
- Track the baseline count and prevent new non-trivial global constructors in CI once existing
  cases have been classified.

## Validation

- Run the complete unit-test suite and all normal platform CI jobs after each focused change.
- Keep `git diff --check` clean and run examples affected by shutdown-order changes under ASan.
- Confirm GPU/resource handles are destroyed before `Engine::destroySingleton()` in examples and
  tools that own them locally.

