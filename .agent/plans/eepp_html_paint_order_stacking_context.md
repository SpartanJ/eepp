# EEPP HTML Paint Ordering and Stacking Support Plan

## 1. Objective

Implement a limited, efficient CSS paint-order and stacking model for EEPP’s HTML compatibility layer.

The goal is not full browser-engine compliance. The goal is to support the most common ordering and hit-testing cases required by moderately complex traditional websites such as old Reddit, while preserving EEPP’s existing performance characteristics and avoiding a large parallel rendering architecture.

The first concrete compatibility target is the old Reddit layout:

```text
parent
├── .side      float: right
└── .content   later sibling, full parent width
```

The `.content` box geometrically overlaps the sidebar region and appears later in the node tree. EEPP currently hit-tests it first because hit-testing effectively follows reverse child order.

CSS painting order places the floating `.side` above the ordinary in-flow `.content` block. Therefore, `.side` and its descendants should also be hit-tested first.

This problem should be solved before attempting complete stacking-context support.

---

# 2. Design principles

## 2.1 Preserve the existing node tree

The existing `Node` and `UIHTMLWidget` hierarchy remains authoritative for:

* ownership;
* lifetime;
* layout;
* CSS inheritance;
* selectors;
* event propagation;
* relative coordinates;
* containing blocks;
* scrolling;
* transforms.

Nodes must not be reparented for painting.

No complete duplicate element or render tree should be introduced.

## 2.2 Separate paint order from node order

The node tree describes document and layout relationships.

It does not always describe final CSS painting order.

EEPP should derive a lightweight paint-order representation from the existing tree. Initially, this can remain local to individual containers. Later phases can add cross-parent stacking participation only where required.

## 2.3 Keep the common path unchanged

Containers that have none of the following should retain the current direct traversal path:

* floats;
* positioned children requiring special ordering;
* applicable `z-index`;
* flex or grid order modification;
* supported stacking groups.

For normal containers:

```cpp
UILayout::drawChildren();
UILayout::overFind();
```

should remain the effective implementation.

## 2.4 Drawing and hit-testing must share one order

Drawing traverses the computed paint order forwards.

Hit-testing traverses the exact same order backwards.

There must not be separate approximations for rendering and input.

## 2.5 Rebuild only when ordering changes

Paint-order data must be cached.

It should not be rebuilt:

* every frame;
* every draw call;
* every mouse move;
* every hit-test.

Ordering data should be invalidated only by structural or classification changes.

## 2.6 Prefer contiguous, non-owning data

Use:

* `Node*` or `UIHTMLWidget*` references;
* compact enums;
* integer tree-order values;
* `SmallVector`;
* document or widget-local caches;
* generation or dirty flags.

Avoid:

* one allocated paint object per DOM node;
* linked render-layer hierarchies;
* general-purpose polymorphic paint nodes;
* dynamic sorting in hot paths.

## 2.7 Prioritize practical compatibility

The implementation should first support:

* traditional block layouts;
* floats;
* positioned overlays;
* integer `z-index`;
* correct hit-testing;
* nested positioned stacking groups.

Modern stacking triggers should only be added when required by real target sites.

---

# 3. Existing EEPP foundation

`UIHTMLWidget` already contains important integration points:

* CSS `position`;
* numeric `z-index`;
* flex `order`;
* `drawChildren()`;
* `overFind()`;
* flags indicating that sorting is required;
* `buildDrawOrderVector()`, shared by drawing and hit-testing.

The current implementation collects direct children, optionally applies flex ordering, stable-sorts them by numeric `z-index`, draws from first to last, and hit-tests from last to first.

This is a useful foundation, but it currently assumes that child order plus numeric z-index is sufficient.

It does not represent CSS paint phases such as floats, and it cannot handle positioned descendants that participate in the stacking scope of a non-direct ancestor.

---

# 4. Supported scope

## Initial scope

The first implementation phases should support:

* ordinary DOM order;
* flex order-modified child order;
* block versus float paint ordering;
* positioned elements;
* `z-index: auto`;
* integer z-index values;
* negative, zero and positive positioned ordering;
* nested atomic positioned stacking groups;
* positioned descendants crossing non-stacking ancestors;
* drawing and hit-testing agreement.

