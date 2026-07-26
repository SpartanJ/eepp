# Subpixel Text Rendering Plan

Status: implemented after maintainer approval; review fixes completed on 2026-07-25.

Date: 2026-07-25

## Goal

Make `FontAntialiasing::Subpixel` render useful horizontal RGB LCD text in eepp and ecode,
using the coverage filter and per-channel compositing model used by lite-xl while preserving
eepp's renderer portability, mixed-font behavior, transparent framebuffers, and hot text paths.

The public option is historically named “subpixel hinting”, but the work here is principally
**LCD subpixel antialiasing and compositing**. `FontHinting` continues to control FreeType outline
hinting independently.

## Evidence and current-state analysis

### The FreeType half is already present

`FontTrueType::fontSetRenderOptions()` already performs the important rasterization setup:

- `FontAntialiasing::Subpixel` selects `FT_RENDER_MODE_LCD`.
- The LCD filter weights are `{ 0x10, 0x40, 0x70, 0x40, 0x10 }`, exactly the weights used by
  the supplied lite-xl renderer.
- LCD bitmaps are interpreted at one logical pixel per three FreeType bitmap bytes.
- Each glyph atlas pixel stores the three LCD coverages in RGB.

The atlas alpha is currently set to the arithmetic mean of those coverages. Normal eepp texture
blending then treats that one alpha value as the coverage for all destination channels. That loses
the information LCD rendering needs and is the direct reason the option does not work correctly.

Repository history confirms this is deliberately unfinished rather than dead code. Commit
`7425b77f9` introduced the settings with the note that subpixel support still needed a fragment
shader. The ecode menu later exposed that state as “SubPixel (not working)”.

### What lite-xl does

The supplied `ren_draw_text()` path has two separable behaviors:

1. It rasterizes horizontal LCD masks using the same five FreeType filter weights already used by
   eepp.
2. Its software compositor blends each destination color channel with its corresponding LCD
   coverage:

   ```text
   out[c] = text[c] * text_alpha * coverage[c]
          + dst[c]  * (1 - text_alpha * coverage[c])
   ```

lite-xl also caches three glyph masks translated by 0, 1/3, and 2/3 pixel and selects one from the
fractional pen position. That is an additional positioning-quality refinement, not the missing
compositor itself. eepp currently quantizes some shaped positions and its retained `Text` path does
not know the final fractional screen position while building the glyph cache/vertices, so adopting
that part requires a larger cache and layout change. It is explicitly separated into a follow-up
scope below.

### A whole-scene framebuffer is not the required mechanism

`SceneNode` already has an optional framebuffer path (`enableFrameBuffer()`), using a texture-backed
RGBA framebuffer. Enabling it does not restore the three coverages after ordinary alpha blending:
once RGB coverage has been reduced to the atlas's average alpha, a post-process shader cannot infer
the original channel masks.

A destination-sampling implementation could use ping-pong framebuffers, but reading and writing the
same color attachment is not a valid general solution, and ping-pong rendering would add full-screen
copies/bandwidth and nested-FBO complexity. It is unnecessary here. The correct ownership boundary
is the glyph draw operation while both the LCD mask and destination blend are still available.

Therefore this plan makes **no change to `SceneNode` and does not force the application framebuffer
on**. The result must work equally when a scene framebuffer is enabled for an independent reason.

### All render paths that must be covered

`Text` has two materially different pipelines:

- The static/high-throughput `Text::draw()` path emits glyphs through `GlobalBatchRenderer`. This is
  ecode's main editor path and batches across calls.
- Retained `Text` objects build vertex/color arrays and later draw them directly. They support
  per-character colors and their own transforms.

Both paths can mix LCD glyphs, grayscale fallback glyphs, color emoji, effects, and solid atlas
quads for underline/strike-through. A global `FontAntialiasing` check is consequently not a safe
draw discriminator. The actual rasterized glyph format must travel with the cached glyph/drawable.

### Scope-level policy issue in ecode startup

`FontService` is correctly a `ResourceScope`-level policy owner, and changing its hinting or
antialiasing clears associated `FontTrueType` caches. ecode's menu updates that service at runtime.

At startup, however, ecode applies the loaded policy to individually loaded primary fonts without
first updating `defaultResourceScope().getFontService()`. System fallback fonts created later can
therefore inherit the service's default grayscale policy until the user changes the menu. The
implementation needs to establish both loaded policies on the owning service before asynchronous
font/fallback loading starts.

## Proposed architecture

### 1. Describe what each cached glyph contains

Add a compact internal render-kind enum, conceptually:

- `Mask`: monochrome/grayscale coverage and solid decoration texels.
- `LCDMask`: three horizontal RGB coverages.
- `Color`: BGRA/color emoji or another intrinsically colored glyph.

Determine it from the actual FreeType bitmap/pixel mode after rendering, not merely from the font's
requested policy. Store it on the cached `Glyph`, propagate it through `GlyphDrawable`, and preserve
it in text draw ranges. This correctly handles fallback fonts and formats that cannot produce LCD
bitmaps.

This should remain internal rendering metadata; no public API is needed unless implementation shows
that an external renderer consumer genuinely needs it.

### 2. Add a renderer-owned LCD compositor

The built-in renderer, rather than `Text`, `FontService`, or `ResourceScope`, should own the shader
program and cached uniform/attribute locations. GL programs are context/render-pipeline resources,
and renderer lifetime already governs the other built-in programs.

The shader samples the existing atlas RGB mask. To reproduce lite-xl's per-channel equation with
arbitrary vertex colors and ordinary OpenGL blending, render each contiguous LCD text range once per
destination color channel:

1. Select the red mask component in the fragment shader and enable only red in `glColorMask()`.
2. Repeat for green and blue.
3. Use eepp's normal source-over RGB blend factors for every channel pass.
4. Perform an alpha-only pass using the existing mean coverage and source-over alpha factors.

The alpha-only pass is needed because eepp frequently renders into transparent RGBA framebuffers;
lite-xl's software surface simply preserves destination alpha. Omitting a meaningful alpha update
would make correctly colored text disappear or composite incorrectly when that framebuffer is drawn
later. The alpha pass must use eepp's `BlendMode::Alpha()` alpha factors (`One`,
`OneMinusSrcAlpha`), not square the source alpha.

This four-pass strategy is local to LCD ranges, uses no destination texture reads, supports
per-character/vertex text colors, and preserves ordering. It costs additional glyph fragments and
draw calls, so ranges and state transitions must be coarse, cached, and benchmarked. Normal
grayscale and color-glyph paths remain single-pass and unchanged.

The compositor must:

- have shader sources for the supported programmable renderer variants (GL2, GL3/core, and GLES2,
  following the existing renderer conventions);
- compile once per renderer/context, cache all locations, and never do string lookup or shader
  compilation per text draw;
- preserve clipping and the existing model-view/projection conventions;
- save/restore the prior program, color mask, blend mode/equation, texture state, and batch state;
- coexist with externally selected shaders rather than silently replacing unrelated application
  drawing state;
- fail softly and log once if the LCD program is unavailable.

For shaderless/unsupported contexts, an LCD request must fall back to a neutral grayscale mask
(white RGB plus the mean coverage alpha, or grayscale rasterization before upload). It must never
fall through to today's colored-mask-with-average-alpha output.

### 3. Integrate the static batch path without per-glyph overhead

Extend `BatchRenderer` with an internal text coverage mode (`normal` or `LCD`) and flush only when
that mode actually changes. `Text::drawGlyph()` selects the mode from `GlyphDrawable` metadata.

The batch flush delegates an LCD range to the renderer compositor; the batcher must not own or
compile GL programs. Consecutive editor glyphs then stay in one large LCD batch, while transitions
to color emoji, grayscale fallback, or decoration quads produce the minimum necessary flushes.

Shadows and outline glyphs use the glyph's LCD mode. Underline and strike-through atlas quads always
use normal scalar-alpha rendering.

No callback, heap allocation, dynamic cast, or program lookup is permitted per glyph in this hot
path.

### 4. Integrate retained `Text` with compact ordered ranges

While rebuilding retained geometry, record contiguous `(first vertex, vertex count, render kind)`
ranges alongside the existing fill and outline arrays. Draw those ranges in original order:

- normal masks and colored glyphs use their current single draw;
- LCD ranges use the renderer compositor;
- decoration ranges remain normal.

Prefer an inline/small-vector representation because the common case has one range. Do not add a
render-mode field to every vertex: that would increase persistent text memory and GPU bandwidth for
all text to solve a range-level state problem.

Preserve existing character-color behavior, emoji whitening rules, clipping, shadows, outlines,
underline/strike-through, and fallback texture/page changes.

### 5. Apply LCD rendering only where the pixel geometry is valid

This first implementation defines the existing `Subpixel` option as horizontal **RGB** stripes,
matching the supplied lite-xl code and FreeType LCD mode. The current API has no RGB/BGR or vertical
panel-order selection.

LCD masks are only valid when their horizontal samples reach physical framebuffer pixels at a 1:1,
axis-aligned transform. At each LCD range/batch—not per glyph—check the effective 2D transform. Use
the LCD compositor only for unit-scale, non-rotated output; use the neutral average-coverage
grayscale path for rotation, non-unit scaling, shear, or otherwise unsuitable transforms.

