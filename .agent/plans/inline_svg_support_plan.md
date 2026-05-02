# Inline SVG Support & HTML Image Element Analysis Plan

This document outlines the architectural plan for adding inline `<svg>` HTML element support and analyzes whether a dedicated `UIHTMLImage` class is needed to improve `<img>` element behavior.

**AGENT DIRECTIVE:** You are Negen. Follow this plan iteratively. Compile and run unit tests after every step. Do NOT proceed if any regression is detected. Take git stash snapshots (`git stash push -m "Phase X.Y passed" && git stash apply`) on passing checkpoints.

---

## Part A: Current State Analysis

### A.1 How SVG Files Load via `<img src="file.svg">` Today

```
HTML: <img src="image.svg">
  → UIWidgetCreator::createFromName("img") → UIImage
  → UIImage::loadFromXmlNode → UIImage::applyProperty(PropertyId::Src)
  → DrawableImageParser::createDrawable(path) / DrawableSearcher::searchByName(path)
  → resolves file://, http://, data: URI
  → TextureFactory::loadFromFile/Memory → Image() → detects .svg extension
  → Image::svgLoad() → nanosvg parse + rasterize → RGBA pixels → Texture (GPU)
  → UIImage::setDrawable(texture) → draw() renders via OpenGL at widget size
```

### A.2 Why `<svg>` Inline Elements Fail Today

1. `UIWidgetCreator` has no `"svg"` registration (line 164 of widgetcreator.cpp)
2. When the HTML parser encounters `<svg>...</svg>`, it calls `createFromName("svg")` → returns `nullptr` → silently skipped
3. Even if a widget were created, the SVG's **children** (`<circle>`, `<rect>`, `<path>`, etc.) would be recursively loaded as HTML/UI widgets by the parent (since `loadsItsChildren()` would return false) — this would pollute the widget tree with garbage null lookups

### A.3 Existing SVG Infrastructure We Can Reuse

| Component | File | Role |
|---|---|---|
| nanosvg parser | `src/thirdparty/nanosvg/nanosvg.h` | Parses SVG XML to `NSVGimage` (paths, paints, gradients) |
| nanosvg rasterizer | `src/thirdparty/nanosvg/nanosvgrast.h` | Rasterizes to RGBA pixel buffer |
| `Image::svgLoad()` | `src/eepp/graphics/image.cpp:1008` | Parses + rasterizes SVG in a single call |
| `Image::getInfoFromMemory()` | `include/eepp/graphics/image.hpp:183` | Reads SVG intrinsic width/height without rasterizing |
| `TextureFactory::loadFromMemory()` | `include/eepp/graphics/texturefactory.hpp:77` | Creates GPU Texture from raw pixel data with `FormatConfiguration` (including `svgScale`) |
| `UISVGIcon` class | `include/eepp/ui/uiicon.hpp:50` | Rasterizes SVG XML on-demand at requested size (icons only) |
| `UIImage` class | `include/eepp/ui/uiimage.hpp` | Drawable-based rendering with scale types, alignment, tinting, aspect-ratio-preserving auto-sizing |
| `DrawableSearcher::searchByName()` | `src/eepp/graphics/drawablesearcher.cpp` | Handles `data:image/svg+xml,...` URIs in CSS `url()` |
| `UISceneNode::getThreadPool()` | `src/eepp/ui/uiscenenode.cpp:470` | Thread pool for async operations |
| `UIImageViewer::loadImageAsync()` | `src/eepp/ui/tools/uiimageviewer.cpp:100` | Proven async image loading pattern (thread pool + Sprite ownership) |

### A.4 Key Class Hierarchy (What UISvg Needs to Fit Into)

```
UINode
 └── UIWidget                  ← default SizePolicy::WrapContent (width + height)
      ├── UIImage              ← mDrawable, mScaleType, mColor, onAutoSize(), calcDestSize(), draw()
      │     └── UISvg (NEW)    ← our new class
      └── UILayout
            └── UIHTMLWidget   ← CSSDisplay, CSSPosition, layouter integration
                  ├── UIRichText  ← rebuildRichText() processes inline/block children
                  └── UITextSpan
```

