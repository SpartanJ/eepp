# Old Reddit Thread Rendering Plan

## Objective

Render `bin/unit_tests/assets/html/reddit_old_thread.html` close to the Chrome reference image for a 1024px old Reddit comments page. The fixture is intentionally large enough to exercise real old Reddit layout patterns while still being local and deterministic.

The target is not a Reddit-specific hack. Each fix must move the HTML/CSS engine closer to the relevant CSS behavior and be covered by reduced tests before relying on the full-page screenshot.

## Current Fixture And Harness

- Fixture: `bin/unit_tests/assets/html/reddit_old_thread.html`
- Local assets:
  - `bin/unit_tests/assets/html/reddit_old_thread_files/reddit.ETA_etA2z5U.css`
  - `bin/unit_tests/assets/html/reddit_old_thread_files/sprite-reddit.13AvZYXRW_4.png`
  - `bin/unit_tests/assets/html/reddit_old_thread_files/pixel.png`
- Reference image: `bin/unit_tests/assets/html/reddit_old_thread_reference_image.png`
- Smoke test: `UIHTML.redditOldThreadWebViewSmoke`

The smoke test opens the old Reddit fixture through `UIWebView` without loading the app theme. Browser-like HTML defaults are supplied by the HTML base defaults stylesheet when HTML widgets enter the node tree, so the test exercises the same default-style path used by real HTML content. It asserts the important page regions exist and emits `assets/html/eepp-reddit-old-thread-current.webp` through the existing image comparison helper.

The smoke test is opt-in because the full fixture is slow in ASAN:

```sh
EEPP_REDDIT_OLD_THREAD_VISUAL=1 projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIHTML.redditOldThreadWebViewSmoke"
```

It currently writes the visual artifact to `bin/unit_tests/output/eepp-reddit-old-thread-current.webp`. This is not a golden comparison yet. Replace or extend it with a true reference comparison only after the high-priority layout issues are fixed enough for pixel diffs to be useful.

Current progress:

