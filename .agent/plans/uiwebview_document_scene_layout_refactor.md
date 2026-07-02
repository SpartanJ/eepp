# UIWebView Document Scene Layout Refactor Plan

> Status: REVISED - use a real layout widget as the scroll target, while keeping the
> owned `UISceneNode` nested under that widget as the document boundary.
>
> This supersedes the earlier "make `UISceneNode` behave as a layout" attempt and
> corrects the overcorrection where the scene was removed from the widget tree entirely.

---

## Lessons From Failed Attempts

The first approach tried to make `UISceneNode` participate directly in the layout system:

1. Override `isLayout()` on `UISceneNode`.
2. Give it layout policies such as `MatchParent` width and `WrapContent` height.
3. Let the scene root (`mRoot`, a `UIRoot`) act as the document container.

That approach hit real class-hierarchy dead ends.

### Dead End 1: UISceneNode Is Not A UIWidget

`UISceneNode` inherits from `SceneNode`. `UILayout` inherits from `UIWidget`. Returning
`true` from `isLayout()` does not make layout APIs available: `updateLayoutTree()`,
`setInternalPixelsSize()`, layout margins, padding, and size policies are widget/layout
contracts that `SceneNode` does not implement.

### Dead End 2: UIRoot Is Not A UILayout

Making `mRoot` the document container also fails. `UIRoot` extends `UIWidget`, not
`UILayout`; it does not auto-pack children. Once content grows, manual size propagation
is still required.

### Dead End 3: Scroll Extent Became Layout Viewport

The document has at least three separate metrics:

- **CSS viewport:** the visible `UIScrollView::mContainer` size. Media queries,
  `vw`/`vh`, fixed positioning, sticky positioning, and document minimum height use it.
- **Layout viewport / initial containing block:** the viewport-sized reference used by
  root/body auto-width layout and percentage descendants.
- **Scrollable overflow extent:** the measured document overflow size used by
  `UIScrollView` for scroll ranges.

When the scene itself is the scroll target, `UISceneNode::onSizeChange()` naturally
resizes `mRoot` to the scene extent. That makes scroll extent become the containing
block, so a previous wide document can keep responsive content artificially wide after
the web view shrinks.

### Dead End 4: Removing Viewport Style Invalidation Breaks CSS Units

Viewport-dependent style values such as `height: 100vh` and media query results must
recompute when the viewport changes. A full `reloadStyle(true, true, true)` is expensive,
but removing viewport invalidation entirely is incorrect. The implementation needs a
targeted viewport/style/layout dirty path.

### Dead End 5: Removing Event-Driven Viewport Sync Causes Multi-Step Resize

Container size and scrollbar visibility changes must be observed promptly. Relying only
on a later scheduled update can produce visible intermediate states at the old viewport
width. The fix is not per-frame polling; it is a dirty, synchronous style/layout flush
for viewport geometry changes that does not run actions or arbitrary node updates.

### Dead End 6: Scene Outside The Widget Tree Breaks Ownership

The inverted plan originally proposed a scene owned internally by a layout widget but
not present in the widget tree. That avoids `UISceneNode` as scroll target, but creates
new hard problems:

- document widgets no longer naturally return the document scene from `getUISceneNode()`
  and `getSceneNode()`;
- rendering must be manually forwarded or redirected;
- input hit testing must be manually bridged;
- focus and invalidation need custom routing;
- inspector/debug traversal becomes special-case heavy.

That is too much new machinery for the first correct `UIWebView` implementation.

---

## Corrected Architecture

Use a real layout widget as the scroll target, but keep the document scene nested in
the widget tree below that scroll target.

```text
application UISceneNode
└── ... application widgets ...
    └── UIWebView
        └── UIScrollView::mContainer                host-scene clipped viewport
            └── UIWebView::mDocumentLayout          UILayout scroll target, scroll-extent sized
                └── UIWebView::mDocumentScene       owned UISceneNode, document boundary
                    └── UISceneNode::mRoot           root sized by layout viewport policy
                        └── UIWebView::mDocContainer document layout container
                            └── html
                                └── body
                                    └── document content
```