**Key insight:** `UISvg` inherits from `UIImage` (not `UIHTMLWidget`). This means:
- Reuses all drawing, scaling, and alignment code
- Does NOT participate in the CSS display/position system (treated as a "custom" widget in rich text flow)
- In `rebuildRichText()`, it's classified as `isBlock` only if `mWidthPolicy == MatchParent`, otherwise inline — which matches HTML's default inline-block behavior for `<svg>`

---

## Part B: Implementation Plan — UISvg Widget

### Phase 1: Core UISvg Class

#### Step 1.1: Add `UI_TYPE_SVG` to UINodeType Enum

**File:** `include/eepp/ui/uihelper.hpp`

Insert `UI_TYPE_SVG` after `UI_TYPE_HTML_LIST_ITEM` (line 131), before `UI_TYPE_MODULES`:

```cpp
UI_TYPE_HTML_LIST_ITEM,
UI_TYPE_SVG,          // NEW
UI_TYPE_MODULES = 10000,
```

#### Step 1.2: Create UISvg Header

**File:** `include/eepp/ui/uisvg.hpp` (NEW)

```cpp
#ifndef EE_UI_UISVG_HPP
#define EE_UI_UISVG_HPP

#include <eepp/ui/uiimage.hpp>

namespace EE { namespace UI {

class EE_API UISvg : public UIImage {
  public:
    static UISvg* New();

    virtual ~UISvg();

    virtual Uint32 getType() const;

    virtual bool isType( const Uint32& type ) const;

    virtual void loadFromXmlNode( const pugi::xml_node& node );

    const std::string& getSvgXml() const;

  protected:
    UISvg();

    void onSizeChange() override;

    std::string mSvgXml;
    Uint64 mTag{ 0 }; // async task tag for cleanup on destruction

    static const Action::UniqueID sRasterizeId;

    void loadSvgXml( const pugi::xml_node& node );
    void scheduleRasterize();
    void rasterizeSvg( const std::string& svgXml );
    void clearThreadTag();
};

}} // namespace EE::UI

#endif
```

**Design decisions:**
- Inherits from `UIImage` (not `UIHTMLWidget`) — simpler, reuses all rendering/scaling/alignment code
- Stores raw SVG XML in `mSvgXml` for re-rasterization when the widget resizes
- Overrides `loadFromXmlNode` to capture the SVG subtree and trigger rasterization
- Overrides `onSizeChange` to schedule async re-rasterization with debounce
- `getType()` returns `UI_TYPE_SVG` for type-checking (e.g., `widget->isType(UI_TYPE_SVG)`)
- Thread pool task tag stored in `mTag` for cleanup in destructor

#### Step 1.3: Create UISvg Implementation

**File:** `src/eepp/ui/uisvg.cpp` (NEW)

**Constructor:**
```cpp
UISvg::UISvg() : UIImage() {
    // Prevent parent from recursively loading SVG children as UI widgets
    mFlags |= UI_LOADS_ITS_CHILDREN;
}
```

**Destructor:**
```cpp
UISvg::~UISvg() {
    clearThreadTag();
}
```

**loadFromXmlNode override:**
```cpp
void UISvg::loadFromXmlNode( const pugi::xml_node& node ) {
    // Process regular attributes (style, id, class, width, height, etc.)
    beginAttributesTransaction();
    UIWidget::loadFromXmlNode( node );
    endAttributesTransaction();

    // Serialize the <svg> subtree to string
    loadSvgXml( node );

    // Kick off async rasterization
    scheduleRasterize();
}
```

**XML serialization helper:**
```cpp
// Simple pugi::xml_writer that accumulates into a std::string
class XmlStringWriter : public pugi::xml_writer {
  public:
    std::string result;
    virtual void write( const void* data, size_t size ) override {
        result.append( static_cast<const char*>( data ), size );
    }
};

void UISvg::loadSvgXml( const pugi::xml_node& node ) {
    XmlStringWriter writer;
    node.print( writer );
    mSvgXml = writer.result;
}
```

**Async rasterization schedule (initial load + size changes):**
```cpp
void UISvg::scheduleRasterize() {
    if ( mSvgXml.empty() )
        return;

    auto size = getPixelsSize();
    if ( size.getWidth() <= 0.f || size.getHeight() <= 0.f )
        return;

    if ( !getUISceneNode()->hasThreadPool() )
        return;

    clearThreadTag();

    std::string svgXml( mSvgXml );
    auto pixelDensity = PixelDensity::getPixelDensity();

    mTag = getUISceneNode()->getThreadPool()->run(
        [this, svgXml = std::move( svgXml ), pixelDensity] {
            rasterizeSvg( svgXml );
        },
        []( const Uint64& ) {},
        (Uint64)this ); // tag by `this` pointer to allow cancelling
}
```