- `UIHTMLFloat.leftFloatOverflowHiddenBlockFormattingContextSitsBesideFloat` covers the old Reddit `.midcol` + `.entry { overflow:hidden }` pattern.
- `overflow` values other than `visible` are tracked as block-formatting-context semantics on `UIHTMLWidget`.
- RichText atomic block metadata now distinguishes normal block boxes from block formatting contexts.
- `.entry` now starts to the right of `.midcol` in the fixture (`entry.x=44`, `midcol.x=15`, `midcol.width=19`).
- `UIHTMLFloat.rightFloatConstrainsTextInsideFollowingNormalBlock` covers a normal block whose internal text line boxes must avoid an active right float.
- `UIHTMLFloat.rightFloatConstrainsNestedBlockFormattingContext` covers inherited right-float exclusions flowing through a normal block into a nested BFC child.
- RichText can now receive external float exclusions from a parent block formatting context. Imported exclusions constrain line boxes and BFC placement but do not contribute to the receiver's own content height.
- `BlockLayouter` now forwards inherited float exclusions through normal non-floating blocks and preserves the parent-computed used width for BFC match-parent children next to floats.
- The fixture now keeps the main `.entry` selftext box to the left of `.side` (`side.x=719`, `entry.x=44`, `entry.width=670` in the current smoke run).
- External float exclusions are filtered for fixed-width descendants whose content box is entirely outside the float's horizontal range. This keeps the comment form textarea from being pushed below the sidebar while still letting match-parent content compute a float-constrained used width.
- The smoke test no longer loads `breeze.css`; HTML defaults now come from automatic HTML base-default injection.
- Legacy `<strike>` is registered as an HTML phrasing element and receives default `line-through` styling, removing a missing-element warning from the fixture.
- Horizontal `auto` margins are recomputed at RichText/block layout read points, and the old Reddit vote arrow is centered inside `.midcol` (`arrow.x=17`, `midcol.x=15`, `arrow.width=15`, `midcol.width=19` in the current smoke run).
- Vote arrow sprite CSS now resolves relative to the stylesheet file, preserves negative `background-position`, and is asserted in both a reduced sprite test and the old Reddit smoke test.
- CSS `white-space` now maps browser values such as `normal`, `nowrap`, `pre`, `pre-wrap`, `pre-line`, and `break-spaces` into RichText whitespace-collapse and soft-wrap behavior. `nowrap` suppresses soft wrapping for text and atomic inline boxes, including old Reddit-style inline flat lists, and overflowing text runs no longer force following inline content onto a new line.
- Collapsed whitespace-only text nodes between non-inline boxes no longer create a spurious line box in RichText layout.
- HTML `<button>` is created as an HTML rich-text element with browser-like inline-block defaults so it can participate in CSS display/float layout while still rendering child text. This fixes the top-row placement of old Reddit's `#redesign-beta-optin-btn`.
- RichText virtual block breaks no longer split a line that contains only floats, allowing a following BFC to remain beside those floats in reduced cases.
- Collapsed whitespace-only text between floated inline-display boxes no longer creates in-flow line content. This fixes the old Reddit `#sr-header-area .sr-list` placement: it now sits in the top row beside the floated redesign button and subreddit dropdown, instead of landing at `y=18` and overlapping `#header-bottom-left`.
- The old Reddit subreddit bar now remains a single clipped 18px row under `white-space: nowrap`; the current smoke run reports `srList=(252,0 772x18)`.
- CSS absolute font-size keywords now use the browser scale (`medium` = 16px, `small` = 13px, `x-small` = 10px) instead of the app theme's 12px UI default. This fixes old Reddit title and metadata sizing rules such as `.link .title { font-size: medium }` and `.tagline { font-size: x-small }`.
- Floated inline text elements now follow CSS blockification: once `float` is not `none`, inline spans/anchors stop being rebuilt as inline text and enter the atomic float path. This fixes old Reddit's comment form footer links (`.help-toggle` and `a.reddiquette`) so they align to the right under the textarea.
- `vertical-align` on an inline-block `UITextSpan` now applies to the inline-block box in the parent line, not to its own internal self text. This keeps old Reddit's `.domain a { display:inline-block; vertical-align:middle }` from inflating the anchor's own line box while preserving parent-line alignment semantics.
- `clear` now ignores external float exclusions inherited from an outer formatting context. This matches the old Reddit `.entry { overflow:hidden }` BFC case: its child `.expando { clear:left }` must not clear the sibling `.midcol` float outside `.entry`. The current smoke run moves `.expando` from `y=126` to `y=119.609` and `.usertext-body .md` from `y=131` to `y=124.609`.
- `updateOutOfFlowPosition()` no longer uses the generic `mPacking` early return. That guard kept old Reddit's `#header-bottom-right { position:absolute; right:0; bottom:0 }` at an early `x=297,y=-1` measurement. The current smoke run places it at `x=717,y=41` in the `1024x64` header and asserts its right/bottom edges.
- `input checked` / `checked="checked"` now initializes `UIHTMLInput` checkbox and radio children. Checkbox/radio `value` is preserved as the submitted form value instead of being interpreted as checked state by the child control. This fixes old Reddit's checked sidebar flair checkbox and is covered by `UIHTMLInput.sizeAttribute`.
- Current smoke geometry on 2026-05-31: `side=(719,64 300x1186.36)`, `entry=(47,71 667x531.609)`, `selftextMd=(47,124.609 667x450)`, `commentArea=(5,610.609 1014x391)`, `commentTextarea=(16,694.609 500x100)`, `srList=(252,0 772x18)`, `headerBottomRight=(717,41 307x22)`.
- Remaining visible blockers: the main selftext and comment form still start lower than the Chrome reference; footer/comments vertical spacing still diverges from Chrome; sidebar/footer content below the first viewport still needs tightening; additional header/logo/submit-button sprite details remain.