## Initially excluded

The initial project should not attempt complete support for:

* opacity-created stacking contexts;
* transform-created stacking contexts;
* filters;
* masks;
* blend modes;
* `isolation`;
* `contain`;
* `will-change`;
* 3D transforms;
* compositing;
* complete inline-fragment stacking;
* all CSS Appendix E phases;
* browser-compatible outline painting;
* full pseudo-stacking behavior for every formatting construct.

The architecture should permit future expansion without requiring those features now.

---

# 5. Phase 1 — Introduce local CSS paint categories

## Goal

Fix common cases where direct sibling order differs from CSS paint order, especially the old Reddit float sidebar.

This phase should require no cross-parent paint promotion and no document-wide stacking tree.

## Initial classification

Introduce a compact category for direct paint participants:

```cpp
enum class HTMLPaintCategory : Uint8 {
    NegativePositioned,
    NormalFlow,
    Float,
    PositionedAutoOrZero,
    PositivePositioned,
};
```

The first implementation may internally use fewer categories if needed:

```cpp
enum class HTMLPaintCategory : Uint8 {
    NormalFlow,
    Float,
    Positioned,
};
```

However, using the more complete enum from the beginning will avoid changing the cache format immediately in Phase 2.

## Local paint item

```cpp
struct HTMLLocalPaintItem {
    Node* node{ nullptr };
    HTMLPaintCategory category{ HTMLPaintCategory::NormalFlow };
    int zIndex{ 0 };
    Uint32 treeOrder{ 0 };
};
```

The item is non-owning and compact.

## Ordering

The local order should be derived from:

1. paint category;
2. z-index where applicable;
3. stable CSS tree order.

For the old Reddit case:

```text
.content → NormalFlow
.side    → Float
```

Result:

```text
drawing:
    .content
    .side

hit-testing:
    .side
    .content
```

This resolves the overlap without changing geometry or layout.

## Float treatment

A non-positioned float should be treated as a local atomic paint participant in the float phase.

This does not mean the float creates a full stacking context.

Its ordinary subtree can still draw recursively as it does today.

In later phases, explicitly stacked descendants inside a float may participate in an ancestral stacking group.

## Container activation

A container needs local paint ordering when it contains at least one direct child that is:

* floated;
* positioned;
* carrying applicable z-index;
* flex/grid order-modified;
* later classified as a stacking-group root.

Ordinary containers retain the current traversal.

## Drawing

```cpp
for (const HTMLLocalPaintItem& item : paintOrder)
    item.node->nodeDraw();
```

## Hit-testing

```cpp
for (auto it = paintOrder.rbegin(); it != paintOrder.rend(); ++it) {
    if (Node* hit = it->node->overFind(point))
        return hit;
}
```

## Performance

* no cross-parent traversal;
* no new structure for ordinary containers;
* no per-frame allocation;
* a small cached vector only where special order exists;
* one category comparison per special child during rebuild.

## Tests

### Float over later block

Create two overlapping siblings:

```html
<div class="side"></div>
<div class="content"></div>
```

with `.side` floated and `.content` covering the parent width.

Verify:

* both bounds contain the test point;
* `.side` draws after `.content`;
* `.side` is hit-tested before `.content`;
* a link inside `.side` receives the hit.

### Equal-category ordering

Two floats with equal classification retain stable tree order.

### Mixed normal and float children

Normal blocks paint before floats regardless of direct child insertion order where CSS requires it.

### Old Reddit regression

Use the existing old Reddit fixture and assert that a point over a sidebar link resolves to that link instead of `.content`.

---

# 6. Phase 2 — Cache local paint order

## Goal

Ensure that Phase 1 does not introduce repeated sorting or classification work into drawing and hit-testing.

## Cache design

Add a lazily allocated cache only to containers requiring special order:

```cpp
struct UIHTMLPaintOrderCache {
    SmallVector<HTMLLocalPaintItem, 16> items;
    bool dirty{ true };
};
```

Possible ownership:

```cpp
UIHTMLPaintOrderCache* mPaintOrderCache{ nullptr };
```