**Rasterization (runs on thread pool):**
```cpp
void UISvg::rasterizeSvg( const std::string& svgXml ) {
    Image::FormatConfiguration format;
    format.svgScale( PixelDensity::getPixelDensity() );

    // Determine target pixel size for rasterization:
    // Use the widget's content size at pixel density, or intrinsic SVG size
    Texture* texture = TextureFactory::instance()->loadFromMemory(
        (const unsigned char*)svgXml.data(), svgXml.size(),
        false,                              // mipmap
        Texture::ClampMode::ClampToEdge,    // clamp mode
        false, false,                       // compress, keepLocalCopy
        format );

    if ( !texture )
        return;

    // Wrap in Sprite to handle TextureFactory ownership lifecycle properly.
    // Sprite will remove the texture from TextureFactory and delete it when
    // destroyed. UIImage takes ownership of the Sprite via setDrawable(true).
    Sprite* sprite = Sprite::New();
    sprite->createStatic( texture );
    sprite->setAsTextureOwner( true );
    sprite->setAsTextureRegionOwner( true );

    runOnMainThread( [this, sprite] {
        // Use the widget's content size to compute the correct drawable scale.
        // The actual SVG intrinsic size determines the drawable's pixel dimensions,
        // while the widget's layout size determines the on-screen display bounds.
        setDrawable( sprite, true ); // UISvg owns the Sprite → Sprite owns the Texture
    } );
}
```

**onSizeChange override (debounced re-rasterization):**
```cpp
void UISvg::onSizeChange() {
    UIImage::onSizeChange();

    auto size = getPixelsSize();
    if ( size.getWidth() <= 0.f || size.getHeight() <= 0.f )
        return;

    // Debounce: cancel any pending rasterization and schedule a new one.
    // Node::debounce() automatically cancels the previous call with the same
    // uniqueIdentifier if called again before the delay expires.
    debounce( [this] { scheduleRasterize(); },
              Milliseconds( 150 ),
              sRasterizeId );
}

// In the .cpp file:
const Action::UniqueID UISvg::sRasterizeId = String::hash( "UISvg_rasterize" );
```

**Thread tag cleanup:**
```cpp
void UISvg::clearThreadTag() {
    if ( mTag != 0 && getUISceneNode()->hasThreadPool() ) {
        getUISceneNode()->getThreadPool()->removeWithTag( (Uint64)this );
        mTag = 0;
    }
}
```

**Important notes on `UI_LOADS_ITS_CHILDREN`:**
- Setting this flag tells `UIRichText::loadFromXmlNode()` and `UISceneNode::loadNode()` to skip recursive child processing for the SVG node
- Without this flag, the parent would try to create widgets for `<circle>`, `<rect>`, `<path>` etc. — all of which are unknown to `UIWidgetCreator` and would fail silently (but still waste cycles)
- The SVG is NOT expected to contain child elements that should become UI widgets

#### Step 1.4: Register `"svg"` in UIWidgetCreator

**File:** `src/eepp/ui/uiwidgetcreator.cpp`

Add after the existing `"img"` registration (line 168):

```cpp
registeredWidget["svg"] = [] {
    auto svg = UISvg::New();
    svg->setFlags( UI_HTML_ELEMENT );
    return svg;
};
```

This makes `<svg>...</svg>` elements in HTML content create `UISvg` widgets flagged as HTML elements (so the rich text engine treats them appropriately).

#### Step 1.5: Update Makefiles (premake4)

Since we added new `.hpp` and `.cpp` files, regenerate makefiles:
```
premake4 --disable-static-build --with-mold-linker --with-debug-symbols --address-sanitizer gmake
```

**Validation:** Run `make -C make/linux -j$(nproc)` and ensure clean compile. (Snapshot)

#### Step 1.6: Unit Test

**File:** `src/tests/unit_tests/` (specific file TBD, likely create `htmlsvg.cpp` or extend existing HTML tests)

