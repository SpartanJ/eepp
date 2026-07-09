# Font Weight Support Plan

## Goal

Make CSS `font-weight` select the best available real font representation:

1. An explicit `@font-face` entry for the requested family, style, and weight.
2. A variable font instance with its `wght` axis set to the requested weight.
3. A system or locally discovered static font file matching the requested weight.
4. Synthetic bold only as a fallback when no real bold-capable face exists.

The current implementation already handles the first two cases for HTML
`@font-face` fonts. This plan is now focused on consolidating that behavior and
filling the remaining gap: native/static family variants beyond the old
Regular/Bold/Italic/BoldItalic model.

## Current State

### Implemented

- `FontWeight` exists with CSS weights 100 through 900.
- `FontStyleConfig` stores `Weight`.
- `Text::stringToFontWeight()` and `Text::fontWeightToString()` parse and
  serialize named and numeric weights.
- Text widgets expose `setFontWeight()` / `getFontWeight()`.
- `font-weight` CSS updates `Weight` and maps weights >= `SemiBold` to the
  legacy `Text::Bold` style bit for renderer compatibility.
- `UISceneNode::getFontFromNamesList()` accepts `FontWeight`.
- `UISceneNode::loadFontFaces()` registers scene-scoped `@font-face` aliases by:
  - author family,
  - `Text::Bold` / `Text::Italic` style bits,
  - numeric `FontWeight`.
- `UISceneNode` keeps a reverse `Font* -> author family` map for `@font-face`
  fonts.
- `UISceneNode::reevaluateFontStyle()` uses that reverse map to re-resolve the
  actual `Font*` when style or weight changes after the font was selected.
- `UISceneNode::getFontFamilyName()` uses the reverse map so
  `getPropertyString(font-family)` returns the author family instead of the
  internal `__eepp_font_face_...` name.
- `FontTrueType::setVariableFontWeight()` applies the OpenType `wght` axis with
  FreeType and clears glyph/kerning caches.
- `@font-face` loads call `setVariableFontWeight()`, so multiple CSS weights can
  point to the same variable WOFF2/TTF file and still render at distinct weights.
- `SystemFontResolver` is weight-aware at the descriptor/query level.

### Still True

- The rendering hot path is still binary:
  - `Font::getGlyph(..., bool bold, bool italic, ...)`
  - `FontTrueType::getGlyph(..., bool bold, bool italic, ...)`
  - `Text` / `TextLayout` pass `bool bold` derived from `Text::Bold`.
- `FontTrueType` still has only three sibling pointers:
  - `mFontBold`
  - `mFontItalic`
  - `mFontBoldItalic`
- `FontFamily::loadFromRegular()` only discovers Bold, Italic, and BoldItalic
  siblings by filename.
- Native non-`@font-face` families do not yet support separate Medium,
  SemiBold, ExtraBold, Black, etc. files as first-class variants.
- FontManager naming for native style variants still uses `Family#bold` /
  `Family#italic`, not a stable numeric weight key.

## Design Direction

Keep real font selection outside the glyph hot path for now.

The current architecture works best when a widget's `FontStyleConfig.Font` is
already the best concrete `Font*` for the requested family, style, and weight.
The renderer can then continue passing `bool bold` as a compatibility and
synthetic fallback signal.

Do not start by changing `Font::getGlyph()` to take `FontWeight`. That would
touch many rendering paths and bitmap/sprite font implementations while not
solving the main missing piece: discovering and selecting the right concrete
font object before rendering.

Instead:

- keep `FontWeight` in style/config/resolution layers;
- improve how native/static font variants are registered and found;
- keep synthetic emboldening as fallback behavior;
- only revisit the glyph API if concrete font selection proves insufficient.

## Important Existing Behavior

`reevaluateFontStyle()` remains necessary.

CSS and inherited style can arrive in separate steps. A widget can first resolve
`font-family` while weight is still normal, then later receive
`font-weight: 700`. At that point the widget holds a `Font*`, not the original
CSS family string. `reevaluateFontStyle()` bridges back from the current font to
the author/system family and asks `getFontFromNamesList()` for the correct
variant.

This is especially important for:

- `UIRichText::setFontWeight()` / `setFontStyle()`;
- `UITextSpan::setFontWeight()` / `setFontStyle()`;
- `UITextSpan::setInheritedStyle()`;
- `UITextView`, `UICodeEditor`, and `UITooltip` weight/style changes;
- `@font-face` fonts with internal names.

## Remaining Work

### Step 1 - Define Native Variant Keys

Add one canonical key format for concrete native font instances:

```text
Family#w400
Family#w700
Family#w700#italic
```

Requirements:

- Keep reading existing `Family#bold`, `Family#italic`, and
  `Family#bold|italic`/equivalent names during transition.
- Use numeric weight keys for all new native variant registrations.
- Keep `@font-face` aliases separate from FontManager names. The scene alias map
  should remain authoritative for document-scoped author fonts.

Open decision:

- Whether the key should be `Family#w700#italic` or `Family#w700|italic`.
  Choose one and centralize it in a helper instead of constructing strings at
  call sites.

### Step 2 - Centralize Font Lookup Name Construction

Create a helper for native font lookup keys, likely in `UISceneNode` or a small
graphics utility:

```cpp
std::string makeFontVariantName( std::string_view family, FontWeight weight, bool italic );
std::string makeLegacyFontStyleName( std::string_view family, Uint32 fontStyle );
```

Then update `UISceneNode::getFontFromNamesList()` so lookup order is:

1. `@font-face` alias by author family/style/weight.
2. Native numeric key (`Family#wNNN`, plus italic if needed).
3. Legacy key (`Family#bold`, `Family#italic`, etc.).
4. Plain family.
5. Generic/system resolver.

This keeps existing applications working while enabling non-binary weights.

### Step 3 - Expand Static Sibling Discovery

Update `FontFamily::loadFromRegular()` to discover standard weight files.

Suggested suffix table:

| Weight | Suffixes |
|--------|----------|
| 100 | `Thin`, `Hairline` |
| 200 | `ExtraLight`, `Extra-Light`, `UltraLight`, `Ultra-Light` |
| 300 | `Light` |
| 400 | `Regular`, `Book` |
| 500 | `Medium` |
| 600 | `SemiBold`, `Semi-Bold`, `DemiBold`, `Demi-Bold` |
| 700 | `Bold` |
| 800 | `ExtraBold`, `Extra-Bold`, `UltraBold`, `Ultra-Bold` |
| 900 | `Black`, `Heavy` |

For each discovered face:

- load it as a separate `FontTrueType`;
- register it in `FontManager` under the numeric key;
- keep populating old `mFontBold`, `mFontItalic`, and `mFontBoldItalic` for
  compatibility;
- copy relevant behavior from the regular face:
  - `setBoldAdvanceSameAsRegular()`;
  - dynamic monospace configuration;
  - fallback/emoji/system fallback flags if needed.

Do not introduce a weight map inside `FontTrueType` yet unless a concrete
rendering bug requires it.

### Step 4 - Improve System Resolver Integration

`SystemFontResolver` can already resolve by `FontWeight` and italic. Tighten
`UISceneNode::getFontFromNamesList()` around that capability:

- when `SystemFontResolver` returns a `FontDesc`, name/load the result with the
  numeric key;
- cache/reuse an existing loaded numeric-key font before loading a new face;
- link legacy sibling pointers only for bold/italic compatibility;
- preserve the actual resolved `FontDesc::weight` when available.

This should allow CSS such as:

```css
body {
  font-family: system-ui;
  font-weight: 500;
}
```

to use a real Medium file when the platform has one.

### Step 5 - Audit Style/Weight Synchronization

Today `setFontWeight()` updates the `Text::Bold` bit, but `setFontStyle()` can
derive a weight from only the bold bit:

```cpp
( fontStyle & Text::Bold ) ? FontWeight::Bold : FontWeight::Normal
```

Audit all text widgets so style and weight stay coherent:

- If `setFontStyle()` receives `Text::Bold`, set weight to `Bold` unless a more
  specific weight was already explicitly set by CSS.
- If `setFontStyle()` only changes italic/decoration bits, preserve the existing
  `Weight`.
- Make `UIConsole::setFontWeight()` consistent with the other text widgets:
  currently it updates style but does not call `reevaluateFontStyle()`.