### Key Decisions

1. **`mDocumentLayout` is the scroll target.** It is a normal layout/widget child of
   `UIWebView`, so `UIScrollView::onChildCountChange()` reparents it into
   `mContainer` and uses it as `mScrollView`.

2. **`mDocumentScene` remains in the widget tree.** It is a child of
   `mDocumentLayout`, not a direct child of `UIWebView`. This preserves rendering,
   hit testing, invalidation propagation, focus routing through the shared dispatcher,
   and the core invariant that document widgets resolve to the document scene.

3. **`mDocumentLayout` owns scroll extent.** Its pixel size is the size observed by
   `UIScrollView` for scrollbar calculations. It must be at least the visible viewport
   and may grow from measured overflow.

4. **`mDocumentScene` owns document state.** Stylesheet, URI, referer, cookies,
   navigation callback, dirty queues, actions, keyframes, and author font aliases remain
   scene-local.

5. **The scene root must not inherit scroll extent as its layout viewport.** The scene
   may need a scroll-target size for world bounds and hit testing, but `mRoot`/HTML/body
   layout must use the layout viewport, not the measured overflow extent. Implement this
   with explicit viewport/layout-viewport metrics or a root-sizing policy on embedded
   document scenes.

6. **Manual extent measurement remains, but it is dirty-driven.** Horizontal overflow,
   images, fonts, tables, fixed-width content, and replaced controls still need measured
   scroll extent. The difference is that extent measurement is tied to explicit dirty
   signals, not a full descendant-tree scan every frame.

---

## Why This Architecture Is Better

| Problem | Failed scene-as-layout approach | Corrected approach |
|---|---|---|
| `UISceneNode` is not a layout | Tried to make it act like `UILayout` | `mDocumentLayout` is the layout/scroll target |
| `UIRoot` is not a layout | Tried to make root auto-pack content | `mDocContainer` remains a real `UILinearLayout` |
| Scroll extent vs viewport | Scene size resized root to extent | Layout viewport and scroll extent are separate |
| Document isolation | Owned scene provided isolation | Owned nested scene still provides isolation |
| Rendering/input | Worked because scene was in tree | Still works because scene remains in tree |
| Resize responsiveness | Needed event sync | Uses event dirtying plus narrow style/layout flush |
| Performance | Tended toward per-frame scans/cascades | Dirty-driven extent measurement |

---

## Current Implementation Status

### Implemented

- `UIWebView` now creates `mDocumentLayout` as the real `UIScrollView` scroll target.
- The owned `mDocumentScene` is nested under `mDocumentLayout`, so document widgets
  still resolve to the document scene while normal scroll-view clipping and scrolling
  operate on a widget/layout node.
- `UISceneNode` has explicit CSS viewport and layout viewport metrics. Embedded
  document scenes can keep `mRoot` viewport-sized while the scene/layout scroll extent
  grows to the measured document overflow.
- `UISceneNode::flushDirtyStyleAndLayout()` exists as the narrow dirty flush. It
  processes dirty styles, style states, and layouts without running actions, timers,
  scheduled updates, or a full scene update.
- `UIWebView` has dirty-driven document metric updates through
  `markDocumentExtentDirty()` / `updateDocumentMetricsIfNeeded()`.
- Viewport changes are coalesced into the scheduled update path. The CSS viewport is
  derived from current web-view geometry and scrollbar visibility, not from queued
  `UIScrollView::mContainer` size updates.
- Scroll extent measurement resizes `mDocumentLayout` and `mDocumentScene`, while
  root/html/body layout remains viewport-sized.
- Hit testing now uses a root-scoped traversal extent instead of a
  `UISceneNode::overFind()` compatibility override. `UIRoot` keeps its layout/self-hit
  bounds viewport-sized, but embedded document scenes can ask it to traverse child
  hit testing through the measured document extent.
