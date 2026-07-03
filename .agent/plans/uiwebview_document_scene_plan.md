# UIWebView Document Scene Isolation Plan

> Status: IMPLEMENTED WITH FOLLOW-UPS - the owned document scene, real scroll-target
> layout widget, viewport/extent split, root-scoped hit-test traversal, and focused
> UIWebView coverage are implemented. Remaining work is future cache/session integration.

## Goal

Make every `UIWebView` own an independent `UISceneNode` that represents its loaded HTML document.
All widgets created from the document, all document styles, URI resolution, navigation interception,
media-query evaluation, dirty style/layout queues, and document resources must use that owned scene
instead of the application `UISceneNode`.

The owned scene remains attached inside the application scene tree so it renders and receives input
as part of the embedding UI. A real layout widget, not the scene node itself, is the scroll target.
This establishes the document boundary needed for multiple independent web views and is the
foundation for future `iframe` support.

This plan should be implemented together with
`.agent/plans/uiwebview_document_scene_layout_refactor.md`, which details the corrected scroll-target
architecture.

## Required Invariants

After implementation:

- The application scene stylesheet never matches elements inside a `UIWebView`.
- A document stylesheet never matches application widgets or another `UIWebView`.
- Loading or navigating one web view never removes or changes another scene's styles, URI,
  referer, navigation callback, dirty queues, or document lookup results.
- Every loaded HTML widget returns the web view's owned scene from `getUISceneNode()` and
  `getSceneNode()`.
- The owned scene is attached below the web view document layout widget but is not registered as a
  top-level `SceneManager` scene.
- The web view document layout widget is the `UIScrollView` scroll target and follows the scrollable
  HTML/body document extent.
- The document scene separately stores the visible web-view viewport size for viewport units, media
  queries, document minimum height, and fixed-position semantics.
- The document scene distinguishes CSS viewport, layout viewport / initial containing block, and
  scrollable overflow extent. Scroll extent must not become the containing block used for normal
  root/body layout.
- Document scroll extent is recomputed only from explicit dirty signals, not by scanning the full
  document every frame.
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

The owned document scene must expose the HTML/body scrollable overflow extent to a real layout-widget
scroll target. For example, a 600px-tall web view displaying a 3000px-tall page needs a 3000px-tall
scroll target so `UIScrollView` can derive the correct scroll range. The scroll target should be a
`UILayout`/`UIWidget` owned by `UIWebView`, not `UISceneNode` itself.

`UISceneNode` currently also uses its own size for viewport-dependent CSS behavior. Once the document
has a separately measured scroll extent, those uses must not read the scroll extent:

- `UISceneNode::getMediaFeatures()` must report the visible web-view viewport.
- `vw`/`vh` and related viewport-relative lengths must resolve against the visible viewport.
- HTML/body minimum height must use the visible viewport.
- Fixed-position layout must remain relative to the web-view viewport while the scroll target is
  translated by scrolling.

The design therefore needs separate metrics:

- **CSS viewport:** the visible `UIScrollView::mContainer` size. Media queries, `vw`/`vh`, fixed
  positioning, sticky positioning, and document minimum height use this size.
- **Layout viewport / initial containing block:** the viewport-sized root layout reference used for
  normal root/body auto-width layout. This prevents a previously measured scroll extent from becoming
  the containing block and keeping responsive content artificially wide or tall.
- **Scrollable overflow extent:** the measured document overflow size used only as the scroll target
  size observed by `UIScrollView`.

The scroll extent participates in scrolling; the CSS and layout viewport are provided by the
embedding `UIWebView`.

## Proposed Scene Tree

```text
application UISceneNode
└── ... application widgets ...
    └── UIWebView
        └── UIScrollView::mContainer              host-scene clipping shell
            └── UIWebView::mDocumentLayout        UILayout scroll target, scroll-extent sized
                └── UIWebView::mDocumentScene     owned UISceneNode, document boundary
                    └── UISceneNode::mRoot         owned scene root, layout-viewport sized
                        └── UIWebView::mDocContainer
                            └── html
                                └── body
                                    └── document content
```