This avoids increasing the footprint of every HTML widget by the size of a vector.

A container without special paint behavior has no cache allocation.

## API

```cpp
Span<const HTMLLocalPaintItem> getHTMLLocalPaintOrder();
void invalidateHTMLLocalPaintOrder();
bool needsHTMLLocalPaintOrder() const;
```

## Rebuild behavior

When dirty:

1. collect direct children;
2. assign CSS visual tree order;
3. classify each child;
4. stable-sort only if required;
5. retain allocated capacity.

When clean:

* drawing reads the cached order;
* hit-testing reads the same cached order in reverse.

## Fast path

```cpp
void UIHTMLWidget::drawChildren() {
    if (!needsHTMLLocalPaintOrder()) {
        UILayout::drawChildren();
        return;
    }

    drawCachedHTMLPaintOrder();
}
```

Equivalent behavior should apply to `overFind()`.

## Invalidation sources

Invalidate local paint order on:

* direct child insertion;
* removal;
* reparenting;
* float changes;
* position changes;
* z-index changes;
* flex/grid `order` changes;
* display changes that add or remove participation.

## Performance tests

Add counters in test or debug builds:

```cpp
Uint32 getPaintOrderRebuildCount() const;
```

Verify:

* repeated drawing does not rebuild;
* repeated mouse movement does not rebuild;
* unrelated style changes do not rebuild;
* one relevant property change causes one rebuild.

---

# 7. Phase 3 — Correct the `z-index` representation

## Goal

Represent the minimum CSS semantics required for positioned stacking behavior.

## Current limitation

A numeric field cannot distinguish:

```css
z-index: auto;
z-index: 0;
```

This distinction is essential.

An explicit zero can establish an atomic stacking group.

`auto` normally does not trap descendant stacking groups in the same way.

## Data type

Introduce:

```cpp
struct CSSZIndex {
    int value{ 0 };
    bool isAuto{ true };

    bool operator==(const CSSZIndex&) const = default;
};
```

Alternative:

```cpp
std::optional<int> mZIndex;
```

An explicit small value type is preferable because it makes the semantic meaning clear and avoids optional-related ambiguity in APIs.

## API

```cpp
const CSSZIndex& getCSSZIndex() const;
bool hasAutoZIndex() const;
int getZIndexValue() const;

void setZIndexAuto();
void setZIndex(int value);
```

Existing `getZIndex()` can remain temporarily for compatibility, but internal ordering code should use the tagged value.

## Serialization

```text
isAuto == true  → "auto"
isAuto == false → integer string
```

## Applicability

Introduce:

```cpp
bool UIHTMLWidget::isCSSPositioned() const;
bool UIHTMLWidget::hasApplicableZIndex() const;
```

Initial behavior:

```cpp
bool UIHTMLWidget::hasApplicableZIndex() const {
    if (mZIndex.isAuto)
        return false;

    return isCSSPositioned() || isFlexItem() || isGridItem();
}
```

This prevents ordinary static block elements from being reordered simply because they contain a numeric z-index property.

## Local paint classification

Classification now becomes more precise:

```text
positioned + negative z-index
    → NegativePositioned

float, otherwise
    → Float

positioned + auto or explicit zero
    → PositionedAutoOrZero

positioned + positive z-index
    → PositivePositioned

otherwise
    → NormalFlow
```

The exact ordering of explicit zero versus auto can remain locally grouped until atomic stacking groups are introduced in Phase 4.

## Tests

* default z-index is auto;
* explicit zero remains distinguishable;
* negative values parse and serialize correctly;
* static block plus z-index does not reorder;
* positioned explicit zero is classified separately from normal flow;
* switching auto to integer invalidates the order cache;
* switching position static to relative updates applicability.

---

# 8. Phase 4 — Add direct-child atomic stacking groups

## Goal

Support nested positioned groups among direct children without yet allowing descendants to escape non-group ancestors.

This provides a useful intermediate capability with relatively low complexity.

## Initial group creation rules

A direct child creates a supported atomic stacking group when:

* it is positioned and has explicit non-auto z-index;
* it is fixed;
* it is sticky;
* it is a flex item with explicit non-auto z-index;
* it is a grid item with explicit non-auto z-index.

The root document is always the root stacking scope.

## Helper

```cpp
bool UIHTMLWidget::createsSupportedStackingGroup() const;
```

## Atomic behavior

Consider:

```html
<div id="a">
    <div id="high"></div>
</div>

<div id="b"></div>
```

```css
#a {
    position: relative;
    z-index: 1;
}

#high {
    position: absolute;
    z-index: 1000;
}

#b {
    position: relative;
    z-index: 2;
}
```

The complete `#a` subtree remains below `#b`.

The high z-index is only meaningful inside `#a`’s group.

## Implementation

The local paint item gains a flag:

```cpp
enum HTMLPaintItemFlags : Uint8 {
    HTMLPaintItemNone = 0,
    HTMLPaintItemAtomicGroup = 1 << 0,
};
```

A direct child that creates a supported group is drawn as one atomic local item.

Its internal children still use its own local cached paint ordering.

No document-wide group tree is needed yet.

## Benefit

This phase establishes correct nested atomicity for direct-child contexts while retaining normal recursion.

## Limit

It does not solve:

```html
<div id="non-group-parent">
    <div id="high-z"></div>
</div>

<div id="medium-z"></div>
```

where `#high-z` should participate in an ancestor stacking scope.

That requires Phase 5.

---

# 9. Phase 5 — Add limited cross-parent stacking participation

## Goal

Support the common positioned-descendant case that cannot be represented by sorting direct children.

A positioned descendant of a non-stacking ancestor may need to participate in the nearest ancestral stacking group.

## Example

```html
<div id="section">
    <div id="popup"></div>
</div>

<div id="overlay"></div>
```

```css
#popup {
    position: absolute;
    z-index: 10;
}

#overlay {
    position: relative;
    z-index: 5;
}
```

If `#section` does not create a stacking group, `#popup` should paint above `#overlay`.

The current recursive model paints the entire `#section` subtree before `#overlay`, so local sorting is insufficient.

## Architecture

Introduce a small document-owned stacking-group index.

This is not a second node tree.

It contains only:

* the root group;
* supported real group roots;
* promoted positioned participants;
* ordering metadata.

## Data structures

```cpp
using HTMLStackingGroupId = Uint32;

static constexpr HTMLStackingGroupId InvalidHTMLStackingGroup =
    std::numeric_limits<HTMLStackingGroupId>::max();
```

```cpp
struct UIHTMLStackingItem {
    UIHTMLWidget* node{ nullptr };
    HTMLStackingGroupId childGroup{ InvalidHTMLStackingGroup };
    int zIndex{ 0 };
    Uint32 treeOrder{ 0 };
    HTMLPaintCategory category{ HTMLPaintCategory::NormalFlow };
};
```

```cpp
struct UIHTMLStackingGroup {
    UIHTMLWidget* owner{ nullptr };
    HTMLStackingGroupId parent{ InvalidHTMLStackingGroup };

    SmallVector<UIHTMLStackingItem, 8> negative;
    SmallVector<UIHTMLStackingItem, 8> zero;
    SmallVector<UIHTMLStackingItem, 8> positive;
};
```

The first implementation may use one item vector plus category keys if that is simpler.

## Group ownership

Each supported real group belongs atomically to its nearest ancestral real group.

Normal non-group ancestors do not create records.

## Build traversal

Perform a depth-first traversal when stacking data is dirty.

Maintain:

```cpp
struct HTMLStackBuildState {
    HTMLStackingGroupId currentGroup;
    Uint32 treeOrder;
};
```

### Real group

When a node creates a supported group:

1. create a group record;
2. add it as an atomic item in the current group;
3. traverse its descendants using the new group as current.

### Non-group ancestor

When a node does not create a group:

1. leave its ordinary drawing in the normal recursive path;
2. continue searching descendants for positioned participants;
3. assign those participants to the current ancestral group.

### Positioned `z-index:auto`

Initially:

* leave its ordinary box at the normal recursive location;
* do not create a real group;
* allow explicit descendant groups to continue participating in the ancestral group.