- Author `@font-face` isolation and cleanup are implemented. Scene-local aliases resolve before
  global font fallback; WebView navigation clears the document scene's previous author aliases and
  internally registered font resources; document scene destruction removes any remaining scene-owned
  author fonts. Tests cover sibling-scene isolation, navigation cleanup, and WebView destruction
  cleanup.
- Tests cover the new topology, viewport-vs-extent behavior, scrolling, two-scene
  style isolation, navigation supersession, and a resize metric regression that guards
  against no-op queued viewport churn rebuilding RichText. They also cover document
  root hit testing below the layout viewport.

### Pending / Follow-Up

- **Subresource lifetime coverage** should be completed for every async path described
  in Phase 6, including deferred CSS, fonts, images, redirects, cookies, and destruction.
- **Example and documentation integration** should be completed after the code shape
  settles, especially `.agent/rules/html-layout-architecture.md` and the HTML example
  stylesheet injection path.
- **Fixed/sticky positioning coverage** is still listed as required test coverage for
  the final architecture. Existing viewport tests cover the core sizing behavior, but
  fixed/sticky document behavior should remain an explicit acceptance item.

---

## Required Supporting API

### UISceneNode Viewport And Root Sizing

Keep or add explicit viewport metrics:

```cpp
void setViewportPixelsSize( const Sizef& size );
void clearViewportPixelsSize();
const Sizef& getViewportPixelsSize() const;
```

Add one of the following:

```cpp
void setLayoutViewportPixelsSize( const Sizef& size );
void clearLayoutViewportPixelsSize();
const Sizef& getLayoutViewportPixelsSize() const;
```

or an equivalent embedded-document root sizing policy. The invariant is more important
than the exact API: setting the scrollable extent must not resize `mRoot` into the
normal-flow containing block.

### Narrow Dirty Flush

Add a narrow flush API:

```cpp
void UISceneNode::flushDirtyStyleAndLayout();
```

It should process dirty styles, style states, and layouts using the same invalidation
depth semantics as `UISceneNode::update()`, but it must not call `SceneNode::update()`
and must not run actions, timers, scheduled updates, or arbitrary node update callbacks.

`UIWebView` uses this after viewport geometry changes and before measuring scroll
extent.

### Document Extent Dirtying

Add dirty state owned by `UIWebView`:

```cpp
bool mDocumentExtentDirty{ true };
LayoutInvalidationFlags mDocumentExtentDirtyReasons{};
void markDocumentExtentDirty( LayoutInvalidationFlags reasons );
void updateDocumentMetricsIfNeeded();
```

Mark extent dirty when:

- `mContainer` size changes;
- scrollbar visibility changes and the scroll view type consumes viewport space;
- document children are loaded or closed;
- inline or external CSS applies;
- media queries change;
- images/replaced controls change intrinsic size;
- author fonts load;
- layout invalidation includes document, viewport, or overflow-affecting reasons;
- table/flex/grid/rich-text layout reports size-affecting changes.

Do not recompute document extent from the normal scheduled update path unless this flag
is set.

---

## Implementation Plan

### Phase 1: Add DocumentLayout Scroll Target

**Files:**

- `include/eepp/ui/uiwebview.hpp`
- `src/eepp/ui/uiwebview.cpp`

Steps:

1. Add `UILayout* mDocumentLayout` or a narrow internal subclass if a hook is needed.
2. Construct `mDocumentLayout` as the only direct scroll child of `UIWebView`.
3. Give `mDocumentLayout` `MatchParent` width and `WrapContent` height defaults.
4. Move `mDocumentScene` under `mDocumentLayout`, not directly under `UIWebView`.
5. Keep `mDocContainer` under `mDocumentScene->getRoot()`.
6. Ensure `mDocContainer`, `html`, `body`, and descendants still return
   `mDocumentScene` from `getUISceneNode()` and `getSceneNode()`.

Tests:

- `UIScrollView::getScrollView()` returns `mDocumentLayout`.
- `mDocumentScene->getParent()` is `mDocumentLayout`.
- Document descendants belong to the document scene.
- The document scene is not registered with `SceneManager`.