Test at minimum:
1. **Basic inline SVG rendering:** `<svg width="100" height="100"><circle cx="50" cy="50" r="40" fill="red"/></svg>`
2. **SVG with viewBox:** `<svg viewBox="0 0 200 200"><rect width="100" height="100" fill="blue"/></svg>`
3. **CSS sizing on SVG:** `<svg style="width: 200px; height: 150px;">...</svg>`
4. **SVG with xmlns:** `<svg xmlns="http://www.w3.org/2000/svg">...</svg>`
5. **Verification that SVG children are NOT created as UI widgets**
6. **Resize re-rasterization:** Verify the SVG re-renders crisply after resizing the widget

Reference existing SVG test asset: `bin/unit_tests/assets/html/triangle.svg`

**Validation:** Run `ASAN_OPTIONS=detect_leaks=0 xvfb-run -a -s "-screen 0 1280x1024x24" bin/unit_tests/eepp-unit_tests-debug --filter="Svg"` — must pass. (Snapshot)

---

### Phase 2: Edge Cases & Polish

#### Step 2.1: Handle SVG Without Intrinsic Dimensions

Some SVGs lack explicit `width`/`height` attributes. In this case, fall back to the widget's content size or a reasonable default (e.g., 300×150, matching browser defaults for replaced elements).

#### Step 2.2: Handle SVG with viewBox Only

When the SVG has a `viewBox` attribute (e.g., `viewBox="0 0 200 150"`) but no `width`/`height`, the intrinsic aspect ratio should come from the viewBox dimensions. Nanosvg's `Image::getInfoFromMemory` handles this.

#### Step 2.3: HiDPI / Pixel Density

The `svgScale` in `Image::FormatConfiguration` handles this:
- `format.svgScale( PixelDensity::getPixelDensity() )`
- For a device with 2× pixel density, the SVG renders at 2× pixel resolution
- The widget's logical size remains in CSS pixel units

This is correctly handled in the rasterization code above.

#### Step 2.4: SVG with Internal `<style>` / CSS

Nanosvg supports inline styles and the `<style>` element. No special handling needed — the SVG XML serialization preserves all content.

#### Step 2.5: SVG with `<use>` / External References

Nanosvg may not fully support external references (xlink). This is a known limitation inherited from nanosvg, not from our implementation. Document as a known limitation.

#### Step 2.6: Sync Fallback When Thread Pool Unavailable

When `!hasThreadPool()`, rasterize synchronously on the main thread directly in `scheduleRasterize()`. This ensures the SVG still renders in environments without a thread pool.

---

## Part C: UIHTMLImage Analysis & Recommendation

### C.1 Current `UIImage` Behavior as `<img>` Element

| Aspect | Current Implementation | HTML Spec Behavior | Match? |
|---|---|---|---|
| Intrinsic sizing | `onAutoSize()` uses drawable dimensions | Replaced element intrinsic dimensions | ✓ |
| CSS `width: 200px` only | Height auto-computed from aspect ratio | Same | ✓ |
| CSS `height: 200px` only | Width auto-computed from aspect ratio | Same | ✓ |
| Both WrapContent | Sizes to drawable dimensions | Same | ✓ |
| Max-width constraint | Respected in `onAutoSize()` | Same | ✓ |
| `scale-type` | `FitInside`/`Expand`/`None` | Maps to `object-fit` (approximated) | ≈ |
| `text-align` | Used for horizontal alignment | CSS `text-align` on inline elements | ✓ |
| Default display flow | Inline (WrapContent width → `isBlock=false`) | Inline-block | ≈ |
| `alt` attribute | Registered as `tooltip` alias (tooltip text only) | Text fallback when image fails to load | ✗ |
| HTML `width`/`height` attrs | Treated as CSS width/height (Fixed policy) | Presentational hints separate from CSS | ≈ |
| `srcset`/`sizes` | Not supported | Responsive images | ✗ |
| `loading="lazy"` | Not supported | Deferred loading | ✗ |

### C.2 Note on `alt` Attribute

The `alt` attribute is already registered as a tooltip alias in `propertydefinition.cpp`:
```
registerProperty( "tooltip", "" )
    .setType( PropertyType::String )
    .addAlias( "alt" );
```

This means that currently `<img alt="My Image">` simply sets a tooltip on the widget. It does NOT provide the HTML-spec fallback behavior (showing alt text when the image fails to load). Any UIHTMLImage implementation would need to separately handle the visual alt-text fallback.