## Reference Layout Invariants

At 1024px wide, the Chrome reference has these visible structural invariants:

- Header bands consume the top area only; the body content starts below the blue subreddit header.
- `.side` is a 300px right float, positioned at the right edge with small horizontal margins.
- `.content` is a normal block, not floated. It starts at the left margin and is not pushed below `.side`.
- Main post:
  - `.rank` and `.midcol` sit to the left of `.entry`.
  - `.midcol` is a left float with a narrow fixed width.
  - `.arrow.up`, score text, and `.arrow.down` are stacked vertically inside `.midcol`.
  - `.entry` has `overflow: hidden`, establishing a block formatting context that sits beside the left floats instead of overlapping them.
- The post body `.usertext-body .md` has a bordered light-blue box with line-wrapped paragraphs.
- `.entry .buttons li` render as a compact horizontal flat list.
- Comment controls and comment bodies line up under the main post content, with nested comment vote columns offset but not collapsed into text.
- Sprite-backed elements such as arrows, the Reddit logo, and submit button nubs render from `sprite-reddit.13AvZYXRW_4.png` using background position.

## Pending Issues To Fix

### 1. Float Placement And Block Formatting Contexts

Old Reddit depends heavily on CSS2 float rules:

- `.side { float: right; width: 300px; }`
- `.content { margin: 7px 5px 0 5px; }`
- `.midcol { float: left; overflow: hidden; }`
- `.entry { overflow: hidden; margin-left: 3px; }`

Important behavior:

- A normal block following a float keeps its normal-flow block position. The float affects line boxes inside that block, not the block border box itself.
- A block formatting context next to a float must not overlap the float. If there is enough horizontal space, it should sit beside the float; otherwise it moves below.
- `overflow` values other than `visible` create a block formatting context for block boxes.
- Floats participate in the current formatting context and must be visible to later sibling line layout until cleared or until their bottom is passed.
- A `clear` inside a nested block formatting context clears floats from that same formatting context, not external float exclusions inherited only for line avoidance and BFC placement.

Implementation gaps to investigate:

- `RichText::layoutWithFloats()` currently owns the float exclusion logic for inline atomic boxes, but full block layout still lacks a first-class float context shared across nested block layouters.
- `BlockLayouter` needs explicit CSS block formatting context semantics instead of relying on generic layout sizing and RichText atom placement.
- Nested line boxes inside `.content` and `.entry` need access to active sibling floats when the CSS formatting context requires it.

Reduced tests to add before the full-page comparison:

- Right float plus following normal block: block x/y unchanged, inner text lines avoid the float.
- Right float plus following `overflow:hidden` block: BFC block is placed beside the float and width is reduced to available space.
- Left float plus following `overflow:hidden` block: old Reddit `.midcol` and `.entry` pattern, with `.entry.x >= midcol.right + margin`.
- Left and right floats on the same line: following inline content uses the remaining middle strip.
- Clear behavior inside a mixed float context: `.clearleft` lands below left floats but not necessarily right floats.
- Clear behavior inside a nested BFC: a child with `clear:left` does not jump below an external sibling float from the parent formatting context.

### 2. Float Width Resolution

Recent work fixed floated `li` elements by making auto-width floated match-parent widgets shrink-to-fit. This needs broader coverage:

- Floats with `width:auto` should use CSS shrink-to-fit width.
- Floats with explicit width should honor width plus margins.
- Percentage widths on floats should resolve against the containing block.
- Min/max width should constrain the shrink-to-fit result.

Old Reddit examples:

- `.side` is fixed 300px and should stay fixed.
- `.midcol` gets a width from an inline style rule in the page, using `ex`: `width: 3.1ex`.
- `.rank` gets `width: 1.1ex`.

Reduced tests:

- Floated block `width:auto` shrink-to-fit.
- Floated block `width:300px` fixed.
- Floated block `width:3.1ex` resolves with font metrics.
- Float margins are included in placement and exclusion rectangles.

### 3. `overflow` Semantics

Old Reddit uses `overflow:hidden` for layout, not only clipping:

- `.entry { overflow: hidden; }`
- `.midcol { overflow: hidden; }`
- `.morelink a { overflow: hidden; text-overflow: ellipsis; }`

Required behavior:

- Block boxes with `overflow` other than `visible` establish a BFC.
- BFC boxes should avoid overlapping active floats in the same block formatting context.
- Actual clipping should be handled separately from BFC creation.

Reduced tests:

- `overflow:hidden` block beside a float.
- `overflow:visible` block can overlap float border box while its line boxes avoid the float.
- `overflow:hidden` clips children visually without changing normal-flow y unexpectedly.

### 4. CSS Display Defaults And Element Creation

The old `breeze.css` dependency has been removed from the old Reddit smoke test. HTML base defaults are now injected automatically when HTML content is attached to a scene.

Old Reddit relies on browser defaults for:

- `html`, `body`, `div`, `p`, `form`, `ul`, `li`, `h1`, `hr`
- inline anchors and spans
- replaced/form controls such as `input`, `textarea`
- hidden inputs and `display:none` nodes

Remaining plan:

- Audit `UIWidgetCreator` and HTML element constructors for browser-like default display.
- Keep theme-specific visual styling in CSS, but move semantic defaults into element creation.
- Add tests that load minimal HTML without `breeze.css` and verify display, margins, and intrinsic sizes for the core elements old Reddit uses.
- Keep input state semantics in `UIHTMLInput`, not in the child theme widgets. Boolean HTML attributes such as `checked` need HTML truthiness (`checked="checked"` is true), while `value` remains the submitted value.

Reduced tests:

- `p` has block display and browser-like top/bottom margins.
- `ul`/`li` default list layout remains correct.
- `form` is block but does not add unexpected margins.
- `input type=hidden` is not visible and does not affect layout.
- `textarea rows/cols` has intrinsic size before external CSS.

### 5. CSS Selector And Stylesheet Coverage

The old Reddit CSS is minified and selector-heavy. The fixture exercises:

- chained classes: `.thing.id-t3_...odd.link.self`
- descendant selectors: `.entry .buttons li`
- child selectors in inline page CSS: `body > .content .link .midcol`
- pseudo-classes/pseudo-elements in many rules
- media queries and vendor-prefixed declarations

Priority:

- Confirm selectors that affect the visible reference image are parsed and matched.
- It is acceptable to ignore unsupported dynamic pseudo-classes initially if the default state renders correctly.
- Pseudo-elements are not critical for the first old Reddit pass except where old Reddit uses `:before`/`:after` for visual icons.

Reduced tests:

- `body > .content .link .midcol` applies to the fixture structure.
- Multiple class selectors match correctly.
- Unsupported pseudo-class rules do not invalidate the whole selector list if another selector is valid.

### 6. Sprite Background Rendering

The reference depends on a sprite image for:

- Reddit logo/header icon.
- Vote arrows.
- Submit button nubs and gradients.
- Mail/preference/user icons.

Required behavior:

- `background-image: url(sprite-reddit.13AvZYXRW_4.png)` resolves relative to the CSS file or document URI as browsers do.
- `background-position: -42px -1678px` draws the correct sprite sub-rectangle.
- `background-repeat: no-repeat` is honored.
- Backgrounds are clipped to the padding/border box correctly.
- CSS custom properties that expand to image values should keep working where used by the bundled or older copied CSS.

Reduced tests:

- Background sprite with negative `background-position`.
- CSS-file-relative URL resolution.
- `background-repeat: repeat-x` button strip rendering.
- Element with explicit width/height and sprite background draws nonblank pixels.

### 7. Auto Margins And Centering

Vote arrows use:

- `.arrow { display: block; width: 15px; margin-left: auto; margin-right: auto; }`

Required behavior:

- Block-level auto horizontal margins center fixed-width children inside their containing block.
- Auto margin recomputation must happen after parent width and child width are known.
- This must work inside floated containers and BFC blocks.

Reduced tests:

- Fixed-width block centered with `margin-left:auto; margin-right:auto`.
- Same case inside a float.
- Same case after parent width changes.

### 8. Text Layout, Paragraph Metrics, And Form Controls

The screenshot is text-heavy. Issues here may look like float bugs even when positioning is correct.

Needs:

- Old Reddit font sizing: `font-size: x-small`, percentages, and inherited line-height.
- Inline-block `vertical-align` must align the atomic inline-block box in the parent line without changing the inline-block's own internal text line metrics.
- Paragraph margin collapse or equivalent spacing close enough for old Reddit.
- `textarea` dimensions from CSS and attributes: comment box should be large and aligned.
- `input type=text` shortlink field width/height and border.
- Link color, visited link color, bold text, and small metadata text.

Reduced tests:

- Absolute `font-size` keywords use the browser scale, while `smaller`/`larger` remain relative to the parent.
- Inline-block with `vertical-align: middle` has the same own text line height as the baseline-aligned inline-block, while the parent line still applies the middle alignment.
- `font-size:x-small` and percentage font-size inheritance.
- Paragraph margins inside `.md`.
- `textarea` with CSS width/height and `rows/cols`.
- `input readonly type=text` fixed width.
- `input type=checkbox checked value=...` initializes checked state and submits the explicit value.
- `input type=radio checked` initializes the radio active state.

### 9. Page Width, Scroll View, And Viewport Semantics

The fixture includes `<meta name="viewport" content="width=1024">`. The `UIWebView` path should behave like a 1024px viewport for this test.

Needs:

- `UIWebView` document container width should match the viewport and propagate into `html`/`body`.
- Body min-height should not force incorrect content placement.
- The full page should scroll vertically without clipping or changing layout widths.
- Screenshot tests should be explicit about viewport size.

Reduced tests:

- `UIWebView` file load sets document width equal to viewport width.
- Body/html min-height follows viewport height but content can exceed it.
- Right-floated sidebar remains at the same x before and after a scroll update.

### 10. Layout Invalidation And Performance

The full old Reddit fixture is expensive in debug/ASAN. Release builds are expected to be much faster, but the fixture still appears to trigger excessive invalidation and repeated layout recomputation. Treat this as a separate investigation track from visual correctness so we do not hide spec bugs behind caching.

Likely sources:

- `UIWebView::loadDocumentData()` loads a large DOM, external CSS, inline CSS, and then adjusts `html`/`body` min-height, causing follow-up invalidations.
- CSS application may update many widget properties one by one, each producing layout invalidation instead of batching style changes per element.
- `UIRichText` rebuilds RichText content from children and can be invalidated repeatedly while descendants are still loading/applying styles.
- Auto-size, match-parent, min/max intrinsic measurement, and float-aware layout can recursively query child sizes and trigger re-entry.
- Image/background/style resource resolution can mutate widget state after the first layout pass.

Plan:

- Add counters around dirty layout enqueue, `UIWidget::updateLayout()`, `UILayouter::layout()`, `UIRichText::rebuildRichText()`, RichText layout, and CSS property application.
- Add an opt-in diagnostic mode for the old Reddit smoke test that prints per-frame and total counts.
- Measure debug without ASAN, debug with ASAN, and release to separate sanitizer overhead from algorithmic churn.
- Batch style application where possible: apply all matched CSS properties, then invalidate layout once per widget.
- Avoid relayout when a property setter receives the same computed value.
- Ensure `UIWebView` viewport/html/body size synchronization happens once after document load when possible, not as a cascade of independent size changes.
- Cache intrinsic width/height results during a layout pass and invalidate them only on relevant style/content changes.