`mDocumentLayout` is added as the `UIWebView` child through the normal `UIScrollView` path.
`UIScrollView::onChildCountChange()` reparents it into the clipped `mContainer` and selects it as the
scroll target. `mDocumentLayout` exposes the measured scrollable extent and is translated when
scrolling.

The visible `mContainer` size is separately passed to `mDocumentScene` as its CSS viewport size.
The document scene root keeps a layout-viewport size unless an explicit API says a caller is setting
the scroll target extent. Every document node, including `mDocContainer`, belongs to the owned scene.

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

### Phase 1: Separate Viewport, Layout Viewport, And Scroll Extent

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
2. Add explicit layout-viewport metrics or an equivalent root-sizing policy for embedded document
   scenes:

   ```cpp
   void setLayoutViewportPixelsSize( const Sizef& size );
   void clearLayoutViewportPixelsSize();
   const Sizef& getLayoutViewportPixelsSize() const;
   ```

   If a separate field is not needed, this can be implemented as a document-scene root sizing policy
   that keeps `UISceneNode::mRoot` viewport-sized while the document layout widget carries the scroll
   target extent. The important invariant is that setting the scrollable extent must not resize the
   root containing block used by normal HTML/body layout.
3. Add a narrow way to disable the nested scene's automatic parent-size following or override root
   sizing for embedded document scenes. Preserve the current default for existing nested scenes;
   `UIWebView` must prevent automatic parent-size following from turning scroll extent into the root
   layout viewport.
4. Make `UISceneNode::getMediaFeatures()` use `getViewportPixelsSize()` for viewport width/height
   while keeping device metrics from the window.
5. Audit viewport-relative length conversion. Replace direct `getSceneNode()->getPixelsSize()` use
   for CSS viewport units with the owning `UISceneNode` viewport metrics.
6. Make HTML/body viewport minimum-height calculations use the document scene viewport metrics.
7. Keep ordinary top-level scene/root sizing unchanged, but for embedded document scenes make root
   sizing use the layout viewport while world bounds and scroll target size use the scrollable extent.
8. Document the split close to `UISceneNode::onSizeChange()` or the new root-sizing policy because
   future changes to scene sizing can easily reintroduce root-as-scroll-extent behavior.

Tests:

- A normal scene without a viewport override preserves existing size-based behavior.
- A 3000px-tall scene with a 600px viewport reports 600px media height and resolves `100vh` to
  600px.
- HTML/body minimum height uses the viewport while the document scroll extent can grow beyond it.
- Changing viewport width re-evaluates media queries without forcing the scroll extent to viewport
  height.
- Growing the scroll extent does not make `html`, `body`, or viewport-width descendants keep a stale
  wide containing block after the web view later shrinks.
- Explicit overflow content can grow the scroll extent while `width: 100vw`, `width: 100%`, and root
  auto-width layout continue resolving against the layout viewport.

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

1. Add `UISceneNode* mDocumentScene` and `UILayout* mDocumentLayout` (or a narrow internal
   `DocumentLayout` subclass if hooks are needed).
2. Construct `mDocumentLayout` as the only direct `UIWebView` scroll child. Let the normal
   `UIScrollView` path reparent it into the clipped `mContainer` and select it as the scroll target.
3. Construct the owned scene with the host window, initialize its embedded services, and parent it
   under `mDocumentLayout` so rendering, hit testing, invalidation, and document-scene ownership flow
   through the normal tree.
4. Create `mDocContainer` under `mDocumentScene->getRoot()`, preserving its current vertical,
   match-width, wrap-content behavior and white initial background.
5. Disable automatic parent-size following for `mDocumentScene` if following would make scene/root
   sizing consume the scroll extent. Prefer an explicit embedded-document root sizing policy that
   keeps root layout viewport-sized.
6. Feed the visible `UIScrollView::mContainer` size into
   `mDocumentScene->setViewportPixelsSize()` and the document root/layout viewport sizing API.
7. Size `mDocumentLayout` to at least the viewport width/height and grow it to the measured
   scrollable overflow extent. This size is the size observed by `UIScrollView` for scrollbar
   calculations, but it must not become the root layout containing block.
