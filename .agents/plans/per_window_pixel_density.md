# Per-window pixel density implementation

Reviewed against `997fe664e04f7d56baed03de0f15b2bfd85a3c04` (Linux/X11, SDL3).
Source design: `eepp_per_window_dpi_implementation-plan.md`, baseline
`2613ec0921e628786f4dcbe8db2b1fa379b0ae47`.

## Current architecture and design corrections

- `UISceneNode::Context` is now the inherited `SceneNode::Context`. Keep that single binding,
  including calls through `SceneNode*`; add density to it rather than restoring a separate UI
  context. Plain scenes use their window density; UI scenes may override/inherit it.
- Window and scene scopes already cover manager update/draw, event dispatch, and asynchronous
  main-thread resource publication. Audit native input callbacks and initialization separately.
- Scene construction must establish the requested window density before `UIThemeManager::New()`.
- `WindowSettings::PixelDensity` and `EEPP_PIXEL_DENSITY` already exist. Application settings
  currently overwrite global density after window creation; pass those settings to every window
  instead. Initialize the legacy fallback only from the first window.
- SDL3 already derives display DPI from continuous content scale. Use
  `SDL_GetWindowDisplayScale()` for window UI scale, preserving fractional values. `getScale()`
  remains the framebuffer/native-coordinate ratio. SDL2 still queries display 0 for DPI: fix the
  index and provide a continuous approximation.
- Existing tests sometimes mutate the process fallback to exercise conversions. That remains
  supported outside a scope; it must not change a scoped scene's density.
- Stylesheets share parsed nodes and media lists. Removing parse-time DPI conversion alone is
  insufficient: media match state must also stay independent between scene stylesheet copies.

## Invariants

Existing static `PixelDensity` conversions use a thread-local scope, falling back to the process
default. Scopes nest, move, and restore without allocations; worker threads inherit no scope.
Density precedence: application override > window override > environment > native scale > 1.
Automatic offscreen/terminal density is 1. Explicit fractional overrides remain exact.
No monitor movement may mutate the process fallback or another scene's resolved resources.

## Work sequence

1. Scoped conversion context, window-owned density, shared scene-context binding, initialization
   scopes, and per-window application settings. Prove simultaneous 1x/2x windows and restoration.
2. UI scene override/inheritance and manual transitions. Reconstruct retained logical geometry,
   padding and margins; synchronize the root from physical viewport dimensions. Stop recursion at
   child scene boundaries and transition each inherited child in its own scope.
3. Fonts before layout: scene-local defaults; provisional scaling of resolved UI font sizes;
   parent-first forced style reapplication without transitions. Preserve physical CSS px and the
   existing HTML px contract; repair media-query evaluation and shared match-state ownership.
4. Preserve normalized scrolling and editor document anchors across wrapping/reflow; stop and
   synchronize transient smooth-scroll state.
5. Coalesce SDL3 display/scale/pixel-size events on Window, resolve final native state after event
   pumping and before scene update. Add SDL2 display-change approximation. Validate native input,
   window size, framebuffer size, scale and density separately on Linux/X11.
6. Audit persistent widget metrics and density-sensitive SVG/icon/raster cache identities.
   Capture density explicitly only for worker jobs that perform density-sensitive work.

## Verification

Use regenerated SDL3 release Ninja unit tests. Native hidden windows on the desktop are the
default; use headless only when needed or to check that runtime. Cover nested/moved scopes,
thread isolation, first-window fallback, settings precedence, independent windows/scenes,
construction-time defaults, media queries, fonts and dp/px/em/rem/inherit. For manual reflow use
1 -> 2 -> 1.25 -> 1 and test geometry, wrapping, layout, scroll and hit testing. Real mixed-DPI
monitor movement and other operating systems require separate platform verification.

## Progress

- Initial audit complete. Rebuilt the stale release test binary and shared library with SDL3
  Ninja; native `--filter='PixelDensity*'` passes all 21 existing tests. This is the baseline,
  not validation of the in-progress implementation.

## Call-site audit follow-ups

- `src/tools/ecode/ecode.cpp`: `App::init()` forces `winSettings.PixelDensity = 1`, then sets the
  configured process density twice after creating the window. With scoped conversions those
  global assignments no longer affect the scene. Move the configured override into window
  creation; keep automatic configuration automatic instead of freezing a quantized display
  density. Audit mapeditor, textureatlaseditor and the UI performance harness for the same legacy
  initialization pattern.
- `Engine::createWindowSettings()`: `pixeldensity=auto` currently resolves display 0's quantized
  density immediately. Preserve zero for auto so the created window can resolve its actual
  display and remain eligible for native updates.
- `UISceneNode::getMediaFeatures()` currently uses ambient `PixelDensity` in a const query. Calls
  targeting another scene must use that scene's density even outside its execution scope.
  `resizeNode()` likewise needs the receiving scene's scope, since a native resize can arrive
  while another window/scene is active.
- `Input::processEvent()` routes by window but does not bind density for raw input callbacks or
  the native resize before the scene dispatcher callback. Cover these window-bound callbacks;
  avoid introducing unnecessary GL context switches for every mouse event.
- `MediaQuery::parse()` reads the current window/display and resolves lengths at parse time.
  Store semantic lengths and resolve against supplied features. Exact `pixel-density` matching
  also uses the integer `val` instead of `fval`, losing fractional values such as 1.25.
- `StyleSheet::operator=()` and `combineStyleSheet()` share `StyleSheetStyle`/`MediaQueryList`
  instances, and `updateMediaLists()` mutates the list's `mUsed`. A second window can therefore
  change which rules the first window sees. Preserve parsed-source sharing while separating
  environment-dependent match state (or deliberately clone only the affected representation).
- Text initialization reads `UITheme::getDefaultFontSize()` in `UITextView`, `UITextInput`,
  `UITextEdit`, `UITooltip`, `UITextSpan`, `UIRichText`, and `UICodeEditor`. A shared theme created
  in the first window retains that window's pixel size. Route effective sizing through the
  receiving scene's theme manager while preserving the theme's logical defaults and existing
  programmatic customization semantics.
- `UISvg::scheduleRasterize()` hands work to a pool; `rasterizeSvg()` calls `pxToDp()` and creates
  density-bearing drawable resources. Capture the submitting scene density by value and install
  a worker-local scope. Main-thread publication already has an owning-scene scope. Keep this
  separate from the later raster invalidation/cache-key work.
