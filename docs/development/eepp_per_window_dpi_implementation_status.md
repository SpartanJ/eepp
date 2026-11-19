# Per-window DPI implementation status

Implementation of the **eepp Per-Window Dynamic Pixel Density Implementation Plan**, original
baseline `2613ec0921e628786f4dcbe8db2b1fa379b0ae47`.

## Implemented dynamic-DPI milestone

- **Phase 1:** movable, nested, allocation-free thread-local density guards. Numeric worker scopes
  use a snapshot; window/scene scopes bind live sources so a change inside a callback also updates
  its enclosing scope. The inherited `SceneNode::Context` remains the only scene binding, including
  base pointers and mixed UI/plain scenes.
- **Phase 2:** application override > window override > environment > native scale > 1. Exact
  fractional values are retained. Offscreen/terminal automatic density is 1. Only the first window
  initializes the legacy process fallback; subsequent windows and live transitions leave it alone.
  `getScale()` retains its framebuffer/native-coordinate meaning.
  The automatic policy defaults to desktop UI scale. Opt-in `PhysicalPPI` uses physical monitor
  `max(1, PPI/reference)` (96 by default) on X11, Windows and macOS, with desktop-scale fallback where measurements are unavailable.
  The policy is configurable per window, through `EEPP_PIXEL_DENSITY_MODE`, and in INI/ecode state.
- **Phase 3:** explicit scene overrides, embedded host inheritance and native window inheritance.
  Initializer-list metrics use the target window. Embedded scenes refresh independently and rebind
  native resize callbacks when changing windows. Same-window subtree moves between different
  densities repair geometry and fonts parent-first before applying destination styles.
- **Phase 4:** centralized reflow repairs logical geometry/padding/margins while tracking physical
  setters separately. The root keeps the physical native viewport. Traversal crosses plain nodes
  and stops at child scene boundaries. Forced style resolution is parent-first, includes inline
  properties, and suppresses new transitions without restarting unchanged keyframe animations.
  Anchor distances retain logical sources. The transition snapshots existing nodes, excludes new
  controls initialized under the new density, and removes deleted/rebound nodes from pending work.
  Embedded scenes following parent bounds resize even when their own density override stays fixed.
  Custom logical transform origins follow density in UI scenes, including on plain nodes.
  Explicit pixel origins stay fixed.
  Unchanged resolved visibility, enabled and display declarations preserve runtime state, including
  declarations with CSS variables. Changed media rules still apply.
- **Phase 5:** themes retain logical font sources; each scene theme manager resolves its own
  defaults without modifying shared themes. Text, rich text/spans, inputs/hints, editors, tooltips
  and consoles update before layout. Defaults reconstruct on fractional round trips; other resolved
  UI font sizes scale provisionally before CSS reapplication. Native CSS px remains physical;
  HTML px retains logical scaling. Media lengths resolve against supplied features, and stylesheet
  copies own independent match state while sharing parsed sources. Fractional density and
  DPI/dppx/dpcm resolution matching are covered.
- **Phase 6:** normalized scrollbar values and a wrapped editor document anchor plus fractional
  line offset are captured before reflow and restored after fonts/styles/layout. Transient smooth
  and touch scrolling stops; controllers resynchronize to the final position.
  Widgets capture their own state in `onBeforePixelDensityReflow()` and register one-shot work
  with `UIDensityReflow::restoreAfterLayout()`. The scene cancels callbacks for widgets that close
  or leave it during the transition.
  Editor anchors require a nonempty, nonloading view without pending reconstruction, both before
  capture and after layout. Hidden zero-width wrapped editors can legitimately lack that cache;
  reflow skips anchor lookup until the cache is ready. The editor updates both scrollbar thumbs
  from the restored position and the final scroll range. Font and style changes invalidate cached
  proportional line widths.
- **Phase 7:** display/scale/move/pixel-size events mark pending metrics. Window refresh queries
  final native state after input pumping; Engine processes it before scene updates. Scene updates
  also support legacy loops that pump Input directly. The UI update checks pending window metrics
  once before layout and enters the rebind helper only when work is queued. Environment revisions
  avoid repeated reflow for low-level events and refresh metadata even with fixed UI overrides. SDL3 invalidates
  display enumeration after display events and resolves physical PPI using the window's display ID.