8. Add a narrow scene flush API that processes dirty styles, dirty style states, and dirty layouts
   without running actions, scheduled updates, timers, or arbitrary node updates:

   ```cpp
   void UISceneNode::flushDirtyStyleAndLayout();
   ```

   Internally this should reuse the existing dirty queues and invalidation-depth behavior used by
   `UISceneNode::update()`, but stop before `SceneNode::update(elapsed)`. `UIWebView` uses this when
   viewport geometry changes need a synchronous style/layout settlement before measuring scroll
   extent. Do not call `mDocumentScene->update(Time::Zero)` for this purpose.
9. Add a document-extent dirty flag owned by `UIWebView`, for example:

   ```cpp
   bool mDocumentExtentDirty{ true };
   LayoutInvalidationFlags mDocumentExtentDirtyReasons{};
   void markDocumentExtentDirty( LayoutInvalidationFlags reasons );
   ```

   Mark it dirty when viewport metrics change, document children are loaded/closed, external CSS
   applies, images/fonts/replaced controls change intrinsic size, layout invalidation includes
   document/viewport/overflow-affecting reasons, or the scroll target size itself changes.
10. Recompute scrollable extent only when the document-extent dirty flag is set. The recomputation
   sequence should be:

   - update CSS/layout viewport metrics from `mContainer`,
   - if viewport changed, mark root/html/body dirty and invalidate viewport-dependent intrinsic
     widths,
   - call `mDocumentScene->flushDirtyStyleAndLayout()`,
   - measure visible/unclipped scrollable overflow from html/body/document descendants,
   - set only `mDocumentLayout` / scroll-target size to the measured extent,
   - call `UIScrollView::containerUpdate()` / `updateScroll()` as needed,
   - clear the dirty flag only after the measured extent is stable for that pass.

   Avoid full-document extent scans from the normal per-frame path when no dirty signal occurred.
11. Listen for document extent and visible-container changes so scroll-target extent, viewport
   metrics, media queries, scrollbar state, and root/body layout are updated in the correct order
   without layout loops. Treat scrollbar visibility changes as viewport changes only when the scroll
   view type makes scrollbars consume viewport space.
12. Override `scheduledUpdate()` in `UIWebView`, call the inherited
   `UITouchDraggableWidget::scheduledUpdate()` behavior, and then call
   `mDocumentScene->update(elapsed)` exactly once per host-scene frame. `UIScrollView` is already
   subscribed through `UITouchDraggableWidget`; do not add a second subscription or add the document
   scene to `SceneManager`. If document extent is dirty after the update, run the narrow
   dirty-style/layout flush and extent measurement path; do not run a second full scene update.
13. Expose `getDocumentSceneNode()`.
14. On web-view scene changes, reinitialize/rebind the document scene's inherited services without
   touching document-owned state.

Tests:

- The document scene is attached below the host scene but absent from `SceneManager`.
- The `UIScrollView` scroll target is `mDocumentLayout`, not `mDocumentScene`.
- `mDocumentScene` is parented below `mDocumentLayout` and remains the scene owner for descendants.
- `mDocContainer`, `html`, `body`, and descendants all belong to the document scene.
- A page taller than the viewport grows the document layout scroll target and produces the correct
  scroll range.
- Scrolling translates the document layout, including the nested document scene, inside the fixed
  `UIScrollView` viewport.
- `vh`/`vw`, viewport media queries, fixed elements, and sticky elements continue using the visible
  web-view viewport rather than the document scroll extent while the document is scrolled.
- Document actions/animations and dirty style/layout queues update through the web view.
- A viewport change can synchronously settle style/layout through `flushDirtyStyleAndLayout()` but
  does not run actions, timers, scheduled updates, or arbitrary node update callbacks twice in one
  host frame.
- A large stable document does not rescan its full descendant tree every frame when no style, layout,
  viewport, image, font, or replaced-control dirty signal occurred.
- After growing to a very wide viewport and then shrinking, root/body/`width:100vw` descendants
  resolve against the new viewport, while explicit wide overflow still creates horizontal scroll.

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
5. On navigation, close only `mDocContainer` children, remove only nonpersistent rules from the
   document scene, clear document-local author font aliases/resources from the previous document, and
   mark document extent dirty.
6. Update the HTML example's injected Hacker News stylesheet to combine into
   `webView->getDocumentSceneNode()`.
