# UIWebView Document Scene Isolation Plan

> Status: PROPOSED - investigated against the current `UIWebView`, `UISceneNode`,
> `UIScrollView`, HTML loading, and nested-scene behavior.

## Goal

Make every `UIWebView` own an independent `UISceneNode` that represents its loaded HTML document.
All widgets created from the document, all document styles, URI resolution, navigation interception,
media-query evaluation, dirty style/layout queues, and document resources must use that owned scene
instead of the application `UISceneNode`.

The owned scene remains attached inside the application scene tree so it renders and receives input
as part of the embedding UI. This establishes the document boundary needed for multiple independent
web views and is the foundation for future `iframe` support.

## Required Invariants

After implementation:

- The application scene stylesheet never matches elements inside a `UIWebView`.
- A document stylesheet never matches application widgets or another `UIWebView`.
- Loading or navigating one web view never removes or changes another scene's styles, URI,
  referer, navigation callback, dirty queues, or document lookup results.
- Every loaded HTML widget returns the web view's owned scene from `getUISceneNode()` and
  `getSceneNode()`.
- The owned scene is attached to the application tree but is not registered as a top-level
  `SceneManager` scene.
- The owned scene size follows the scrollable HTML/body document extent.
- The document scene separately stores the visible web-view viewport size for viewport units, media
  queries, document minimum height, and fixed-position semantics.
- Input and focus continue through the embedding application's shared `UIEventDispatcher`.
- Closing or destroying the web view destroys its owned scene and document without leaving async
  callbacks, actions, listeners, or dirty pointers behind.

## Current State

### UIWebView Uses The Application Scene As Its Document

`UIWebView` currently creates `mDocContainer` directly under the web view. `UIScrollView` reparents
that container into its clipped internal container and uses it as the scroll target.

`UIWebView::loadDocumentData()` then uses `getUISceneNode()`, which is the application scene, to:

- remove all stylesheet rules except `mStyleSheetDefaultMarker`,
- set the document URI and referer,
- load the HTML tree into `mDocContainer`,
- install a navigation interceptor.

`onSizeChange()` and `updateHTMLMinHeightForDocument()` also search the entire application scene for
the first HTML and body nodes. With more than one document, those lookups are ambiguous.

This means one navigation can erase application or sibling-document CSS, relative URLs use whichever
document URI was set last, and the last web view to load replaces the global navigation interceptor.

### Nested UISceneNode Support Already Exists

`UISceneNode` is a `SceneNode` and can be nested in a UI tree. The UI editor demonstrates this by
attaching its preview `UISceneNode` below an application widget.

When a `UISceneNode` receives a parent:

- its own root and descendants remain associated with that nested scene,
- it retains its own stylesheet, URI, resource state, and dirty queues,
- it adopts the ancestor UI scene's event dispatcher,
- it follows its direct parent's pixel size.

This is the correct base mechanism, but the UI editor manually registers and updates both scenes.
An owned web-view scene must not require top-level `SceneManager` registration.

### Viewport And Scroll Extent Are Currently Conflated

The owned scene must be the `UIScrollView` scroll target and must grow to the HTML/body document
extent. For example, a 600px-tall web view displaying a 3000px-tall page needs a 3000px-tall
`mDocumentScene` so `UIScrollView` can derive the correct scroll range.

`UISceneNode` currently also uses its own size for viewport-dependent CSS behavior. Once the scene is
content-sized, those uses must not read the scene extent:

- `UISceneNode::getMediaFeatures()` must report the visible web-view viewport.
- `vw`/`vh` and related viewport-relative lengths must resolve against the visible viewport.
- HTML/body minimum height must use the visible viewport.
- Fixed-position layout must remain relative to the web-view viewport while the content-sized scene
  is translated by scrolling.

The design therefore needs separate scene extent and viewport metrics. The scene extent participates
in scrolling; the viewport size is provided by the embedding `UIWebView`.

## Proposed Scene Tree

```text
application UISceneNode
└── ... application widgets ...
    └── UIWebView
        └── UIScrollView::mContainer              host-scene clipping shell
            └── UIWebView::mDocumentScene         owned UISceneNode, content-sized scroll target
                └── UISceneNode::mRoot             owned scene root, content-sized
                    └── UIWebView::mDocContainer   document layout container
                        └── html
                            └── body
                                └── document content
```