- **Phase 8:** SDL2 DPI lookup uses the requested display index; its window scale uses continuous
  DPI/backing-scale approximation, with Cocoa using backing scale. Display-change events are
  supported where available. SDL2 display events invalidate cached mode and physical PPI data
  before refreshing windows. SDL2 can reuse display indexes, so wrappers stay associated with
  their index slots and refresh their data when the display list changes. Unresolved indexes use
  the window's desktop scale. Movement and size events also trigger refresh. Supported SDL2
  Windows hints request per-monitor-v2 awareness/scaling before video initialization.
- **Phase 9 priorities:** editor gutter/fold metrics, tree indentation/expanders, table icon/drag
  defaults, tab detach resistance, window decoration/shadow layout, and pager drag thresholds.
  Explicit physical metric setters remain fixed; CSS reapplies its original units.
- **Phase 10:** inline SVGs target final physical dimensions, capture worker density,
  rerasterize after reflow and reject obsolete publications. Workers decode into CPU images and
  publish through a weak lifetime handle. The owning UI context uploads a result after checking
  its generation. SVG data URIs and `.svg` file and web URLs use density-specific raster cache keys.
  Deferred file SVG jobs capture the raster scale and upload in the owning scene. HTML intrinsic
  sizing accounts for the texture's raster scale. Glyph, SVG, and system icon caches already
  distinguish requested pixel sizes. Image-viewer layout retains fractional full-size dimensions
  to preserve aspect ratio.
  Image destination caches, list geometry and WebView viewport invalidation receive density hooks;
  WebView document/scrollbar reconciliation continues through its normal scheduled updates.

## Compatibility decisions

`WindowSettings::PixelDensity` historically also scales requested native dimensions on some
platforms. The optional UI override argument to `Engine::createWindow()` keeps application UI
scaling separate from that legacy size behavior. UIApplication passes its setting for every window;
ecode passes its configured scale while retaining saved native dimensions and automatic mode. Its
settings display the effective scale in automatic mode. INI `pixeldensity=auto` stays zero until
creation. Standalone editors and the UI performance harness use window density instead of changing
the process fallback after creation.

ecode's Settings → Appearance → Fonts & Scale → UI Scale Factor updates the window override live.
It accepts 1–6 and persists the chosen manual scale. Select **Auto** under **UI Scale Mode** to
clear the saved factor and apply the configured automatic policy. Existing positive values retain
their effect on upgrade because older profiles used the same key for detected and manual values.
For automatic monitor checks, launch without `--pixel-density` or `EEPP_PIXEL_DENSITY` overrides.

The new framework fields, contexts and window-creation signature change binary layouts/symbols;
rebuild the library and dependent applications together.

## Allocation and layout review

On this Linux x86-64 build, `sizeof` comparisons against the current repository HEAD show:

| Type | Before | After |
| --- | ---: | ---: |
| `WindowSettings` | 96 | 96 |
| `Window` | 768 | 792 |
| `Scene::Node` | 560 | 560 |
| `OriginPoint` | 80 | 80 |
| `Texture` | 208 | 216 |
| `ResourceScope` | 176 | 248 |
| `Engine::WindowContext` | 24 | 32 |
| `SceneNode::Context` | 40 | 64 |
| `UINode` | 824 | 824 |
| `UIWidget` | 1144 | 1144 |
| `UISceneNode` | 4360 | 4456 |
| `UIStyle` | 440 | 440 |
| `UITheme` | 656 | 728 |
| `UIThemeManager` | 176 | 184 |
| `MediaQueryExpression` | 56 | 56 |
| `StyleSheet` | 408 | 480 |
| `UIAbstractTableView` | 2584 | 2584 |
| `UITreeView` | 2688 | 2688 |
| `UITabWidget` | 1792 | 1792 |
| `UISvg` | 1288 | 1320 |

`PixelDensity::Context` is 16 bytes and allocates nothing. Geometry/edge/default-metric flags fit
existing widget padding; logical anchor distances replace the previous same-sized pixel cache.
Reflow snapshots use inline storage for 64 node pointers and eight restore callbacks. Individual
callbacks can store up to 32 bytes of captured state inline when they meet alignment and move
requirements. The transition state is 928 bytes on Linux x86-64, down from 1,064 bytes with the
separate scrollbar and editor arrays. The scene clears its pointer to this stack-owned state on
exit. Stylesheets own a separate active-media set;
this accounts for most scene/theme growth and keeps parsed query sources shared. SVG worker jobs
retain one source copy for asynchronous lifetime, move it across the handoff, and capture density
and generation by value. Async SVG widgets allocate a lifetime state on their first worker job.
The decoded pixel buffer moves into a shared image for the callback handoff. Only accepted results
allocate GPU textures. SVG data-URI hashing adds density incrementally without duplicating the source.
Scene rebinding queues one 16-byte record per pending widget and reuses the queue's storage. The SDL3
display manager retains wrappers until teardown so callers holding a `Display*` keep a valid pointer
after a display is disconnected.
Custom origin provenance uses existing alignment padding. Textures retain one raster scale value,
which adds eight bytes per texture including padding. Deferred file SVG decoding moves the image
buffer to the owning scene for upload and caches textures at each requested density.
Each resource scope tracks in-flight file texture loads by cache key. Matching bitmap requests
share the result, and deferred SVGs use the same lookup for upload and publication. The scope
mutex covers the in-flight map. Decode and upload run after releasing it.