7. Audit document descendants that use `getUISceneNode()` for links, forms, images, inline styles,
   external CSS, fonts, and relative URLs. They should work without special cases once parentage is
   correct.
8. Ensure any document-scoped style injection, including application-provided user CSS, calls the
   document-scene style API and then marks the owning `UIWebView` document extent dirty when layout
   may be affected.

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
- Two web views can load external CSS files with the same selectors and relative URLs without either
  stylesheet, URL base, or late style application leaking into the other document.
- Injecting document CSS through `getDocumentSceneNode()->combineStyleSheet(...)` relayouts and
  remeasures the document extent, while injecting host CSS does not affect document descendants.

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
5. Add an explicit `clearFontFaces()` operation used during
   navigation before new document CSS is loaded. It removes only this scene's internally registered
   author fonts and clears aliases; it must not remove application/system fonts or sibling-document
   author fonts.
6. Remove any remaining scene-owned author fonts during scene destruction.
7. Apply the async scene-lifetime guard from the next phase to remote, local deferred, and VFS
   `@font-face` loads.
8. Mark the owning web-view document extent dirty after a loaded font is registered because font
   metrics can change line wrapping, table sizing, and scroll extent.

Tests:

- Two web views can declare the same family name with different font files and each resolves its own
  font.
- Destroying or navigating one web view does not remove or replace the sibling's author font.
- Navigating one web view clears its previous document-local author font alias and replaces it with
  the new document's face without leaving the old alias visible to CSS.
- Application/system font lookup still works when no document-local face matches.

### Phase 6: Make Navigation And Resource Loading Lifetime-Safe

**Files:**

- `include/eepp/ui/uiwebview.hpp`
- `src/eepp/ui/uiwebview.cpp`
- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- `src/eepp/ui/uiimage.cpp`
- `src/eepp/graphics/drawablesearcher.cpp`
- `src/tests/unit_tests/uiwebview_tests.cpp`

The current async/deferred callbacks can capture raw `this` and raw scene pointers. Owning a scene
makes correct cancellation and stale-response handling part of the document lifecycle. The audit must
cover every deferred document resource path, not only HTTP:

- top-level HTTP document navigation,
- HTTP redirects and cookies,
- external HTTP CSS,
- deferred local-file CSS through the thread pool,
- VFS CSS if it can become deferred later,
- HTTP, local-file, data URI, and VFS `@font-face`,
- async image/replaced-resource loads,
- main-thread reposts created by any of the above.

The browser-engine model to follow is:

- A document owns the resource clients and lifetime tokens.
- Shared caches may outlive the document.
- Network, file, and decode work may finish after navigation or destruction.
- Finished work may mutate stylesheet, font, image, cookie, layout, or widget state only if the
  owning document is still alive and still represents the same navigation generation.
- Cancellation is useful for saving work, but stale-callback rejection is the correctness mechanism.

This must be compatible with the current global `ResourceManager`/`TextureFactory`/`FontManager`
model and with the planned future resource refactor to shared pointers. Do not solve this by adding
heavy ownership to `Node` or by making global managers document-aware. The document-scene resource
context should be a narrow compatibility layer today, and later it should naturally become a set of
`weak_ptr`/`shared_ptr` resource clients once eepp resources stop being lifetime-owned by global
managers.

Also keep the future local cache layer in mind. The lifetime checks below must distinguish
**resource cache lifetime** from **document client lifetime**:

- A cached CSS/font/image response may be reusable by a later document.
- A cached cookie may be preserved by a future browsing-session/domain-cookie store.
- A stale document client must still be unable to apply the cached result or store cookies into the
  wrong document/session.

Current Phase 6 implementation progress:

- Implemented: shared document-scene subresource admission state with atomic owner/alive/generation
  checks and a safe main-thread admission queue that is not scheduled through a possibly destroyed
  document node.
- Implemented: deferred local CSS and remote CSS callbacks are generation-checked before applying
  stylesheets.
- Implemented: remote author `@font-face` callbacks construct/register scene-owned fonts only after
  current-generation main-thread admission.
- Implemented: HTML `img` / `UIImage src` remote loads use document-scene admission and per-widget
  load ids before replacing textures.