This protects retained text transformations and scene/world text. An independently enabled scene
FBO remains eligible only when it is finally presented 1:1 without scaling/filtering that would mix
the RGB samples.

BGR/vertical stripe layouts and transformed-output reconstruction are deferred rather than guessed.

### 6. Correct policy initialization and ecode presentation

After ecode loads font settings and before it starts asynchronous main/fallback font loading:

- set the loaded hinting and antialiasing values on the default scope's `FontService`;
- keep per-font setup only where it is still needed for ownership/thread timing;
- verify local scene-scope fonts and imported/default fonts retain the intended owning-service
  semantics;
- leave runtime policy changes cache-invalidating and immediately visible;
- rename the menu item from “SubPixel (not working)” to “SubPixel” only after the renderer path is
  complete and tested.

## Expected file areas

Exact signatures should follow local conventions discovered during implementation, but the expected
touch points are:

- `include/eepp/graphics/font.hpp` and/or internal font/glyph headers: glyph render-kind metadata.
- `src/eepp/graphics/fonttruetype.cpp`: derive the actual kind and provide neutral fallback data.
- `include/eepp/graphics/glyphdrawable.hpp`, `src/eepp/graphics/glyphdrawable.cpp`: propagate kind.
- `include/eepp/graphics/batchrenderer.hpp`, `src/eepp/graphics/batchrenderer.cpp`: range mode and
  transition flushes.
- renderer headers/implementations and built-in shader source area: context-owned LCD compositor,
  capability detection, and full state restoration.
- `include/eepp/graphics/text.hpp`, `src/eepp/graphics/text.cpp`: both static and retained paths,
  ordered mode ranges, effects, and transform eligibility.
- ecode application/font setup and `src/tools/ecode/settingsmenu.cpp`: service initialization and
  final menu label.
- `src/tests/unit_tests/fontrendering_tests.cpp`: focused coverage and regression tests.

`src/eepp/scene/scenenode.cpp` is deliberately not an expected implementation file.

## Implementation sequence

### Phase A — establish a measurable baseline

1. Add a small diagnostic/test scene that renders the same colored edge onto opaque and transparent
   contrasting backgrounds using grayscale and current subpixel policies.
2. Record baseline pixels and batch/draw counts for the static editor-like path and retained path.
3. Confirm active test renderers and framebuffer formats so shader variants and alpha behavior are
   exercised deliberately.

### Phase B — metadata and safe fallback

1. Introduce glyph render-kind metadata derived from the actual rasterized bitmap.
2. Propagate it through `GlyphDrawable` without changing layout metrics or cache keys unnecessarily.
3. Make unsupported LCD compositing neutral and grayscale instead of colored.
4. Add unit coverage for bitmap-mode classification and policy-driven cache invalidation.

### Phase C — renderer compositor

1. Add and validate built-in shader variants.
2. Add the RGB channel passes and alpha-only pass with scoped state restoration.
3. Add transform/capability gating and the grayscale fallback path.
4. Exercise opaque and transparent targets before wiring the main editor path.

### Phase D — both `Text` paths

1. Add mode-aware static batching and transition flushes.
2. Add compact ordered ranges to retained fill/outline geometry.
3. Audit shadows, outlines, decorations, fallback texture pages, color emoji, clipping, and
   per-character colors.
4. Compare both paths pixel-for-pixel where their geometry is otherwise identical.

### Phase E — service and ecode integration

1. Initialize the default `FontService` policy before font work starts.
2. Verify live option changes and cache rebuilds.
3. Remove the “not working” suffix.
4. Manually inspect editor text on dark/light themes and layered/transparent UI surfaces.

### Phase F — performance and correctness validation

1. Ensure LCD text is range-batched and no new allocation occurs per glyph or per frame.
2. Compare frame time, draw calls, flush count, atlas memory, and glyph rebuild behavior in a large
   syntax-highlighted ecode document.
3. If four-pass LCD ranges regress the editor materially, optimize range coalescing/state caching
   before considering a destination-sampling/FBO design.
4. Run formatting, the focused unit tests, the complete suite, and an ASan/debug build according to
   the project rules.

## Test plan and acceptance criteria

### Automated rendering tests

- Render colored LCD text over a non-neutral opaque background and inspect edge pixels. Each output
  channel must follow its own atlas coverage rather than the mean coverage.
- Render the same case through static `Text::draw()` and retained `Text`; tolerate only documented
  geometry differences.
- Render into a transparent RGBA framebuffer, composite that texture onto another background, and
  verify text remains visible with correct alpha and RGB behavior.