### C.3 Gap Analysis

The most impactful gap is the **`alt` attribute fallback display**: when an image fails to load (e.g., broken URL), there's no visible indicator. Everything else is either already handled or an advanced feature.

The sizing behavior is already close to the HTML spec for common use cases. The default `WrapContent` policy on `UIWidget` ensures images display at their intrinsic size unless overridden by CSS, and aspect-ratio preservation works when only one dimension is specified.

### C.4 Recommendation: Create UIHTMLImage (Phase 3)

**Verdict: YES, create a dedicated `UIHTMLImage : public UIImage` class.** Reason:
1. Adding `alt` text fallback display is the most immediate improvement and justifies the class
2. It provides a clean extension point for future HTML-specific image features
3. It separates concerns: HTML semantics can evolve without touching `UIImage`'s general-purpose code
4. Low-risk: it's a thin wrapper with one added feature

#### UIHTMLImage Class Design:

```cpp
class EE_API UIHTMLImage : public UIImage {
  public:
    static UIHTMLImage* New();

    virtual Uint32 getType() const;
    virtual bool isType( const Uint32& type ) const;

    virtual void loadFromXmlNode( const pugi::xml_node& node );
    virtual void draw();

    const std::string& getAlt() const;
    UIHTMLImage* setAlt( const std::string& alt );

  protected:
    UIHTMLImage();

    std::string mAlt;
    UITextView* mAltLabel{ nullptr };

    void createAltLabel();
    void removeAltLabel();
};
```

**loadFromXmlNode override:**
```cpp
void UIHTMLImage::loadFromXmlNode( const pugi::xml_node& node ) {
    // Read alt attribute before base class processing.
    // Note: "alt" is already registered as a tooltip alias in propertydefinition,
    // so the base class will handle it as a tooltip. We separately capture mAlt
    // for the visual fallback display.
    for ( auto& attr : node.attributes() ) {
        if ( String::iequals( attr.name(), "alt" ) ) {
            mAlt = attr.value();
            break;
        }
    }

    beginAttributesTransaction();
    UIWidget::loadFromXmlNode( node );
    endAttributesTransaction();

    // If image failed to load (no drawable) and alt text exists, show alt label
    if ( !mDrawable && !mAlt.empty() ) {
        createAltLabel();
    } else if ( mDrawable && mAltLabel ) {
        removeAltLabel();
    }
}
```

**alt text fallback:**
- Create a `UITextView` child widget positioned over the image area
- Show it only when `mDrawable` is null and `mAlt` is non-empty
- The text view displays the alt text with appropriate styling (centered, italic, gray)
- Override `draw()` to show either the image or the alt text
- On drawable resource change (image loads later or reloads), remove the alt label

**Registration replacement in UIWidgetCreator:**
```cpp
// Replace this (line 164):
registeredWidget["img"] = [] {
    auto img = UIImage::NewWithTag( "img" );
    img->setFlags( UI_HTML_ELEMENT );
    return img;
};

// With this:
registeredWidget["img"] = [] {
    auto img = UIHTMLImage::New();
    img->setFlags( UI_HTML_ELEMENT );
    return img;
};
```

**Additional note on CSS display:** Since `UIHTMLImage` inherits from `UIImage` (not `UIHTMLWidget`), it inherits the same inline behavior in `rebuildRichText()`. If full CSS `display` support is needed later, consider adding `UIHTMLWidget` to the inheritance chain (or using composition).

---

## Part D: Implementation Order

| Step | Description | Files | Risk |
|---|---|---|---|
| **P1.1** | Add `UI_TYPE_SVG` to `UINodeType` | `uihelper.hpp` | Low |
| **P1.2** | Create `UISvg` header | `uisvg.hpp` (NEW) | Low |
| **P1.3** | Create `UISvg` implementation | `uisvg.cpp` (NEW) | Medium |
| **P1.4** | Register `"svg"` in widget creator | `uiwidgetcreator.cpp` | Low |
| **P1.5** | Regenerate makefiles + compile | premake4 + make | Low |
| **P1.6** | Unit test for inline SVG | `src/tests/unit_tests/` | Medium |
| **P2.x** | Edge cases (no-intrinsic-dims, viewBox, HiDPI, sync fallback) | `uisvg.cpp` | Low-Medium |
| **P3.1** | Create `UIHTMLImage` class | `uihtmlimage.hpp/.cpp` (NEW) | Low |
| **P3.2** | Replace `img` registration | `uiwidgetcreator.cpp` | Low |
| **P3.3** | Unit test for alt text behavior | `src/tests/unit_tests/` | Low |