- Implemented: CSS background/foreground image URLs loaded through `UINodeDrawable` use
  document-scene admission, scene-unique placeholder textures, and per-layer load ids before
  replacing texture pixels.
- Implemented: stale top-level redirect cookies are ignored after a newer navigation.
- Implemented: explicit mixed-resource destruction coverage exercises pending CSS, font, HTML image,
  and CSS background-image callbacks while the `UIWebView` is destroyed.
- Pending: cache/session integration remains a future layer; current work only preserves the
  document-client lifetime boundary that a cache will need.

#### Phase 6.1: Shared Lifetime Primitive And Main-Thread Admission

1. Keep top-level navigation guarded by `UIWebView` navigation generation state.
2. Keep/add a scene-local document subresource state with:
   - `alive`, cleared during `UISceneNode` destruction;
   - a monotonically increasing generation, incremented on navigation and explicit document resource
     reset.
3. Subresource workers may capture immutable values such as resolved URLs, marker hashes, parsed CSS,
   downloaded bytes, and generation numbers.
4. Subresource workers must not dereference `UISceneNode*`, `UIWidget*`, `Texture*`, or `Font*`
   unless that lifetime is independently guaranteed.
5. All document mutation must happen on the main thread after checking `alive` and generation.
6. Audit `Node::runOnMainThread()` usage carefully. Because it queues an action on a node, using it
   through a possibly destroyed document node is not a complete lifetime guard. Prefer a safe
   scheduler or a host/document lookup that performs the lifetime check before touching the node.

Tests:

- Destroying a web view before any pending subresource callback completes is safe.
- A stale callback can complete without touching a destroyed node, stylesheet, font, texture, widget,
  cookie jar, navigation events, or document extent state.

#### Phase 6.2: CSS Resources

1. For external CSS, resolve URI and marker before dispatch.
2. For deferred local CSS, file IO and parsing may happen off the main thread, but only immutable
   data and parsed stylesheet state may cross the worker boundary.
3. For remote CSS, HTTP callbacks must check the resource generation before accepting the response
   and again before applying it on the main thread.
4. `combineStyleSheet()`, media-query updates, relative URL resolution, font-face processing, and
   document extent dirtying must run only for the current document generation.
5. VFS CSS is currently synchronous, but it should use the same admission helper if it later becomes
   deferred.

Tests:

- A delayed local stylesheet from page A completes after navigating to page B and does not affect B.
- A slow remote stylesheet from an old navigation cannot change the current document's style or
  document extent.
- Destroying a web view with a pending stylesheet load is safe.

#### Phase 6.3: Author Fonts

1. Keep author family names as document-scoped aliases before global font fallback.
2. For remote fonts, do not register a global `FontManager` resource or alias until the current
   generation is admitted on the main thread. `Font` construction currently registers globally, so
   avoid constructing scene-owned fonts on a worker unless that behavior is changed first.
3. Local-file, data URI, VFS, and remote font paths should share the same scene-owned registration
   and cleanup path.
4. Font load completion may call `reloadFontFamily()` and mark document extent dirty only if the
   owning document is current.
5. Navigation/destruction must remove only this document scene's internally registered author fonts.

Tests:

- A stale remote `@font-face` response cannot register an alias or global font resource.
- A stale font response cannot replace the current document's text metrics or dirty its extent.
- Local-file, data URI, VFS, and remote author fonts still clean up on navigation/destruction.

#### Phase 6.4: Images And Replaced Resources

1. Audit HTML `img`, SVG/image widgets, CSS background/foreground images, and any replaced resource
   whose intrinsic size can affect layout.
2. Avoid using bare global URL names as the document lifetime boundary for remote document images.
   Use a document image loader path with scene-unique internal texture/resource names or explicit
   document clients.
3. A remote image response from an old generation must not replace a current document texture,
   repaint a current widget, notify a closed widget, or mark current document extent dirty.
4. Widget resource-change subscriptions must be disconnected before scene-owned resources are
   removed.
5. If a pending HTTP image already owns a placeholder texture, stale completion must either drop the
   decoded image or clean up the placeholder on the main thread without notifying stale document
   clients.
6. This path should be designed so a future cache can store decoded bytes or textures separately
   from document clients. A cache hit is still applied only after document-generation admission.