## Verification

The focused regressions cover nested/moved/live bindings, thread isolation, simultaneous windows,
precedence, construction-time metrics, physical setters, repeated `1 → 2 → 1.25 → 1` transitions,
relative/physical/HTML fonts, shared themes/media, animations, scrolling/editor anchors, native
metric delivery, common widget metrics and simultaneous SVG rasters.

The inspector's [density debugging workflow](../articles/ui_inspector.md#debugging-pixel-density-and-embedded-viewports)
documents physical geometry, scene selection, frame barriers, raster timing and the density/source
diagnostics in `ui.contexts`. CSS inspection uses each widget's scene density. A headless ecode launch with an isolated profile, a document and
`--pixel-density 1.25` was inspected successfully.

Final verification:

- Regenerated SDL3 release Ninja build: all Linux release targets built successfully.
- Native release suite ran 2,561 cases. All 2,555
  runnable tests passed, with six UDisks/environment-dependent skips. Rich-text rendering and
  WebView image layout regressions cover the existing relative `file://assets/...` syntax.
- The file texture concurrency test verifies that matching requests share one load and that
  other keys and scopes can finish while that load is blocked.
- The optimized Linux build shows a single window-metrics check in `UISceneNode::update()`.
- Native and headless SDL3 runtimes passed all 73 tests matching `*Density*`. Coverage includes
  custom restores after layout, cancellation for closing, deleted, and rebound widgets, hidden
  wrapped editors, scroll-anchor and thumb preservation, CSS variable changes, logical origins,
  proportional editor widths, file and web SVG raster variants, and SVG destruction while a
  worker job is active. All ten inspector tests passed natively.
- Previous milestone: MinGW SDL3 release unit-test build succeeded; all 59 density-related cases
  passed under Wine. The newer physical-PPI policy/native measurements remain unverified there.
- Current SDL2 display/window/input and physical-PPI sources passed Linux syntax checks against
  SDL2 2.32.8. The current Windows SDL3 display and physical-PPI sources passed MinGW syntax checks.
- Headless ecode at density 1.25: document/editor inspection, a two-frame inspector batch barrier,
  screenshot capture and visual inspection succeeded.
- Native hidden-window ecode with physical-PPI mode and reference 72: fresh welcome and document
  launches passed; live Settings changes through 1.5, 2, 1.25 and 1 passed, including panel centering.
  Reported physical PPI was 108.903 and initial automatic density was 1.51255. Exact fractional
  densities are currently preserved; optional rounding is a policy discussion, not implemented.
- A legacy ecode profile switched from manual 1.75 to physical-PPI Auto, then to 1.33 and back
  to Auto. Auto persisted across relaunch. Injected pointer events verified that the Settings
  panel can be dragged after density reflow. Simulated SDL3 display add/remove events verified
  native display ID lookup and fallback to the window's desktop scale.
- Changed C++ lines were formatted with clang-format 23.1.4; formatting diff and whitespace checks
  are clean. The normal formatting wrapper could not create its temporary index in read-only
  `.git`, so the equivalent changed-line `clang-format-diff` path was used without Git writes.

## Remaining validation and hardening

- Real mixed-density monitor movement and event ordering, especially on Windows and macOS.
  Synthetic events and explicit overrides do not replace this validation.
- SDL2 coordinate/input behavior on real Windows/macOS desktops; its DPI approximation cannot
  guarantee SDL3 parity on every platform/compositor.
- Web SVGs without a `.svg` path can use an explicit `WebResourceRequest::svgScale`. Automatic
  detection of extensionless SVG URLs and sharing response bytes across raster variants are future
  improvements.
- Application and plugin code can retain its own pixel metrics. Those caches need testing in the
  relevant tool or plugin as mixed-density use expands.
- Programmatic font sizes without retained units still use the plan's provisional scaling path;
  CSS and theme defaults reconstruct from their semantic sources.