`mDocumentScene` is added as the `UIWebView` child through the normal `UIScrollView` path.
`UIScrollView::onChildCountChange()` reparents it into the clipped `mContainer` and selects it as the
scroll target. `mDocumentScene` grows to the document extent and is translated when scrolling.

The visible `mContainer` size is separately passed to `mDocumentScene` as its CSS viewport size.
Every document node, including `mDocContainer`, belongs to the owned scene.

## Ownership And Service Policy

The document boundary should distinguish document-owned state from embedding-platform services.

### Owned Per UIWebView

- `UISceneNode` root and document widgets
- stylesheet, keyframes, media-list state, and document markers
- dirty style, style-state, and layout queues
- URI, referer, and relative-resource resolution
- navigation interceptor
- cookie manager for the initial implementation
- document-scoped `@font-face` aliases and loaded-font tracking
- document actions and scheduled updates

The host stylesheet must never be copied or combined into the document scene.

### Explicitly Inherited Or Shared From The Host Scene

- window and DPI
- shared `UIEventDispatcher`
- thread pool
- color-scheme and contrast preferences
- default font, default font size, and default theme pointer needed by native form controls

Add one narrow `UISceneNode` helper for initializing an embedded scene from its host scene. The
helper must copy only the service/configuration values above; it must not copy stylesheet, URI,
referer, navigation callback, cookies, dirty queues, actions, or roots.

The owned scene's theme manager remains a separate manager. Sharing non-owning font/theme pointers is
acceptable because the document scene is a descendant and is destroyed before the host scene.
Do not copy icon-theme ownership into the document scene because `UIIconThemeManager` owns themes.

Future `iframe` work should replace the per-scene cookie policy with an explicit shared browsing
session/context. That should not block document-scene isolation.

`FontManager` remains an engine-wide resource manager for application and system fonts. Author
`@font-face` family names must be resolved through a document-scoped alias registry before falling
back to that global manager; otherwise two documents declaring the same family name can still
interfere.

## Public API

Add:

```cpp
UISceneNode* UIWebView::getDocumentSceneNode() const;
```

Keep:

```cpp
UIWidget* UIWebView::getDocumentContainer() const;
```

`getDocumentContainer()` continues returning the content container, but it will now belong to the
document scene. Applications that intentionally inject document CSS must use
`getDocumentSceneNode()->combineStyleSheet(...)`, not the application scene.

Keep `setStyleSheetDefaultMarker()` for compatibility, but document that it controls which rules
persist across navigations inside this web view. The default remains the HTML defaults marker.

## Implementation Plan

### Phase 1: Separate UISceneNode Extent From Viewport Metrics

**Files:**

- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- `include/eepp/ui/uinode.hpp`
- `src/eepp/ui/uinode.cpp`
- `src/eepp/ui/uirichtext.cpp`
- focused scene/viewport tests

Steps:

1. Add explicit viewport metrics to `UISceneNode`:

   ```cpp
   void setViewportPixelsSize( const Sizef& size );
   void clearViewportPixelsSize();
   const Sizef& getViewportPixelsSize() const;
   ```

   When no override is set, `getViewportPixelsSize()` returns the scene's own pixel size so existing
   top-level and UI editor behavior remains unchanged.
2. Add a narrow way to disable the nested scene's automatic parent-size following. Preserve the
   current default for existing nested scenes; `UIWebView` disables it because its document scene
   extent is content-driven.
3. Make `UISceneNode::getMediaFeatures()` use `getViewportPixelsSize()` for viewport width/height
   while keeping device metrics from the window.
4. Audit viewport-relative length conversion. Replace direct `getSceneNode()->getPixelsSize()` use
   for CSS viewport units with the owning `UISceneNode` viewport metrics.
5. Make HTML/body viewport minimum-height calculations use the document scene viewport metrics.
6. Keep ordinary scene/root sizing and world bounds based on the actual content-sized scene extent.

Tests:

- A normal scene without a viewport override preserves existing size-based behavior.
- A 3000px-tall scene with a 600px viewport reports 600px media height and resolves `100vh` to
  600px.
- HTML/body minimum height uses the viewport while document/root extent can grow beyond it.
- Changing viewport width re-evaluates media queries without forcing the scene extent to viewport
  height.

