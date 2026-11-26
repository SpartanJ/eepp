# Per-window DPI implementation status

Implementation of the **eepp Per-Window Dynamic Pixel Density Implementation Plan**, original
baseline `2613ec0921e628786f4dcbe8db2b1fa379b0ae47`.

## Implemented dynamic-DPI milestone

- **Phase 1:** movable, nested, allocation-free thread-local density guards. Active and suspended
  window or scene bindings retain a numeric snapshot if their density source is destroyed. Numeric
  worker scopes use a snapshot. Window and scene scopes bind live sources so a change inside a callback also updates
  its enclosing scope. The inherited `SceneNode::Context` remains the only scene binding, including
  base pointers and mixed UI/plain scenes.
- **Phase 2:** application override > window override > environment > native scale > 1. Exact
  fractional values are retained. Offscreen/terminal automatic density is 1. Only the first window
  initializes the legacy process fallback; subsequent windows and live transitions leave it alone.
  `getScale()` retains its framebuffer/native-coordinate meaning.
  The library's default automatic policy uses desktop UI scale. Opt-in `PhysicalPPI` uses physical monitor
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
  and consoles update before layout. Text widgets that track the theme default reconstruct it at the
  new density. Explicit font sizes retain the size and scene density at assignment and rescale from
  that source before CSS reapplication. Native CSS px remains physical;
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
  once before layout and enters the rebind helper only when work is queued. Metadata-only changes
  update scene DPI and media matches. Density reflow runs when a scene's effective density changes.
  SDL3 invalidates display enumeration after display events and resolves physical PPI using the
  window's display ID. System Scale and Physical PPI round to the nearest 0.25 by default. Set
  `EEPP_PIXEL_DENSITY_ROUNDING=0` to use exact automatic values.
- **Phase 8:** SDL2 DPI lookup uses the requested display index. Automatic UI scale uses the
  window's effective DPI on Windows, `Xft.dpi` on X11, and configured display density on Android
  when available. The fallback uses the drawable-to-window ratio or 1. Physical PPI is queried
  separately. Display-change events are supported where available. SDL2 display events invalidate
  cached mode and physical PPI data before refreshing windows. SDL2 can reuse display indexes, so wrappers stay associated with
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
  SVG images and backgrounds rasterize at their drawn size. They start at intrinsic size times
  density; once a background layer, `UIImage` or `UIHTMLImage` knows its destination, it
  requests the source again at that exact size on the next scene update, debouncing later
  resizes by 150 ms. Hacker News vote arrows went from a 48×48 raster drawn at 15×15 to an exact
  15×15 raster at 1.5×. File and data-URI SVGs rasterize on the scene's thread pool and upload
  on the UI thread, sharing the deferred texture-load path with request merging and
  cancellation. Web SVG image entries keep their source, which counts toward the web cache
  budget, so other sizes rasterize from it the same way: one download and one cache entry serve
  every size. The current raster keeps drawing until the new one is ready, and results for
  destroyed consumers or superseded sizes are dropped. Scenes without a thread pool rasterize on
  the UI thread. Exact-size rasters are shared while in use through a weak cache in the resource
  scope, so continuous resizes don't accumulate textures. Rasters keep the SVG's aspect ratio;
  ones that would exceed 4096 pixels per side keep their current resolution. Fit and natural
  sizing use the raster's size at the density, not its drawn size. Consumers allocate the
  helper only for sources that can be SVG, and background layers only once they draw an SVG
  raster.

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
clear the saved factor and apply the configured automatic policy. This also clears any environment
factor on the running window across all displays. The environment can supply a factor again when a
new window opens. Existing positive values retain
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
| `ResourceScope` | 176 | 264 |
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
Each resource scope tracks in-flight file texture loads by cache key. Local file lookups through
`DrawableResolver::resolve()` return the final texture, including when joining an existing load.
`resolveAsync()` returns a cached texture or a blank drawable. Deferred image widgets use this
path. A worker prepares file data for upload on the UI thread. A synchronous UI lookup can
finish that same upload directly. A synchronous worker lookup finishes it only with a shared GL
context; otherwise it returns an empty texture instead of waiting for the UI thread. Preparation
skips cancelled requests, and loaders capture compressed-format support at construction, so
preparation never reads the renderer. The drawable and its clones adopt the completed texture
and notify their consumers. Cached lookups allocate no pending state, and bitmap paths serve as
their own cache keys. Imports, load subscribers and upload dispatchers keep their common counts
inline: two catalogs per scope and one subscriber per load.
Web images use the response format to apply scene density to SVGs, including extensionless URLs.
Confirmed bitmap textures use a density-independent cache key. Both raster keys of a request
share one build of the common fields.

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
  Reported physical PPI was 108.903 and initial automatic density was 1.51255 before the rounding
  policy was added. The same calculation rounds to 1.5 with the default policy.
- A legacy ecode profile switched from manual 1.75 to physical-PPI Auto, then to 1.33 and back
  to Auto. Auto persisted across relaunch. Injected pointer events verified that the Settings
  panel can be dragged after density reflow. Simulated SDL3 display add/remove events verified
  native display ID lookup and fallback to the window's desktop scale.
- Changed C++ lines were formatted with clang-format 23.1.4; formatting diff and whitespace checks
  are clean. The normal formatting wrapper could not create its temporary index in read-only
  `.git`, so the equivalent changed-line `clang-format-diff` path was used without Git writes.

## Review status

- Legacy ecode profiles keep their existing startup scale, including the previous Retina backing
  minimum. Linux and macOS startup checks covered legacy settings and new profiles.