This should be done carefully because `Text::Bold` is still both:

- a compatibility style flag;
- the renderer's synthetic-bold signal.

### Step 6 - Synthetic Bold Policy

Keep this policy:

- If a real selected font is already bold (`mIsBold == true`), do not synthesize
  extra emboldening.
- If the selected font is not bold and requested weight >= `SemiBold`, allow
  synthetic emboldening.
- For weights below `SemiBold`, never synthesize bold.

The existing `FontTrueType::loadGlyphByIndex()` already avoids emboldening when
`mIsBold` is true. Tests should make sure future native variant loading does not
regress this.

### Step 7 - Variable Font Follow-Ups

The current `setVariableFontWeight()` only applies the `wght` axis. That is
enough for the Fira Code repro, but the API should eventually support:

- detecting whether a font has a `wght` axis without mutating it;
- storing the currently applied variable weight for diagnostics and tests;
- applying italic/slant axes (`ital`, `slnt`) if needed;
- using variation coordinates in font descriptions where possible.

This is lower priority than native static weight selection because
`@font-face` variable weight rendering already works for the known repro.

## Tests To Keep / Extend

Existing relevant tests:

- `FontRendering.fontWeightToString`
- `FontRendering.stringToFontWeight`
- `FontRendering.fontWeightInStyleConfig`
- `FontRendering.fontFaceReevaluateStyleUsesAuthorFamily`
- `UIWebView.FontFaceWeightSurvivesViewportRelayout`
- `SystemFontResolver.resolveGenericWeights`
- `SystemFontResolver.resolveGenericWeightPreference`
- `SystemFontResolver.resolveBoldFromNamesList`

Add or extend:

| Test | Purpose |
|------|---------|
| `FontRendering.nativeWeightKeyLookup` | `Family#w500` resolves before `Family#bold` or plain family. |
| `FontRendering.siblingDiscoveryAllWeights` | `FontFamily::loadFromRegular()` registers available weight siblings. |
| `FontRendering.realBoldDoesNotDoubleEmbolden` | A real bold selected face is not synthetically emboldened again. |
| `FontRendering.syntheticBoldFallback` | Weight >= 600 still emboldens when no real variant exists. |
| `UIRichText.fontStylePreservesExplicitWeight` | Changing italic/style after `font-weight: 500` preserves weight 500. |
| `UIConsole.fontWeightReevaluatesFont` | Console weight changes reselect available variants like other widgets. |
| `UIWebView.fontFamilyPropertyUsesAuthorName` | `getPropertyString(font-family)` never exposes `__eepp_font_face_...`. |

## Non-Goals For The Next Pass

- Do not replace `Font::getGlyph(bool bold, bool italic)` globally.
- Do not add weight maps to bitmap or sprite fonts.
- Do not remove legacy `mFontBold`, `mFontItalic`, or `mFontBoldItalic`.
- Do not remove `reevaluateFontStyle()`.
- Do not make network-dependent Google Fonts tests permanent unless a local
  variable-font fixture is added.

## Suggested Implementation Sequence

1. Add centralized native font variant name helpers.
2. Update `getFontFromNamesList()` to prefer numeric weight keys while keeping
   legacy names.
3. Update `FontFamily::loadFromRegular()` to register discovered static weights.
4. Tighten `SystemFontResolver` loading/caching to use numeric keys.
5. Audit `setFontStyle()` / `setFontWeight()` consistency across text widgets.
6. Add tests for native weights, synthetic fallback, and property strings.
7. Reassess whether the glyph API needs `FontWeight`. Only do that if concrete
   font selection cannot cover a real use case.

## Allocation / Performance Notes

- Keep variant discovery/load costs outside the draw loop.
- Numeric key lookup is a `FontManager` lookup during style resolution, not per
  glyph.
- Do not add an `UnorderedMap` lookup to every glyph fetch unless measurements
  show it is necessary and acceptable.
- Variable font coordinate changes clear glyph/kerning caches and should happen
  only at load/configuration time.
- Loading every possible sibling eagerly can increase memory use. Prefer loading
  files discovered beside an explicitly loaded regular font, and rely on
  `SystemFontResolver` for platform fonts.