### Phase 2: Harden Embedded UISceneNode Behavior

**Files:**

- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- focused nested-scene tests

Steps:

1. Add a helper that initializes an embedded scene from a host `UISceneNode` according to the
   service policy above.
2. Override `UISceneNode::onSceneChange()` so moving an embedding subtree to another application
   scene rebinds the nested scene's shared event dispatcher and inherited host services. Current
   `onParentChange()` alone does not run when only an ancestor changes scene.
3. Preserve the current rule that a nested scene does not own/delete the shared event dispatcher.
4. Add comments documenting that nested scenes need an owner-driven update unless they are
   top-level `SceneManager` scenes.

Tests:

- Widgets below a nested scene resolve `getUISceneNode()` to the nested scene.
- Host and nested stylesheets select only their own descendants.
- Nested media queries use the explicit embedded viewport size, not the window or content extent.
- Reparenting the embedding subtree to another host scene rebinds the dispatcher/services.
- Deleting a nested scene does not delete or corrupt the host dispatcher.

### Phase 3: Give UIWebView An Owned Document Scene

**Files:**

- `include/eepp/ui/uiwebview.hpp`
- `src/eepp/ui/uiwebview.cpp`
- `src/tests/unit_tests/uiwebview_tests.cpp`

Steps:

1. Add `UISceneNode* mDocumentScene`.
2. Construct the owned scene with the host window, initialize its embedded services, and add it as
   the `UIWebView` scroll child. Let the normal `UIScrollView` path reparent it into the clipped
   `mContainer` and select it as the scroll target.
3. Create `mDocContainer` under `mDocumentScene->getRoot()`, preserving its current vertical,
   match-width, wrap-content behavior and white initial background.
4. Disable automatic parent-size following for `mDocumentScene`.
5. Feed the visible `UIScrollView::mContainer` size into
   `mDocumentScene->setViewportPixelsSize()`.
6. Size `mDocumentScene` to at least the viewport width/height and grow it to the laid-out
   HTML/body/document extent. Its size is the size observed by `UIScrollView` for scrollbar
   calculations.
7. Listen for document extent and visible-container changes so scene extent, viewport metrics,
   media queries, and scrollbar state are updated in the correct order without layout loops.
8. Override `scheduledUpdate()` in `UIWebView`, call the inherited
   `UITouchDraggableWidget::scheduledUpdate()` behavior, and then call
   `mDocumentScene->update(elapsed)` exactly once per host-scene frame. `UIScrollView` is already
   subscribed through `UITouchDraggableWidget`; do not add a second subscription or add the document
   scene to `SceneManager`.
9. Expose `getDocumentSceneNode()`.
10. On web-view scene changes, reinitialize/rebind the document scene's inherited services without
   touching document-owned state.

Tests:

- The document scene is attached below the host scene but absent from `SceneManager`.
- `mDocContainer`, `html`, `body`, and descendants all belong to the document scene.
- A page taller than the viewport grows the document scene and produces the correct scroll range.
- Scrolling translates the content-sized document scene inside the fixed `UIScrollView` viewport.
- `vh`/`vw`, viewport media queries, fixed elements, and sticky elements continue using the visible
  web-view viewport rather than the document scene extent while the scene is scrolled.
- Document actions/animations and dirty style/layout queues update through the web view.

### Phase 4: Route All Document Operations To The Owned Scene

**Files:**

- `src/eepp/ui/uiwebview.cpp`
- `src/examples/ui_html/ui_html.cpp`
- relevant tests

Steps:

1. Replace every document-related use of `UIWebView::getUISceneNode()` with `mDocumentScene`:
   stylesheet cleanup, URI/referer, cookie manager, layout loading, relative navigation resolution,
   navigation interception, and HTML/body lookup.
2. Restrict HTML/body lookup to `mDocumentScene->getRoot()` or `mDocContainer`; never search the host
   scene.
3. Keep navigation events emitted by `UIWebView` itself so application listeners remain on the host
   widget.
4. Install the navigation interceptor once during document-scene setup. It must resolve and navigate
   through the owned scene and web view, not replace a host callback on every load.
5. On navigation, close only `mDocContainer` children and remove only nonpersistent rules from the
   document scene.
6. Update the HTML example's injected Hacker News stylesheet to combine into
   `webView->getDocumentSceneNode()`.