Reduced tests:

- Loading a generated page with many styled block children should produce a bounded number of layout passes.
- Reapplying the same style sheet should not dirty unchanged widget layout repeatedly.
- Measuring min/max intrinsic widths during a single parent layout should not rebuild the same child RichText multiple times.

## Suggested Phase Order

### Phase 1: Measurement Harness

Exit criteria:

- Done. `UIHTML.redditOldThreadWebViewSmoke` passes when `EEPP_REDDIT_OLD_THREAD_VISUAL=1` is set.
- Done. A current eepp screenshot is produced at `bin/unit_tests/output/eepp-reddit-old-thread-current.webp`.
- Done. The smoke dumps important node rects for `.side`, `.content`, `.midcol`, `.entry`, `.arrow`, `.usertext-body`, `.commentarea`, topbar/header nodes, and comment form controls.

Validation:

```sh
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIHTML.redditOldThreadWebViewSmoke"
```

### Phase 2: Float/BFC Correctness

Implement a shared CSS float context that block and inline layout can both see. Keep `RichText` responsible for line placement, but do not make it the only owner of float state when block children participate in the same formatting context.

Exit criteria:

- Mostly done for the old Reddit fixture. Reduced float/BFC tests cover the active `.side`, `.content`, `.midcol`, `.entry`, nested BFC, clear, and right-float textarea cases.
- `.side`, `.content`, `.midcol`, and `.entry` now match the reference broad geometry closely enough for later visual details to be meaningful.
- Keep this phase open only for newly found generic float regressions.

Validation:

```sh
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIHTMLFloat.*"
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIHTML.redditOldThreadWebViewSmoke"
```

### Phase 3: CSS Visual Features

Prioritize sprite backgrounds, URL resolution, background-position, repeat modes, auto margins, and header/topbar CSS behaviors. These are the biggest remaining visual gap after layout boxes are placed correctly.

Exit criteria:

- Vote arrows draw from the sprite and are centered in `.midcol`. (Reduced and old Reddit smoke coverage exists.)
- Header/logo sprite regions render.
- Submit button strips and nubs render close to reference.
- The `#sr-header-area .sr-list` BFC sits in one clipped topbar row beside the floated redesign button and subreddit dropdown, instead of overlapping `#header-bottom-left` or wrapping through later header rows.

### Phase 4: Defaults And Form Controls

Keep semantic HTML defaults independent from the app theme. `breeze.css` should remain a visual UI theme, not a behavior crutch for HTML content.

Exit criteria:

- Minimal HTML default-display tests pass without loading `breeze.css` for the covered elements.
- The old Reddit smoke test passes without loading `breeze.css`.
- Form controls in the comment box and sidebar match expected broad geometry; checkbox/radio checked state is now wired. Remaining work here is visual polish such as native checkbox/radio appearance, textarea resize affordance, and exact input/button paint.

### Phase 5: Full-Page Visual Gate

Once the page is close enough that screenshot diffs are meaningful, convert the smoke baseline into a real visual comparison against a curated expected image.

Exit criteria:

- Store an eepp expected output image only after review.
- Keep a separate Chrome reference image for human comparison.
- Pixel tolerance should be explicit and justified, because font rasterization and renderer backends may differ.

### Phase 6: Performance Gate

After the main geometry is correct, optimize invalidation and recomputation using the old Reddit fixture as a stress test.

Exit criteria:

- The opt-in old Reddit render reports stable, bounded layout/rebuild counts.
- Debug/ASAN remains tolerable enough for targeted investigation.
- Release render time is documented before and after optimization.
- Any caching preserves all reduced correctness tests.

## Standard Validation Gate

Before considering a phase complete:

```sh
make -C make/linux -j$(nproc)
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIHTMLFloat.*"
projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="UIHTML.redditOldThreadWebViewSmoke"
git diff --check
```

Run the full suite before landing broad layout changes.