This preserves the important distinction between auto and explicit zero.

## Promote only special participants

Do not create stacking items for every node.

Collect only:

* real supported group roots;
* positioned participants requiring ancestral ordering;
* metadata required to skip their normal recursive draw.

All ordinary subtrees remain on the existing fast path.

---

# 10. Phase 5 drawing traversal

## Problem

A promoted positioned descendant must not be painted when ordinary recursion reaches its immediate parent.

It must instead be painted from its owning stacking group.

## Paint-manager state

Add a document-owned manager:

```cpp
class UIHTMLPaintManager {
  public:
    void invalidate();
    void rebuildIfNeeded();

    void drawDocument();
    Node* hitTestDocument(const Vector2f& point);

    bool shouldSkipDuringNormalTraversal(const UIHTMLWidget* node) const;

  private:
    SmallVector<UIHTMLStackingGroup, 8> mGroups;
    bool mDirty{ true };
};
```

## Skip promoted descendants

In the HTML child draw path:

```cpp
for (const HTMLLocalPaintItem& item : localPaintOrder) {
    if (paintManager &&
        paintManager->shouldSkipDuringNormalTraversal(
            item.node->asType<UIHTMLWidget>())) {
        continue;
    }

    item.node->nodeDraw();
}
```

The skip lookup must be cheap.

Possible metadata on promoted widgets:

```cpp
HTMLStackingGroupId mHTMLPaintGroup{ InvalidHTMLStackingGroup };
bool mHTMLPaintPromoted{ false };
Uint32 mHTMLPaintGeneration{ 0 };
```

Only `UIHTMLWidget` needs this state, not generic `Node`.

## Group drawing order

For the limited positioned implementation:

1. negative positioned groups;
2. owner’s normal recursive subtree;
3. auto and zero-level positioned participants;
4. positive positioned groups.

Pseudo-code:

```cpp
void UIHTMLPaintManager::drawGroup(HTMLStackingGroupId groupId) {
    UIHTMLStackingGroup& group = mGroups[groupId];

    drawItems(group.negative);

    drawNormalSubtree(group.owner, groupId);

    drawItems(group.zero);

    drawItems(group.positive);
}
```

A child group is fully painted atomically:

```cpp
if (item.childGroup != InvalidHTMLStackingGroup)
    drawGroup(item.childGroup);
else
    item.node->nodeDraw();
```

## Scope limitation

This phase should not attempt to split every widget into background, content and outline paint operations.

It implements the common positioned ordering model around existing recursive widget painting.

More exact CSS paint phases can be introduced later if fixtures require them.

---

# 11. Phase 5 hit-testing

Hit-testing uses the exact same group data in reverse order.

```cpp
Node* UIHTMLPaintManager::hitTestGroup(
    HTMLStackingGroupId groupId,
    const Vector2f& point) {

    UIHTMLStackingGroup& group = mGroups[groupId];

    if (Node* hit = hitTestItemsReverse(group.positive, point))
        return hit;

    if (Node* hit = hitTestItemsReverse(group.zero, point))
        return hit;

    if (Node* hit = hitTestNormalSubtree(group.owner, groupId, point))
        return hit;

    if (Node* hit = hitTestItemsReverse(group.negative, point))
        return hit;

    return hitTestOwnerSelf(group.owner, point);
}
```

The normal subtree hit-test must skip the same promoted descendants skipped during drawing.

## Required invariant

For any overlapping pair of interactive nodes:

```text
the one painted later must be tested earlier
```

This invariant should be tested directly.

---

# 12. Phase 6 — Refine floats and pseudo-stacking behavior

## Goal

Improve interactions between floats and explicit stacking descendants after the primary old Reddit case works.

## Float behavior

A float is not automatically a real stacking group.

Its ordinary subtree paints together in the float phase.

However, explicit positioned stacking groups inside the float may still participate in an ancestral real stacking group unless another supported property traps them.

## Initial implementation

For Phase 1:

* treat each direct float as an atomic local paint participant;
* draw its ordinary subtree recursively;
* hit-test it atomically in reverse phase order.

For Phase 6:

* allow the stacking-group collector to discover explicit descendant groups inside floats;
* promote those descendants where required;
* skip them from ordinary float recursion.

## Inline-blocks

Inline-blocks have similar pseudo-stacking behavior but are more entangled with `RichText`.

Do not include them until a real compatibility failure requires it.

## RichText interaction

EEPP’s inline content is often painted by `Graphics::RichText`, not independently by each DOM widget.

Therefore:

* ordinary inline spans should remain inside RichText painting;
* out-of-flow positioned children are already easier because they are excluded from normal RichText flow;
* positioned inline or atomic inline stacking should be deferred until required.

---

# 13. Flex and grid paint order

Flex `order` already modifies child drawing order in EEPP.

This should be treated as the CSS visual tree order used when assigning `treeOrder`.

## Local order

For flex and grid containers:

1. compute their CSS order-modified child sequence;
2. assign stable `treeOrder` values in that sequence;
3. classify by paint category;
4. sort by category and relevant z-index while preserving stable tree order.

## Cross-parent traversal

When the stacking-group collector traverses a flex or grid container, it should visit children in CSS paint order rather than raw linked-list order.

Expose a common helper:

```cpp
template <typename Callback>
void UIHTMLWidget::forEachChildInCSSPaintOrder(Callback&& callback);
```

This avoids duplicating ordering logic between:

* layout-related collection;
* local painting;
* stacking-group construction;
* hit-testing.

---

# 14. Invalidation strategy

## Document-level state

Start with a single stacking-order dirty flag:

```cpp
bool mHTMLStackingDirty{ true };
```

Split it only after profiling demonstrates benefit.

## Local cache invalidation

A container’s local paint-order cache is invalidated by:

* direct child insertion;
* removal;
* reparenting;
* float changes;
* position changes;
* z-index changes;
* flex/grid order changes;
* display changes;
* child group classification changes.

## Document stacking invalidation

The document stacking index is invalidated by:

* any HTML node insertion or removal;
* reparenting;
* `position`;
* z-index auto/value;
* float changes where descendant traversal changes;
* display;
* flex/grid item status;
* flex/grid `order`;
* fixed/sticky classification;
* future stacking-trigger properties.

## Changes that should not invalidate order

* colors;
* background images;
* border colors;
* ordinary dimensions;
* ordinary positional movement after layout;
* text color;
* font changes;
* hover styles that do not affect classification.

## Rebuild timing

Rebuild lazily once before either:

* document drawing;
* document hit-testing.

It must not rebuild once for drawing and again for input in the same generation.

---

# 15. Memory and performance strategy

## No complete duplicate tree

The stacking manager stores only:

* actual supported group roots;
* promoted positioned participants;
* small ordering records.

Normal HTML nodes remain absent from the stacking structure.

## No per-frame allocation

Vectors retain capacity across rebuilds.

```cpp
mGroups.clear();
mItems.clear();
```

should not free memory each time.

## No per-hit-test sorting

All ordering is precomputed.

Hit-testing only traverses cached arrays backwards.

## No generic `Node` expansion

Do not add stacking metadata to the base `Node`.

Keep it in:

* `UIHTMLWidget`;
* the document HTML paint manager;
* optional local caches.

## Document fast path

```cpp
bool UIHTMLPaintManager::hasSpecialPaintOrder() const;
```

When false:

* use ordinary recursive drawing;
* use ordinary hit-testing;
* skip all stacking-group traversal.

## Container fast path

A container with no float, positioned participant, z-index, or order modification should not allocate a local cache.

## Sorting strategy

Most special containers are expected to have few relevant children.

Use stable sorting during rebuild.

Do not introduce more complex bucket data structures until profiling shows sorting is significant.

For a small number of categories, a linear stable partition into phase vectors may eventually outperform general sorting:

```cpp
negative.push_back(...)
normal.push_back(...)
floats.push_back(...)
zero.push_back(...)
positive.push_back(...)
```

This may be preferable from the beginning because the number of phases is fixed.

Within negative and positive groups, sort only those vectors by z-index.

This avoids sorting ordinary blocks and floats entirely.

---

# 16. Recommended local cache layout

A phase-based cache is likely more efficient than one globally sorted vector:

```cpp
struct UIHTMLPaintOrderCache {
    SmallVector<Node*, 8> negative;
    SmallVector<Node*, 16> normal;
    SmallVector<Node*, 8> floats;
    SmallVector<Node*, 8> zero;
    SmallVector<Node*, 8> positive;

    bool dirty{ true };
};
```

Drawing:

```cpp
draw(negative);
draw(normal);
draw(floats);
draw(zero);
draw(positive);
```

Hit-testing:

```cpp
hitReverse(positive);
hitReverse(zero);
hitReverse(floats);
hitReverse(normal);
hitReverse(negative);
```

Advantages:

* no category comparison during draw or hit-test;
* no sort for normal or float phases;
* only negative and positive phases need z-index sorting;
* clear mapping to supported CSS paint behavior.

Disadvantage:

* several `SmallVector` objects per allocated cache.

Because the cache is allocated only for special containers, this tradeoff is likely acceptable.

An alternative is one vector of compact items. The final choice should be based on `SmallVector` object size and typical HTML container counts.

---

# 17. Debugging support

Add debug support before cross-parent promotion.

## APIs

```cpp
void dumpHTMLLocalPaintOrder() const;
void dumpHTMLStackingGroups() const;

std::vector<Node*> debugGetHTMLPaintOrder() const;
std::vector<Node*> debugGetHTMLHitTestOrder() const;
```

## Example output

```text
Container: body
  NormalFlow:
    .content
  Float:
    .side
  PositionedAutoOrZero:
    #header
  PositivePositioned:
    #menu z=20
```

Stacking output:

```text
Root group: html
  normal subtree: body
  z=1 group: #header
  z=10 group: #dialog

Group: #header
  z=100 group: #dropdown
```

## Inspector information

Eventually expose:

* paint category;
* z-index auto/value;
* whether z-index applies;
* whether the node creates a supported group;
* nearest group owner;
* whether the node is promoted;
* local paint index;
* group paint index.

This is important for diagnosing site compatibility without inspecting renderer internals manually.

---

# 18. Test plan

## 18.1 Local float ordering

### Old Reddit shape

```html
<div id="parent">
    <div class="side">
        <a id="side-link">Link</a>
    </div>

    <div class="content"></div>
</div>
```

The test point must lie inside both `.side` and `.content`.

Assertions:

```cpp
EXPECT_TRUE(side->getWorldBounds().contains(point));
EXPECT_TRUE(content->getWorldBounds().contains(point));

EXPECT_EQ(document->overFind(point), sideLink);
```

Debug order:

```text
paint:
    content
    side

hit:
    side
    content
```

## 18.2 Float source-order variations

Place the float before and after the normal block and verify supported CSS phase order remains correct.

## 18.3 Multiple floats

Verify stable ordering between floats with equal classification.

## 18.4 Positioned positive ordering

Two overlapping positioned siblings with z-index 1 and 2.

The higher value paints later and receives input first.

## 18.5 Negative positioned ordering

Negative positioned content paints before normal flow according to the supported subset.

## 18.6 Auto versus zero

Explicit zero is distinguishable and can become atomic.

Auto does not trap descendant groups.

## 18.7 Atomic direct-child group

A child z-index 1000 remains inside a parent group at z-index 1 and below a sibling group at z-index 2.

## 18.8 Cross-parent promotion

A positioned z-index 10 descendant of a non-group ancestor paints above a sibling z-index 5 group.

## 18.9 Promotion through a float

An explicit positioned descendant inside a float participates in the appropriate ancestral group where supported.

## 18.10 Dynamic invalidation

At runtime change:

* float to none;
* position static to relative;
* z-index auto to zero;
* zero to positive;
* flex order;
* parent node.

Verify both drawing and hit-testing update.

## 18.11 Cache reuse

Repeated draw and hit-test operations must not rebuild unchanged caches.

## 18.12 Old Reddit fixture

Use the existing old Reddit WebView fixture and add a deterministic sidebar interaction regression.

Avoid relying only on screenshot comparison.

---

# 19. Milestones

## Milestone A — Local float-aware paint and hit order

Deliverables:

* `HTMLPaintCategory`;
* float versus normal-flow ordering;
* cached shared draw/hit sequence;
* old Reddit `.side` interaction regression.

No stacking-group tree.

Expected risk: low to moderate.

Expected benefit: directly fixes the stated old Reddit blocker.

## Milestone B — Correct z-index semantics

Deliverables:

* auto-or-integer z-index;
* applicability rules;
* negative, zero and positive local categories;
* parser and serialization tests;
* cache invalidation.

Expected risk: low.

## Milestone C — Direct-child atomic groups

Deliverables:

* supported group classification;
* explicit zero atomicity;
* nested local group behavior;
* direct sibling stacking correctness.

Expected risk: moderate.

## Milestone D — Cross-parent positioned stacking

Deliverables:

* document-owned lightweight group index;
* positioned descendant promotion;
* skip-promoted-node traversal;
* atomic nested groups;
* reversed group hit-testing;
* debug group dump.

Expected risk: high relative to previous phases, but still bounded.

## Milestone E — Float and formatting refinements

Deliverables:

* explicit stacking descendants inside floats;
* flex/grid traversal integration;
* selected inline-block behavior where required;
* traditional-site fixture validation.

Expected risk: moderate.

## Milestone F — Optional modern stacking triggers

Only when demanded by real sites:

* transforms;
* opacity;
* containment;
* isolation;
* filters and effects.

Expected risk: feature-dependent and potentially high.

---

# 20. Recommended implementation order

1. Add a focused test reproducing `.side` versus `.content`.
2. Add local paint categories.
3. Make float paint order override raw child order.
4. Use the same order for hit-testing.
5. Add lazy local paint-order caching.
6. Verify old Reddit sidebar interaction.
7. Represent `z-index:auto` correctly.
8. Add applicable negative, zero and positive positioned phases.
9. Add direct-child atomic stacking groups.
10. Add document-level stacking invalidation.
11. Implement cross-parent positioned participant collection.
12. Skip promoted descendants during normal recursive drawing.
13. Traverse stacking groups forwards for drawing.
14. Traverse them backwards for hit-testing.
15. Integrate float descendants, flex and grid ordering as fixtures require.
16. Profile memory, cache rebuilds and traversal overhead.
17. Only then consider transforms, opacity and other modern triggers.

---

# 21. Performance acceptance criteria

The implementation should meet these constraints:

* no paint-order sorting every frame;
* no sorting during mouse movement;
* no complete parallel node tree;
* no stacking allocation for ordinary containers;
* no generic `Node` size increase;
* no per-node paint object;
* cached capacities reused across rebuilds;
* local ordering proportional to direct special participants;
* document stacking data proportional to real groups and promoted participants;
* documents without special ordering use the existing fast path;
* drawing and hit-testing do not perform hash lookups for every ordinary node;
* old Reddit and similar pages show no material frame-time regression.

Suggested debug metrics:

```cpp
struct UIHTMLPaintStats {
    Uint32 localCaches;
    Uint32 localCacheRebuilds;
    Uint32 stackingGroups;
    Uint32 promotedNodes;
    Uint32 stackingRebuilds;
};
```

These should be inspectable in tests and performance diagnostics.

---

# 22. Final architecture target

```text
Existing UIHTMLWidget tree
    ├── ownership
    ├── layout
    ├── inheritance
    ├── event propagation
    ├── geometry
    └── ordinary recursive drawing

Optional local paint-order cache
    ├── negative positioned
    ├── normal flow
    ├── floats
    ├── zero/auto positioned
    └── positive positioned

Optional document stacking index
    ├── root stacking group
    ├── actual supported child groups
    ├── promoted positioned descendants
    ├── forward drawing order
    └── reverse hit-testing order
```

The implementation begins with a small local correction:

```text
normal blocks paint before floats
floats hit-test before overlapping normal blocks
```

It then evolves into limited positioned stacking only when direct local ordering is no longer sufficient.

This approach solves the old Reddit case early, minimizes the amount of changed code, preserves ordinary EEPP traversal where possible, and creates an efficient foundation for broader stacking support without committing EEPP to the full complexity of a browser engine.