7. Audit document descendants that use `getUISceneNode()` for links, forms, images, inline styles,
   external CSS, fonts, and relative URLs. They should work without special cases once parentage is
   correct.

Tests:

- Two web views can load documents with conflicting `body`, class, id, keyframe, and CSS-variable
  names without cross-application.
- Host application CSS does not affect document nodes, and document CSS does not affect host
  widgets.
- Navigating one web view does not change the other's stylesheet, URI, referer, cookies, navigation
  callback, or DOM.
- Relative image, link, form, inline-style, external CSS, and `@font-face` URLs resolve from the
  correct document URI.
- Application-level `findByType(UI_TYPE_HTML_HTML)` no longer returns a web-view document node;
  document-scene lookup does.

### Phase 5: Isolate Author Font Faces

**Files:**

- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- `src/tests/unit_tests/uiwebview_tests.cpp`

`UISceneNode::loadFontFaces()` currently tracks loaded fonts per scene, but creates them with the CSS
family name and `getFontFromNamesList()` resolves through the global `FontManager`. Scene ownership
alone therefore does not isolate two documents that declare the same `@font-face` family.

Steps:

1. Add a document/scene-local font-face alias registry keyed by CSS family, style, and weight.
2. Make `getFontFromNamesList()` consult the local registry before global application/system fonts.
3. Register author fonts under an internal scene-unique resource name if `FontManager` registration
   remains required, while preserving the author-visible family only in the scene-local alias.
4. Keep generic/system fonts and explicitly shared application defaults as global fallbacks.
5. Remove only the scene's internally registered author fonts during scene destruction.
6. Apply the async scene-lifetime guard from the next phase to remote `@font-face` loads.

Tests:

- Two web views can declare the same family name with different font files and each resolves its own
  font.
- Destroying or navigating one web view does not remove or replace the sibling's author font.
- Application/system font lookup still works when no document-local face matches.

### Phase 6: Make Navigation And Resource Loading Lifetime-Safe

**Files:**

- `include/eepp/ui/uiwebview.hpp`
- `src/eepp/ui/uiwebview.cpp`
- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- `src/tests/unit_tests/uiwebview_tests.cpp`

The current async HTTP callbacks capture raw `this` and a raw scene pointer. Owning a scene makes
correct cancellation and stale-response handling part of the document lifecycle. The same audit
must cover async document resources such as `UISceneNode::loadCSS()`, which currently captures the
scene directly.

Steps:

1. Add a small shared navigation/load state containing an alive owner pointer and monotonically
   increasing navigation generation.
2. Async callbacks capture a weak load state, not raw `this` or raw scene pointers.
3. Before storing cookies, posting to the main thread, or applying a response, verify that the web
   view still exists and the generation is current.
4. A newer navigation invalidates older responses. Destruction invalidates all pending callbacks.
5. Keep history mutation and navigation events deterministic when requests fail or are superseded.
6. Give document-scene async resource loads a scene-lifetime guard before they enqueue main-thread
   work or mutate the stylesheet. Audit other document-triggered async loaders and use the same
   pattern where they retain scene/widget pointers.

Tests:

- Destroying a web view before an HTTP response completes is safe.
- A slow old response cannot replace a newer document.
- Stale redirects/cookies do not mutate the current document scene.
- Destroying a web view while external CSS is loading is safe and cannot mutate another scene.

### Phase 7: Integration And Documentation

**Files:**

- `.agent/rules/html-layout-architecture.md`
- `src/tests/unit_tests/uihtml_tests.cpp` for realistic fixture coverage where useful
- `src/examples/ui_html/ui_html.cpp`

Steps:

1. Document the web-document scene boundary, viewport/content split, and host-service inheritance.
2. Add a realistic two-web-view integration test with visibly conflicting CSS and independent
   relative resources.
3. Run the existing old Reddit `UIWebView` smoke test against the new document scene.
4. Verify the UI editor nested scene still behaves correctly after nested-scene hardening.
5. Verify inspector/debug tooling can target either the application scene or
   `getDocumentSceneNode()` explicitly.

## Files Expected To Change