Tests:

- A slow image response from page A cannot mutate page B after navigation.
- Destroying a web view with a pending image load is safe.
- An image intrinsic-size change from an admitted current resource still invalidates layout and
  document extent.
- The same remote image URL used by two web views does not let one document's cleanup break the
  other's visible resource.

#### Phase 6.5: Redirects, Cookies, And Future Cache Boundary

1. Treat redirects as part of the active navigation or subresource load, not as separate document
   mutations.
2. Progress callbacks that observe redirects must check the current owner/generation before storing
   cookies or continuing work where cancellation is available.
3. Final response callbacks must check generation before storing `Set-Cookie`.
4. Preserve the current per-document cookie behavior for now, but shape the code so a future
   browsing-session/domain-cookie cache can be introduced without weakening stale-response checks.
5. Cache admission and document admission are separate decisions: a stale response may be eligible
   for a future cache, but it must not be applied to or store cookies through a stale document.

Tests:

- A stale top-level redirect cannot store cookies into the current document.
- A stale subresource redirect cannot store cookies or affect the current document.
- A newer navigation supersedes an older redirected load deterministically.

#### Phase 6.6: Destruction And Cross-Resource Cleanup

This phase is cross-cutting and should be implemented partially inside each resource phase above, not
left until the end.

1. On navigation, invalidate document responses and subresources before clearing children/resources.
2. On destruction, mark the resource state dead before destroying children or resource managers.
3. Pending callbacks may finish later, but they must have no live document client to call into.
4. Cleanup order should be: invalidate, disconnect widget/resource clients, clear document children,
   clear document-scoped aliases/resources, then allow late stale callbacks to drop results.
5. When a stale callback is ignored, it must not store cookies, mutate style/font/image state, mark
   extent dirty, send navigation events, or trigger layout.

Tests:

- Destroying a web view with pending CSS, font, and image loads is safe.
- Navigating repeatedly while mixed subresources are pending leaves only the newest document's
  styles, fonts, images, cookies, and extents visible.

### Phase 7: Integration And Documentation

**Files:**

- `.agent/rules/html-layout-architecture.md`
- `src/tests/unit_tests/uihtml_tests.cpp` for realistic fixture coverage where useful
- `src/examples/ui_html/ui_html.cpp`

Steps:

1. Document the web-document scene boundary, viewport/content split, and host-service inheritance.
2. Add a realistic two-web-view integration test with visibly conflicting CSS and independent
   relative resources. **Implemented:** `DocumentScenesIsolateStylesUriAndLookup` loads two
   documents from separate directories that use the same relative stylesheet path and verifies each
   document resolves it through its own URI.
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
| `src/eepp/ui/uiwebview.cpp` | Document layout scroll target, viewport updates, isolated loading/navigation |
| `src/examples/ui_html/ui_html.cpp` | Inject document CSS through the document scene; already uses `getDocumentSceneNode()` |
| `src/tests/unit_tests/uiwebview_tests.cpp` | New focused isolation/lifecycle tests |
| `src/tests/unit_tests/uihtml_tests.cpp` | Existing realistic web-view fixture adjustments |
| `.agent/rules/html-layout-architecture.md` | Documents final document-scene boundary and metrics |

No new production source file is required for the initial implementation. If the embedded-scene
service policy grows beyond the narrow helper above, introduce a dedicated browsing/document context
only when implementing `iframe` or shared browser-session behavior.

## Rejected Approaches

### Keep One UISceneNode And Add Stylesheet Markers

Markers help remove rules but do not isolate selector matching, URI/referer, navigation interception,
media state, dirty queues, cookies, font faces, or DOM lookup. This does not create a document.

### Keep UISceneNode Size As Both Document Extent And CSS Viewport

The document needs a scrollable extent, but that size cannot also define viewport units, media
queries, or the root/body containing block. Keep a real document layout scroll target and explicit
viewport/layout-viewport metrics instead of treating content extent as the viewport.

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

Current verification status:

- Focused `UISceneNode.*` and `UIWebView.*` suites pass through
  `projects/scripts/xvfb-run-eepp`.
- The full native Linux unit-test suite has been run and passes.

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