### Phase 2: Separate Viewport, Layout Viewport, And Scroll Extent

**Files:**

- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- `src/eepp/ui/uinode.cpp`
- `src/eepp/ui/uiwebview.cpp`

Steps:

1. Keep `getViewportPixelsSize()` for media queries and `vw`/`vh`.
2. Add layout-viewport/root-sizing support so `mRoot` and normal root/body layout use
   the visible viewport even when scroll extent is larger.
3. Update `UINode::convertLength()` so viewport units resolve through the document
   scene viewport.
4. Update HTML/body minimum-height handling to use the CSS viewport.
5. Ensure fixed/sticky positioning uses the visible scroll viewport, not measured
   document extent.

Tests:

- `100vh` resolves to the visible viewport height while scroll extent is much taller.
- After growing wide and shrinking, `html`, `body`, `width:100vw`, and `width:100%`
  content resolve to the new viewport.
- Explicit fixed-width overflow still creates horizontal scroll.
- `position: fixed` remains pinned to the web-view viewport while scrolling.
- `position: sticky` uses the web-view viewport while scrolling.

### Phase 3: Add Narrow Dirty Flush And Dirty-Driven Extent Measurement

**Files:**

- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`
- `include/eepp/ui/uiwebview.hpp`
- `src/eepp/ui/uiwebview.cpp`

Steps:

1. Add `UISceneNode::flushDirtyStyleAndLayout()`.
2. Add `UIWebView::markDocumentExtentDirty()`.
3. Add `UIWebView::updateDocumentMetricsIfNeeded()`.
4. On viewport geometry changes, update scene viewport/layout viewport, dirty
   viewport-dependent document layout, flush style/layout, measure overflow, resize
   `mDocumentLayout`, then update scrollbars.
5. In `scheduledUpdate()`, call `UITouchDraggableWidget::scheduledUpdate(time)`, then
   update the document scene exactly once. After that, call
   `updateDocumentMetricsIfNeeded()` only if dirty.
6. Remove any use of `mDocumentScene->update(Time::Zero)` for synchronous layout
   settlement.
7. Avoid full descendant-tree extent scans when no dirty signal occurred.

Tests:

- Viewport change settles in one frame without a second full scene update.
- Actions/timers/scheduled node updates are not run by `flushDirtyStyleAndLayout()`.
- A stable large document does not remeasure extent every frame.
- Hidden and clipped wide descendants do not create horizontal scroll.

### Phase 4: Route Document Operations Through The Owned Scene

**Files:**

- `src/eepp/ui/uiwebview.cpp`
- `src/examples/ui_html/ui_html.cpp`

Steps:

1. Use `mDocumentScene` for stylesheet cleanup, URI/referer, cookie manager,
   navigation interception, document loading, relative URL resolution, and HTML/body
   lookup.
2. On navigation, close only `mDocContainer` children.
3. Remove only document-scene nonpersistent stylesheet rules.
4. Clear previous document-local author font aliases/resources.
5. Mark document extent dirty after navigation and after document CSS injection.
6. Install the document navigation interceptor once during scene setup, not on every
   document load.
7. Update examples to combine document CSS into
   `webView->getDocumentSceneNode()`.

Tests:

- Two web views with conflicting selectors, IDs, CSS variables, keyframes, and
  `@font-face` names remain isolated.
- Navigating one web view does not mutate the other web view or the host scene.
- Application-level HTML lookup does not find web-view document nodes.
- Document-scene lookup does find document nodes.
- External CSS with relative URLs resolves from the correct document URI.

### Phase 5: Author Font-Face Isolation And Cleanup

**Files:**

- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`

Steps:

1. Add a scene-local font-face alias registry keyed by CSS family, style, and weight.
2. Register loaded author fonts under scene-unique internal names.
3. Resolve author font aliases before global `FontManager` fallback.
4. Add `clearFontFaces()` and call it during navigation.
5. Remove only this scene's internally registered author fonts during scene destruction.
6. Mark document extent dirty after a font load can affect metrics.

Tests:

- Two documents can declare the same family name with different files.
- Navigating one document clears and replaces only its own author font alias.
- Destroying one web view does not remove sibling fonts.
- Global application/system font fallback still works.

### Phase 6: Lifetime-Safe Navigation And Subresources

**Files:**

- `include/eepp/ui/uiwebview.hpp`
- `src/eepp/ui/uiwebview.cpp`
- `include/eepp/ui/uiscenenode.hpp`
- `src/eepp/ui/uiscenenode.cpp`

Steps:

1. Add navigation generation state for top-level document loads.
2. Add scene-local async resource generation state for subresources.
3. Invalidate document responses and subresources on navigation.
4. Invalidate everything on destruction.
5. Apply generation checks before cookies, style mutation, font registration, image
   mutation, main-thread repost mutation, navigation events, or extent dirtying.
6. Cover HTTP, redirects, external CSS, deferred local CSS, VFS CSS, remote/local/data
   fonts, and async image/replaced resources.

Tests:

- A stale document response cannot replace a newer document.
- Stale redirects/cookies do not mutate the current document.
- Deferred local CSS after navigation/destruction is ignored safely.
- Old `@font-face` and image responses cannot mutate current document state.

### Phase 7: Cleanup, Documentation, And Integration

**Files:**

- `.agent/rules/html-layout-architecture.md`
- `.agent/plans/uiwebview_document_scene_plan.md`
- `src/tests/unit_tests/uiwebview_tests.cpp`
- `src/tests/unit_tests/uihtml_tests.cpp`

Steps:

1. Document the final tree and the three document metrics.
2. Remove obsolete viewport/extent sync methods and state booleans.
3. Keep only event listeners that mark viewport/extent dirty.
4. Update tests that assumed the document scene itself was the scroll target.
5. Verify inspector/debug tooling can target either the host scene or
   `getDocumentSceneNode()`.

---

## Files Expected To Change

| File | Change |
|---|---|
| `include/eepp/ui/uiwebview.hpp` | Add document layout scroll target, dirty extent state, lifetime state |
| `src/eepp/ui/uiwebview.cpp` | Use layout widget as scroll target; route document operations to owned scene; dirty-driven extent measurement |
| `include/eepp/ui/uiscenenode.hpp` | Viewport/layout viewport metrics, narrow dirty flush, resource lifetime helpers |
| `src/eepp/ui/uiscenenode.cpp` | Root sizing policy, dirty flush, author font aliases, guarded subresource callbacks |
| `src/eepp/ui/uinode.cpp` | Resolve viewport units from document viewport |
| `src/examples/ui_html/ui_html.cpp` | Inject document CSS through document scene |
| `src/tests/unit_tests/uiwebview_tests.cpp` | Scroll target, isolation, viewport, fixed/sticky, performance, lifecycle tests |
| `src/tests/unit_tests/uihtml_tests.cpp` | Realistic smoke/regression fixture adjustments |
| `.agent/rules/html-layout-architecture.md` | Final architecture documentation |

---

## Validation

```sh
make -C make/linux -j$(nproc)
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIWebView.*"
projects/scripts/xvfb-run-eepp \
  bin/unit_tests/eepp-unit_tests-debug --filter="UIHTML.redditOldThreadWebViewSmoke"
git diff --check
```

Run the full unit suite after focused viewport, scrolling, positioning, resource, and
two-web-view isolation tests pass:

```sh
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug
```

Current verification status:

- Focused `UISceneNode.*` and `UIWebView.*` suites pass through
  `projects/scripts/xvfb-run-eepp`.
- The full native Linux unit-test suite has been run and passes.

---

## Completion Criteria

Two `UIWebView` instances can coexist in one application scene with independent DOM,
styles, URI/referer, navigation, cookies, font faces, resources, input, focus, layout,
and scrolling.

The scroll target is a real layout widget. The owned document scene remains nested as
the document state boundary. CSS viewport, layout viewport, and scrollable overflow
extent are distinct. Viewport changes settle through a narrow style/layout flush, not a
second scene update. Stable documents do not rescan full content every frame.