| File | Change Summary |
|---|---|
| `include/eepp/ui/uiscenenode.hpp` | Separate viewport metrics, embedded sizing policy, host-service binding |
| `src/eepp/ui/uiscenenode.cpp` | Viewport media size, nested reparent hardening, document resources |
| `include/eepp/ui/uinode.hpp` | Viewport-relative CSS conversion support if a helper is required |
| `src/eepp/ui/uinode.cpp` | Resolve viewport units from document viewport instead of scene extent |
| `src/eepp/ui/uirichtext.cpp` | HTML/body viewport minimum-height handling |
| `include/eepp/ui/uiwebview.hpp` | Owned scene, getter, scheduled update, lifetime state |
| `src/eepp/ui/uiwebview.cpp` | Content-sized scene scroll target, viewport updates, isolated loading/navigation |
| `src/examples/ui_html/ui_html.cpp` | Inject document CSS through the document scene |
| `src/tests/unit_tests/uiwebview_tests.cpp` | New focused isolation/lifecycle tests |
| `src/tests/unit_tests/uihtml_tests.cpp` | Existing realistic web-view fixture adjustments |
| `.agent/rules/html-layout-architecture.md` | Document final architecture after implementation |

No new production source file is required for the initial implementation. If the embedded-scene
service policy grows beyond the narrow helper above, introduce a dedicated browsing/document context
only when implementing `iframe` or shared browser-session behavior.

## Rejected Approaches

### Keep One UISceneNode And Add Stylesheet Markers

Markers help remove rules but do not isolate selector matching, URI/referer, navigation interception,
media state, dirty queues, cookies, font faces, or DOM lookup. This does not create a document.

### Keep UISceneNode Size As Both Document Extent And CSS Viewport

The document scene must be content-sized to scroll, but that size cannot also define viewport units
and media queries. Keep one content-sized scene and add explicit viewport metrics instead of adding
an extra nested scroll target or treating the content extent as the viewport.

### Add Every Document Scene To SceneManager

This copies the UI editor's manual arrangement but gives a web-view-owned scene top-level lifecycle.
It risks duplicate update/draw and prevents clean ownership for nested web views and future iframes.

### Copy The Host Stylesheet Into The Document Scene

This recreates the original leakage with a snapshot and makes later host-style updates inconsistent.
Only explicit document/user styles belong in the document scene.

## Validation

During implementation, validate each phase before proceeding:

```sh
make -C make/linux -j$(nproc)
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIWebView.*"
projects/scripts/xvfb-run-eepp \
  bin/unit_tests/eepp-unit_tests-debug --filter="UIHTML.redditOldThreadWebViewSmoke"
git diff --check
```

`projects/scripts/xvfb-run-eepp` is the required Linux/FreeBSD test wrapper. It enables automatic
server-number selection, so independent focused test runs may execute concurrently when useful.

Run the full unit suite with:

```sh
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug
```

Run it after the focused isolation, scrolling, positioned-layout, media-query, and nested-scene
tests pass.

## Phase Backup Stashes

Each implementation phase must end with a persistent Git stash backup after that phase's focused
tests pass.

The phase stash is a cumulative snapshot of the complete working tree at that passing phase,
including new untracked source and test files. Immediately restore the snapshot into the working tree
with `git stash apply`, leaving the backup entry permanently present in the stash list:

```sh
git status --short
git stash push --include-untracked -m "uiwebview-document-scene: phase <N> passing"
git stash apply --index stash@{0}
git status --short
git stash list
```

Rules:

- Create the phase stash only after the phase's required focused tests pass.
- Include untracked files so newly added production files, tests, and fixtures are backed up.
- Use `git stash apply --index`, never `git stash pop`, to restore the phase snapshot while
  preserving the stash.
- Never run `git stash drop`, `git stash clear`, or any other operation that removes these phase
  backup stashes.
- Never overwrite or replace a previous phase stash. Every passing phase keeps its own named,
  cumulative backup.
- After restoring a phase stash, verify both that the expected changes remain in the working tree
  and that the named phase stash remains in `git stash list`.
- If a phase stash is restored later, use `git stash apply --index` and keep the source stash entry.
- If applying a stash conflicts, resolve the working tree without dropping or popping the stash.

## Completion Criteria

The design is complete when two `UIWebView` instances can coexist in one application scene with
conflicting documents and independently navigate, style, resolve resources, update, receive input,
and scroll without observable state leakage into each other or the application UI.