- macOS Physical PPI uses the window's backing pixels for layout. The panel's native PPI remains
  available for diagnostics. Native 1× and 2× backing calculations were checked, and the user
  confirmed both Auto modes during manual testing, including a live scaled-resolution change.
- Unchanged-density display updates refresh DPI and media matches. Embedded viewport changes also
  refresh viewport-dependent styles. Regression tests verify that both paths skip density reflow.
  CSS `resolution` and `pixel-density` use the effective scene density, so an override applies to
  both queries.
- Active and suspended density bindings snapshot the last value when their owner is destroyed.
  When a saved scene or window is gone at scope exit, the enclosing binding follows the live
  target's density. Linux regressions cover this with nested and moved guards, including a live
  density change after unwinding and an independent explicit density scope. The earlier
  restoration tests also passed on macOS and Windows SDL2.
- Stylesheet media queries have per-sheet results, the legacy accessors document their standalone
  semantics, and ecode evaluates media matches for theme inheritance. Theme-manager font overrides
  take precedence, and theme font setters accept an explicit density.
- SDL2/X11 System Scale reads `Xft.dpi` when opening an X11 connection and after display topology
  changes. Window moves reuse the cached result. An XSETTINGS-only desktop uses the drawable
  fallback. Single-axis pixel setters and the settings dropdown null guard are fixed.
- Windows 10 SDL2 passed manual RDP checks at 100% and 125% for live scaling, window placement after
  relaunch, and pointer alignment. The wrapped-editor scroll jump on maximize was fixed, passed the
  native resize regression, and was retested by the user.
- macOS SDL2 passed a Retina → HDMI TV → Retina monitor move. The checkbox gap tracked
  8 → 4 → 8 pixels, and the test tab width returned to 184 pixels. The Welcome screen version and
  shortcuts survived a separate live scale change. Linux passed the 90 density and 127 tab-filtered
  tests after the widget fixes, with overlap between the filters.

## First-run fonts and per-display overrides

- ecode, eefm, eproc, eterm, and eeiv use their established UI font defaults. Set
  `EEPP_USE_SYSTEM_UI_FONT_SIZE=1` to seed missing UI sizes from the OS. Saved sizes take
  precedence. The flag also covers ecode's panel font. Editor and terminal fonts retain their
  own defaults.
- **Per-display Scale** lets users choose a connected display and save an exact factor. Each
  override applies in every Auto mode and can be cleared independently. Global Manual and
  environment overrides still take precedence. The Linux picker check saved an override for
  the second display from the first and verified that Cancel discards edits for both. Selecting
  Manual starts from the current effective scale, including an active per-display value.
- Windows identifies the monitor from the native window and its monitor interface. macOS uses
  the display UUID. X11 uses the connector and the monitor's EDID, so two monitors with the same
  EDID can keep distinct settings on their respective connectors. Moving a monitor to a different
  port or using a dock that changes the output name requires setting the override for that
  connector. Wayland uses SDL's display label when it is meaningful and unique among connected
  displays. A different label, such as after a compositor or port change, needs a new override.
  Displays without a usable key use the selected Auto policy. The picker excludes entries with
  shared keys.
- The five Linux SDL3 apps built. The native suite passed 2,583 tests with six UDisks environment
  skips. Native X11 checks covered moving between two displays, separate exact factors, restart,
  and resetting either display independently. Simulated duplicate names, disconnect, reconnect,
  and reorder also passed. With a GTK preference of 12pt, a fresh ecode profile kept its
  established 11dp defaults and an opt-in profile seeded 12dp. The opt-in profile kept its
  saved 12dp when the GTK preference changed to 14pt.
- The Android ecode APK built and ran on the Pixel_9_API_35 emulator. SDL2 System Scale now uses
  160 dpi as Android's 1× baseline. Auto reported 2.625 at 420 dpi and rounded to 2.75. At
  320 dpi it reported 2.0. Settings stayed reachable after reflow, and Physical PPI used the
  System Scale fallback. With system font seeding enabled and Android font scale set to 1.3,
  an existing ecode profile kept 11dp
  and a fresh profile seeded 14.3dp. The emulator's original density and font scale were restored.
  Android uses the app-private directory for user-directory lookups, which fixes startup when
  `HOME` is unset.

- Windows RDP saved separate factors for both monitors and kept the open Settings panel centered
  when the window moved to a smaller viewport at the same UI scale. The macOS picker showed
  3024 × 1964 for the Retina panel and 1920 × 1080 for the external display. The user confirmed
  that a saved override was restored after physically disconnecting and reconnecting a monitor.
- The opt-in X11 font lookup holds the server grab while reading the XSETTINGS owner and its
  property. It flushes the ungrab before parsing the font preference.

## Auto (Best Fit)

Best Fit is the default automatic mode for all windows, including standalone examples. A recognized
`EEPP_PIXEL_DENSITY_MODE` can select another mode for windows using `Default`. A saved explicit mode
or fixed scale takes precedence.

Best Fit uses the native scale on Windows, Wayland, Android, and iOS. On Emscripten it rounds
the browser scale to the nearest quarter step by default. On macOS it uses the
current backing pixels per inch. On X11 it honors a whole-window scale above 1 when available.
Otherwise the physical estimate competes with the desktop's logical DPI divided by 96.
The estimate divides pixels per inch by `clamp(100 - 4 * (diagonalInches - 20), 60, 100)`,
then clamps the result to 1–6. Estimated scales round to the nearest quarter step, with
`EEPP_PIXEL_DENSITY_ROUNDING=0` preserving the calculated value. Missing or implausible physical
measurements fall back to System Scale.

## Lower-priority follow-ups

- Sharing web SVG response bytes across raster variants remains a cache optimization. Application
  and plugin pixel caches may also need their own regression coverage.