---

## Part E: Files Summary

### New Files
| File | Purpose |
|---|---|
| `include/eepp/ui/uisvg.hpp` | UISvg class declaration (inherits UIImage) |
| `src/eepp/ui/uisvg.cpp` | UISvg implementation (XML serialization, async SVG rasterization) |
| `include/eepp/ui/uihtmlimage.hpp` | UIHTMLImage class declaration (inherits UIImage, alt text fallback) |
| `src/eepp/ui/uihtmlimage.cpp` | UIHTMLImage implementation |
| `src/tests/unit_tests/htmlsvg.cpp` | Unit tests for inline SVG rendering |

### Modified Files
| File | Change |
|---|---|
| `include/eepp/ui/uihelper.hpp` | Add `UI_TYPE_SVG` to `UINodeType` enum |
| `src/eepp/ui/uiwidgetcreator.cpp` | Register `"svg"` → UISvg; replace `"img"` → UIHTMLImage |

---

## Part F: Potential Hazards

1. **pugi::xml_writer dependency:** The serialization code uses `pugi::xml_writer` which is provided by the included pugixml. No additional dependencies needed.

2. **Pixel vs DP math:** `UIImage::onAutoSize()` and `calcDestSize()` already use `mSize`, `mPaddingPx`, `getPixelsSize()`, and `mDrawable->getPixelsSize()` correctly. The SVG rasterization uses `PixelDensity::getPixelDensity()` for the scale factor. Ensure no dp/pixel confusion in the new code.

3. **Lifetime management — UISvg owns the drawable:**
   - `TextureFactory::loadFromMemory()` creates a Texture tracked by the factory with refCount=1
   - The texture is wrapped in a `Sprite`, and `Sprite::setAsTextureOwner(true)` makes the Sprite responsible for the Texture's lifetime
   - `setDrawable(sprite, true)` makes `UIImage` own the Sprite
   - When UISvg is destroyed: `~UIImage()` → `safeDeleteDrawable()` → `eeSAFE_DELETE(sprite)` → `~Sprite()` → `cleanUpResources()` → `eeSAFE_DELETE(texture)` → `~Texture()` → `TextureFactory::removeReference(this)` → refCount reaches 0 → factory removes entry → GPU texture deleted
   - **This follows the exact same pattern as `UIImageViewer::loadImageAsync()`** (`src/eepp/ui/tools/uiimageviewer.cpp:100`)

4. **XML format preservation:** pugi::xml_writer preserves the original formatting. The SVG content is byte-for-byte identical to the serialized XML subtree.

5. **`UI_LOADS_ITS_CHILDREN` side effects:** This flag is also checked by some generic code paths. Verify no negative side effects on layout, clipping, or hit testing.

6. **Thread safety — async rasterization:**
   - Rasterization runs on `UISceneNode::getThreadPool()` to avoid blocking the render loop
   - `TextureFactory::loadFromMemory()` is called on the thread pool (already proven by `UIImageViewer`)
   - The Sprite is constructed on the thread pool (also proven pattern)
   - `setDrawable()` and the subsequent `onAutoSize()` / `invalidateDraw()` run on the main thread via `runOnMainThread()`
   - Task cancellation: `mTag` tracks the last async task, cleared in destructor via `removeWithTag(this)` to prevent callbacks on destroyed widgets

7. **Large SVG files / Debounce on resize:**
   - Re-rasterization is triggered by `onSizeChange()` with a **150ms debounce** via `Node::debounce(cb, delay, uniqueIdentifier)` — if called again before the delay, the previous call is cancelled and the timer resets
   - Rasterization is **skipped** if widget has 0 width or height
   - Thread pool tag (`removeWithTag(this)`) ensures only the most recent rasterization completes (old tasks are cancelled)
   - For huge SVGs, the 150ms debounce prevents multiple expensive parses during rapid layout transitions

8. **Sync fallback:** When no thread pool is available (`!hasThreadPool()`), rasterization happens synchronously on the main thread in `scheduleRasterize()`. This ensures SVGs render in all environments but may cause a frame drop on load.