- Exercise LCD text adjacent to grayscale/system fallback glyphs, color emoji, underline,
  strike-through, shadow, and outline without mode leakage or tinting.
- Exercise per-character colors and syntax-style runs in a single batch.
- Change `FontService` between grayscale and subpixel at runtime and verify cache invalidation,
  render-kind changes, and stable metrics.
- Verify rotated/scaled text and unsupported shader contexts take the neutral grayscale fallback.
- Compare scene-FBO off/on at a 1:1 presentation within a small pixel tolerance; enabling the FBO
  must not be a prerequisite.
- Compile/link every applicable built-in shader variant covered by the test environment.

### Manual ecode checks

- Toggle None, Grayscale, and SubPixel live and restart with each persisted selection.
- Inspect small editor fonts, syntax colors, selection/search overlays, terminal text, popups, light
  and dark themes, fallback scripts, and emoji.
- Check secondary windows and UI opacity/layering paths.
- Check normal and HiDPI configurations, and verify non-1:1 transforms fall back cleanly rather than
  showing colored fringes in the wrong geometry.

### Performance acceptance

- No shader construction, uniform lookup, heap allocation, or glyph-cache lookup added per glyph
  beyond the existing lookup.
- Ordinary grayscale/color text retains its current one-pass path and batching behavior.
- LCD draw calls scale with contiguous batches/ranges, not with glyph count.
- Editor frame-time and batch counters are documented before/after; a material regression blocks
  completion until understood and reduced.

### Project validation commands for the implementation phase

Follow `.agent/rules/build-project.md` and `.agent/rules/unit-tests.md` exactly:

1. Regenerate the Linux project with debug symbols and ASan, retaining the current graphics backend
   and using mold when available.
2. Format only changed C/C++ files with the repository format.
3. Build with `make -C make/linux -j$(nproc)`.
4. Run focused `FontRendering.Subpixel*` tests through `projects/scripts/xvfb-run-eepp`.
5. Run the full test binary through the same wrapper.
6. Run `git diff --check` and inspect the complete diff for accidental public API/ABI or unrelated
   changes.

## Deliberately deferred work

### Three fractional glyph phases

lite-xl caches three horizontally translated LCD bitmaps for every glyph and selects a phase from the
fractional pen position. Adding this in the initial patch would:

- multiply LCD bitmap/cache variants by three;
- require phase-aware glyph keys and drawables;
- require retaining fractional shaped positions now truncated in part of `Text::draw()`;
- require the retained path to select or rebuild phases using the eventual screen-space origin;
- complicate transformed and fallback text behavior.

Recommendation: land and measure the correct LCD compositor first. Then add the three-phase cache as
a separately reviewed quality improvement using the same pixel tests and memory/performance
benchmarks. This still follows lite-xl's essential visual algorithm in the first patch: identical
filter weights and independent per-channel blending.

### Other deferred extensions

- Configurable RGB versus BGR panel order and vertical subpixel layouts.
- Gamma-linearized coverage/compositing; lite-xl's cited implementation blends byte-space values, so
  changing color space would no longer be a direct match.
- Enabling the scene framebuffer for unrelated post-processing.

## Risks and mitigations

- **Four-pass LCD cost:** batch contiguous ranges, cache all state/program data, benchmark ecode's
  actual editor workload, and keep all non-LCD text on the original path.
- **GL state leakage:** use a single renderer-owned entry point with explicit scoped restoration and
  regression tests that draw other primitives immediately before/after LCD text.
- **Mixed atlas content:** classify actual glyph bitmap formats and ordered ranges; never infer the
  whole draw mode solely from the font setting.
- **Transparent FBO alpha:** retain the explicit alpha-only coverage pass and test the final
  framebuffer composition, not just its RGB attachment.
- **Transforms/panel assumptions:** gate LCD at range granularity and fall back neutrally when the
  physical pixel mapping is unsuitable.
- **Startup fallback mismatch:** initialize the owning `FontService` before asynchronous font loads.
- **Legacy renderer support:** compile per-renderer variants and provide a neutral non-LCD fallback;
  do not make subpixel support a requirement for eepp to render text.

## Approval decisions

The recommended initial implementation assumes:

1. Direct per-glyph-range compositing; no forced `SceneNode` framebuffer.
2. Correct RGB and alpha output via three color-channel passes plus one alpha-only pass.
3. Horizontal RGB order only, with neutral grayscale fallback for unsuitable transforms/contexts.
4. Matching lite-xl's existing filter weights and per-channel blend equation now.
5. Deferring lite-xl's three fractional-position glyph phases to a follow-up patch.

Implementation should begin only after these scope decisions are approved or revised.
