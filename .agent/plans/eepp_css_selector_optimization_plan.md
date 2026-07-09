# eepp CSS Selector Optimization Plan

## Goal

Optimize the CSS selector matching hot path in eepp's HTML compatibility layer and UI styling engine, especially for modern class-heavy pages. The plan prioritizes low-risk correctness fixes and micro-optimizations first, then moves toward larger algorithmic improvements such as class-based stylesheet indexing.

This plan is intended for execution by an AI coding agent against the `develop` branch of:

```text
https://github.com/SpartanJ/eepp
```

## Key Files

Primary files to inspect and modify:

```text
include/eepp/scene/node.hpp
src/eepp/scene/node.cpp
include/eepp/ui/uiwidget.hpp
src/eepp/ui/uiwidget.cpp
include/eepp/ui/css/stylesheet.hpp
src/eepp/ui/css/stylesheet.cpp
include/eepp/ui/css/stylesheetselector.hpp
src/eepp/ui/css/stylesheetselector.cpp
include/eepp/ui/css/stylesheetselectorrule.hpp
src/eepp/ui/css/stylesheetselectorrule.cpp
include/eepp/ui/css/stylesheetstyle.hpp
src/eepp/ui/css/stylesheetstyle.cpp
include/eepp/ui/css/elementdefinition.hpp
src/eepp/ui/css/elementdefinition.cpp
src/tests/unit_tests/uihtml_tests.cpp
```

Relevant current behavior:

- `StyleSheet::getElementStyles()` collects candidate style rules using only global, tag, id, and tag+id buckets.
- `StyleSheet::addStyleToNodeIndex()` indexes selectors by the rightmost selector rule's tag/id only.
- Class-only selectors such as `.button`, `.card`, `.post-title`, `.share-button` land in the broad/global bucket.
- `StyleSheetSelector::select()` performs generic right-to-left selector matching and repeatedly calls `StyleSheetSelectorRule::matches()`.
- `StyleSheetSelectorRule::matches()` does repeated string comparisons and linear class-vector scans.
- Node traversal helpers such as `Node::isWidget()` and `Node::isTextNode()` are tiny flag checks but currently implemented out-of-line in `node.cpp`.

## High-Level Strategy

Implement in phases:

1. Low-risk hot-path inlining and traversal cleanup.
2. Correct sibling combinator direction and general sibling behavior.
3. Fix universal selector early-return correctness.
4. Add lightweight profiling counters.
5. Add hash-based matching for tag/id/class.
6. Add class-based stylesheet indexing.
7. Add single-rule selector fast path.
8. Consider deeper caches and sorting reductions only after measuring.

Do not start with a large rewrite. Measure after each phase.

---

# Phase 1: Inline Tiny Hot-Path Accessors

## Motivation

CSS selector matching repeatedly calls small accessors and type checks while walking parents/siblings and matching selector rules. These functions are ideal candidates for header inlining.

Current examples:

```cpp
bool Node::isWidget() const {
	return 0 != ( mNodeFlags & NODE_FLAG_WIDGET );
}

bool Node::isTextNode() const {
	return 0 != ( mNodeFlags & NODE_FLAG_TEXTNODE );
}
```

These should be visible to the compiler at call sites.

## Tasks

Move or duplicate as inline definitions in headers, removing out-of-line definitions where required by ODR.

Recommended inline candidates:

```text
Node::isWidget()
Node::isTextNode()
Node::getNodeFlags()
Node::getNextNode()
Node::getPrevNode()
Node::getParent()
Node::getType() only if feasible and not virtual-dependent
UIWidget::getStyleSheetParentElement()
UIWidget::getStyleSheetPreviousSiblingElement()
UIWidget::getStyleSheetNextSiblingElement()
UIWidget::getStyleSheetPseudoClasses()
UIWidget::getElementTag()
UIWidget::getStyleSheetClasses()
```

Use existing eepp inline conventions. If there is already an `EE_FORCE_INLINE` macro, prefer it. If not, use plain `inline` first unless project style clearly supports a force-inline macro.

## Proposed CSS Traversal Helpers

Move these implementations into `include/eepp/ui/uiwidget.hpp` if dependencies allow it:

```cpp
inline UIWidget* UIWidget::getStyleSheetParentElement() const {
	return NULL != mParentNode && mParentNode->isWidget() && getType() != UI_TYPE_HTML_HTML
			   ? mParentNode->asType<UIWidget>()
			   : NULL;
}

inline UIWidget* UIWidget::getStyleSheetPreviousSiblingElement() const {
	Node* node = mPrev;
	while ( NULL != node ) {
		if ( node->isWidget() && !node->isTextNode() )
			return node->asType<UIWidget>();
		node = node->getPrevNode();
	}
	return NULL;
}

inline UIWidget* UIWidget::getStyleSheetNextSiblingElement() const {
	Node* node = mNext;
	while ( NULL != node ) {
		if ( node->isWidget() && !node->isTextNode() )
			return node->asType<UIWidget>();
		node = node->getNextNode();
	}
	return NULL;
}
```

## Important Design Constraint

Do not exclude non-HTML eepp GUI widgets from selector traversal. The HTML compatibility layer intentionally allows eepp GUI widgets to exist inside the HTML tree and be matched by selectors.

Parent traversal should stop specifically at `UI_TYPE_HTML_HTML`, not at every `UI_HTML_ELEMENT` boundary.

Sibling traversal should skip text nodes, not non-HTML widgets.

## Validation Tests

Add or update tests for:

### HTML root boundary

```html
<html>
	<body>
		<a class="goog-inline-block"></a>
	</body>
</html>
```

```css
.goog-inline-block { display: inline-block; }
* html .goog-inline-block { display: inline; }
```

Expected computed display:

```text
inline-block
```

### Text nodes do not block sibling selectors

```html
<div id="a"></div>
text
<div id="b"></div>
```

```css
#a + #b { color: red; }
```

Expected: `#b` matches the rule despite the text node.

### Non-HTML eepp widget remains selector-visible

Create an HTML tree containing a normal eepp widget between/inside HTML elements. Verify CSS selectors can still match it if exposed as a widget element.

---

# Phase 2: Fix Confirmed Sibling Combinator Direction Bug

## Motivation

This is a confirmed correctness bug, not intentional behavior. Right-to-left selector matching for CSS combinators should follow CSS semantics:

```css
A + B
```

When matching from `B`, inspect the previous element sibling and test it against `A`.

```css
A ~ B
```

When matching from `B`, scan previous element siblings for `A`.

Current code is inconsistent and incorrect:

- `StyleSheetSelector::select()` uses `getStyleSheetNextSiblingElement()` for `DIRECT_SIBLING`.
- `StyleSheetSelector::getRelatedElements()` uses `getStyleSheetPreviousSiblingElement()` for `DIRECT_SIBLING`.
- `SIBLING` scans both previous and next siblings, but CSS `~` should only scan previous siblings in right-to-left matching.

## Tasks

Apply standard CSS semantics directly. Matching runs from right to left, so sibling combinators must move toward previous element siblings for normal CSS `+` and `~`.

Required behavior:

```cpp
case StyleSheetSelectorRule::DIRECT_SIBLING: {
	curElement = curElement->getStyleSheetPreviousSiblingElement();

	if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
		return false;

	break;
}

case StyleSheetSelectorRule::PREVIOUS_SIBLING: {
	curElement = curElement->getStyleSheetNextSiblingElement();

	if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
		return false;

	break;
}
```

For general sibling:

```cpp
case StyleSheetSelectorRule::SIBLING: {
	bool foundSibling = false;

	for ( UIWidget* sibling = curElement->getStyleSheetPreviousSiblingElement();
		  NULL != sibling;
		  sibling = sibling->getStyleSheetPreviousSiblingElement() ) {
		if ( selectorRule.matches( sibling, applyPseudo ) ) {
			curElement = sibling;
			foundSibling = true;
			break;
		}
	}

	if ( !foundSibling )
		return false;

	break;
}
```

Keep the eepp-specific `PREVIOUS_SIBLING` / `|` operator as the inverse sibling direction if still needed. In right-to-left matching, it should use `getStyleSheetNextSiblingElement()`.

Also update `StyleSheetSelector::getRelatedElements()` to use the same direction semantics as `select()`, because non-cacheable style subscriptions depend on related-element discovery.

## Validation Tests

```html
<div id="a"></div>
<div id="b"></div>
```

```css
#a + #b { color: red; }
#b + #a { color: blue; }
```

Expected:

- `#b` matches `#a + #b`.
- `#a` does not match `#b + #a`.

For general sibling:

```html
<div id="a"></div>
<span></span>
<div id="b"></div>
```

```css
#a ~ #b { color: red; }
#b ~ #a { color: blue; }
```

Expected:

- `#b` matches `#a ~ #b`.
- `#a` does not match `#b ~ #a`.

---

# Phase 3: Fix Universal Selector Early Return

## Motivation

`StyleSheetSelectorRule::matches()` currently returns true early for `*` when `applyPseudo == false`.

This is only safe for a plain universal selector:

```css
*
```

It is not safe for selectors like:

```css
*.foo
*[href]
*:not(...)
```

Those must still check class, id, attributes, and structural pseudo selectors when relevant.

## Task

Replace the early return with a safe fast path only for pure universal selectors.

Current risky shape:

```cpp
if ( mTagName == "*" ) {
	if ( !applyPseudo ) {
		return true;
	} else {
		flags |= TagName;
	}
}
```

Safer approach:

```cpp
if ( !mTagName.empty() ) {
	if ( mTagName != "*" ) {
		if ( mTagName != element->getElementTag() )
			return false;
		flags |= TagName;
	} else {
		flags |= TagName;

		const Uint32 nonPseudoReq =
			mRequirementFlags & ~PseudoClass & ~StructuralPseudoClass;

		if ( !applyPseudo && nonPseudoReq == TagName )
			return true;
	}
}
```

Or remove the early return entirely until profiling proves it is necessary.

## Validation Tests

```html
<div class="foo"></div>
<div></div>
```

```css
*.foo { color: red; }
```

Expected: only the first div matches.

```html
<a href="x"></a>
<a></a>
```

```css
*[href] { color: red; }
```

Expected: only the first anchor matches.

---

# Phase 4: Add Selector-Matching Instrumentation

## Motivation

Before larger changes, collect hard numbers. Modern pages can behave very differently from simple UI stylesheets.

## Task

Add optional compile-time or runtime counters. Keep disabled by default.

Example structure:

```cpp
struct StyleMatchStats {
	Uint64 elementsStyled = 0;
	Uint64 candidateRules = 0;
	Uint64 selectorCalls = 0;
	Uint64 matchedRules = 0;
	Uint64 descendantSteps = 0;
	Uint64 siblingSteps = 0;
	Uint64 classChecks = 0;
	Uint64 attrChecks = 0;
	Uint64 cacheHits = 0;
	Uint64 cacheMisses = 0;
	Uint64 globalBucketCandidates = 0;
	Uint64 tagBucketCandidates = 0;
	Uint64 idBucketCandidates = 0;
	Uint64 classBucketCandidates = 0;
};
```

Instrument:

```text
StyleSheet::getElementStyles()
StyleSheetSelector::select()
StyleSheetSelectorRule::matches()
```

Collect at least:

- Number of elements styled.
- Number of candidate rules tested.
- Number of selector calls.
- Number of successful selector matches.
- Number of descendant traversal steps.
- Number of sibling traversal steps.
- Number of class checks.
- Number of attribute checks.
- Node cache hits/misses.
- Candidate counts per bucket.

## Expected Findings

Likely bottlenecks on modern pages:

- Global bucket candidates are very high.
- Class-only selectors dominate candidate rule count.
- Descendant selectors cause repeated ancestor walks.
- Class matching causes many string comparisons.
- Attribute selectors are less frequent but expensive per check.

Use these counters to verify each later phase.

---

# Phase 5: Add Hash-Based Tag/ID/Class Matching

## Motivation

`StyleSheetSelectorRule::matches()` currently compares strings repeatedly and class matching performs a nested linear search:

```cpp
for ( const auto& cls : mClasses ) {
	if ( std::find( elClasses.begin(), elClasses.end(), cls ) == elClasses.end() )
		return false;
}
```

This is expensive with modern pages where elements often have several classes and stylesheets have many class selectors.

## Tasks

### 5.1 Add hashes to selector rules

In `StyleSheetSelectorRule`, keep current strings for compatibility/debug/build output, but add hashes:

```cpp
String::HashType mTagHash{ 0 };
String::HashType mIdHash{ 0 };
std::vector<String::HashType> mClassHashes;
```

Populate them in `pushSelectorTypeIdentifier()` / `parseFragment()`.

### 5.2 Add hashes to widgets

In `UIWidget`, add:

```cpp
String::HashType mTagHash{ 0 };
std::vector<String::HashType> mClassHashes;
```

Maintain `mTagHash` in `setElementTag()`.

Maintain `mClassHashes` in:

```text
resetClass()
setClass()
setClasses()
addClass()
addClasses()
removeClass()
removeClasses()
toggleClass()
```

Keep `mClassHashes` sorted and unique if possible.

### 5.3 Add fast lookup helpers

```cpp
inline String::HashType getElementTagHash() const { return mTagHash; }
inline const std::vector<String::HashType>& getClassHashes() const { return mClassHashes; }
inline bool hasClassHash( String::HashType hash ) const {
	return std::binary_search( mClassHashes.begin(), mClassHashes.end(), hash );
}
```

For very small class counts, a linear scan over hashes may be faster than binary search. Consider:

```cpp
inline bool hasClassHash( String::HashType hash ) const {
	for ( auto classHash : mClassHashes )
		if ( classHash == hash )
			return true;
	return false;
}
```

Benchmark both. Most elements likely have few classes, so linear hash scan may win.

### 5.4 Update `matches()`

Prefer hash checks first, optionally preserving string equality as debug-only collision guard.

Example:

```cpp
if ( !mTagName.empty() && mTagName != "*" ) {
	if ( mTagHash != element->getElementTagHash() )
		return false;
	flags |= TagName;
}

if ( !mId.empty() ) {
	if ( mIdHash != element->getIdHash() )
		return false;
	flags |= Id;
}

if ( !mClassHashes.empty() ) {
	const auto& elementClassHashes = element->getClassHashes();
	if ( elementClassHashes.empty() )
		return false;

	for ( auto clsHash : mClassHashes ) {
		if ( !element->hasClassHash( clsHash ) )
			return false;
	}

	flags |= Class;
}
```

## Collision Handling

If `String::HashType` is already used as a trusted key elsewhere in eepp, follow project convention.

If collision concerns matter, use hash as fast prefilter and keep string comparison on hash match only in debug builds or always for correctness-critical code:

```cpp
#ifdef EE_DEBUG
// verify hash hit corresponds to actual string
#endif
```

## Validation Tests

All existing class/tag/id selector tests must pass.

Add tests for:

```css
.foo {}
.foo.bar {}
div.foo {}
#id.foo {}
```

Ensure class mutation APIs update hashes correctly.

---

# Phase 6: Add Class-Based Stylesheet Index

## Motivation

This is expected to be the largest real-world win.

Current index only narrows candidate rules by:

```text
global
tag
id
tag + id
```

Modern CSS uses class selectors heavily. Without a class index, selectors like `.btn`, `.card`, `.post`, `.share-button`, `.goog-inline-block`, etc. are tested against many unrelated elements.

## Design

Add a class index in `StyleSheet`:

```cpp
UnorderedMap<String::HashType, StyleSheetStyleVector> mClassNodeIndex;
```

A selector should be indexed by its rightmost rule's most selective available key:

Priority suggestion:

```text
id/tag+id existing path
class new path
tag existing path
global existing path
```

Do not put the same style into every possible class bucket initially. Start with one selected class anchor to avoid duplication.

## Tasks

### 6.1 Expose rightmost class info from selector rule

Add to `StyleSheetSelectorRule`:

```cpp
bool hasClasses() const;
const std::vector<String::HashType>& getClassHashes() const;
String::HashType getBestClassHash() const;
```

For `getBestClassHash()`, initially return the first class hash. Later, choose the rarest class using class-frequency statistics collected during stylesheet build.

### 6.2 Update `StyleSheet::clear()` and mutation paths

Clear `mClassNodeIndex` wherever `mNodeIndex` is cleared.

### 6.3 Update `addStyleToNodeIndex()`

Pseudo-code:

```cpp
bool StyleSheet::addStyleToNodeIndex( StyleSheetStyle* style ) {
	const auto& selector = style->getSelector();
	const auto& rule = selector.getRule( 0 );

	const std::string& id = selector.getSelectorId();
	const std::string& tag = selector.getSelectorTagName();

	if ( !( style->hasProperties() || style->hasVariables() ) )
		return false;

	if ( !id.empty() ) {
		// existing mNodeIndex behavior
		return addToNodeIndex(...);
	}

	if ( rule.hasClasses() ) {
		auto clsHash = rule.getBestClassHash();
		auto& nodes = mClassNodeIndex[clsHash];
		if ( std::find( nodes.begin(), nodes.end(), style ) == nodes.end() ) {
			nodes.push_back( style );
			return true;
		}
		return false;
	}

	// existing tag/global behavior
}
```

### 6.4 Update `getElementStyles()`

Collect candidates from existing node hashes plus class buckets for the element's classes.

Pseudo-code:

```cpp
for ( auto classHash : element->getClassHashes() ) {
	auto it = mClassNodeIndex.find( classHash );
	if ( it != mClassNodeIndex.end() ) {
		for ( StyleSheetStyle* node : it->second ) {
			// duplicate guard, media check, selector select
		}
	}
}
```

### 6.5 Avoid duplicate candidate tests

A style might appear in multiple buckets if later changes index it in several places. Even if initially it appears in only one bucket, adding a duplicate guard is useful.

Preferred approach: per-style marker.

In `StyleSheetStyle`:

```cpp
mutable Uint32 mMatchMarker{ 0 };
```

In `StyleSheet`:

```cpp
mutable Uint32 mCurrentMatchMarker{ 1 };
```

During candidate collection:

```cpp
if ( style->getMatchMarker() == marker )
	continue;
style->setMatchMarker( marker );
```

Be careful with thread-safety if stylesheet matching can happen concurrently. If not, this is fastest.

Alternative: local `UnorderedSet<StyleSheetStyle*> visited`, slower but simpler.

## Expected Performance Impact

High. This changes candidate collection from:

```text
all global/class-only selectors tested against every element
```

to:

```text
only selectors anchored to classes the element actually has
```

On modern pages, this can be a multiple-times improvement depending on stylesheet size.

## Validation Tests

Add tests for:

```css
.foo { color: red; }
.bar { color: blue; }
.foo.bar { font-weight: bold; }
div.foo { margin: 1px; }
#id.foo { padding: 1px; }
```

Verify matching behavior is unchanged before/after class index.

Use instrumentation from Phase 4 to confirm:

- Global bucket candidates decrease substantially.
- Total selector calls decrease substantially.
- Matched rule count remains identical.

---

# Phase 7: Add Single-Rule Selector Fast Path

## Motivation

Many selectors are simple:

```css
.foo
.foo.bar
div
button:hover
#id
```

Current `StyleSheetSelector::select()` always loops over `mSelectorRules` and switches on combinator type, even for a single rule.

## Task

Add a field:

```cpp
bool mIsSingleRule{ false };
```

Set after parsing:

```cpp
mIsSingleRule = mSelectorRules.size() == 1;
```

Then split:

```cpp
bool StyleSheetSelector::select( UIWidget* element, const bool& applyPseudo ) const {
	if ( mIsSingleRule )
		return mSelectorRules[0].matches( element, applyPseudo );

	return selectComplex( element, applyPseudo );
}
```

Move current implementation body to `selectComplex()`.

## Expected Impact

Moderate but broad. It avoids loop/switch overhead for the common case.

This becomes more valuable after class indexing because the remaining candidate set will often be highly relevant, simple selectors.

## Validation Tests

Existing selector tests should pass.

Add explicit tests for:

```css
.foo {}
.foo:hover {}
#id {}
div.foo {}
```

with both `applyPseudo == false` and `applyPseudo == true` where applicable.

---

# Phase 8: Reduce Attribute Selector Cost

## Motivation

Attribute selectors are less common than class selectors but expensive when evaluated.

Current matching path calls either:

```cpp
element->getPropertyString( attr.name )
```

or data-property lookup for HTML data attributes.

## Tasks

Keep this phase after class indexing unless profiling says attributes dominate.

Possible improvements:

1. Hash attribute names in `AttributeSelector`.
2. Add faster property lookup by property id/hash when possible.
3. Avoid constructing `std::string elValStorage` unless necessary.
4. Fast-path existence-only selectors `[attr]`.
5. Fast-path exact matching for known properties.

Current code allocates or assigns into local storage for every attribute selector attempt. Avoid work until a selector survives tag/id/class checks.

Ensure `matches()` keeps attribute checks after tag/id/class, as it already does.

---

# Phase 9: Reduce Sorting and ElementDefinition Rebuild Cost

## Motivation

`StyleSheet::getElementStyles()` sorts `applicableNodes` by specificity every time:

```cpp
std::stable_sort( applicableNodes.begin(), applicableNodes.end(), StyleSheetNodeSort );
```

Then it hashes the matched style pointers and uses `mNodeCache` to reuse `ElementDefinition` objects.

`ElementDefinition::refresh()` then resolves winning properties by specificity while iterating styles.

## Potential Optimizations

Do this only after profiling shows sorting or definition creation is significant.

### Option A: Pre-sort buckets

Maintain each index bucket sorted by specificity and source order when styles are added.

Then merge candidates instead of sorting all matched candidates.

### Option B: Resolve cascade directly

Instead of sorting matched styles first, store selector specificity and source order, then resolve property winners directly:

```cpp
if ( incomingSpecificity > currentSpecificity ||
	 incomingSpecificity == currentSpecificity && incomingSourceOrder > currentSourceOrder ) {
	property wins;
}
```

This may require changes to `StyleSheetProperty` specificity/source order representation.

### Option C: Cache applicable style pointer vector earlier

Currently cache lookup happens after selector matching and sorting. Add a cache keyed by element selector signature for simple selectors.

See Phase 10.

---

# Phase 10: Add Selector Signature Cache for Simple Selectors

## Motivation

Current `mNodeCache` avoids rebuilding `ElementDefinition` if the exact set of matched style pointers repeats, but it does not avoid matching selectors.

For simple selectors without combinators, the result depends mostly on:

```text
tag
id
classes
pseudo state, when applyPseudo == true
stylesheet version
```

## Design

Create a selector signature:

```cpp
struct ElementSelectorSignature {
	String::HashType tagHash;
	String::HashType idHash;
	SmallVector<String::HashType, N> classHashes;
	Uint32 pseudoClasses;
	bool applyPseudo;
};
```

Use it only for simple selector groups:

```text
no combinators
no structural pseudo classes
no attributes with dynamic property dependencies, unless included in signature
```

Split stylesheet rules into:

```text
simple rules
complex rules
```

Resolve simple rules from signature cache. Continue resolving complex rules with current matching.

## Caution

This is more invasive. Do after class indexing and only if profiling still shows selector matching cost is high.

---

# Additional Correctness Tests

Add regression coverage for old browser CSS hacks and selector boundaries.

## Star HTML Hack

```html
<html>
	<body>
		<a class="goog-inline-block"></a>
	</body>
</html>
```

```css
.goog-inline-block { display: inline-block; }
* html .goog-inline-block { display: inline; }
```

Expected:

```text
inline-block
```

## Child selector with HTML root

```css
html > body {}
* > html {}
```

Expected:

- `html > body` can match body.
- `* > html` must not match html.

## Descendant selector crossing eepp GUI widget

Ensure this still works if eepp supports GUI widgets inside HTML compatibility trees:

```text
html
  body
    eepp-widget.foo
```

```css
body .foo { ... }
```

Expected: eepp widget matches.

## Text node sibling skip

```html
<div id="a"></div>
text
<div id="b"></div>
```

```css
#a + #b { color: red; }
```

Expected: `#b` matches.

## General sibling direction

```html
<div id="a"></div>
<span></span>
<div id="b"></div>
```

```css
#a ~ #b { color: red; }
#b ~ #a { color: blue; }
```

Expected:

- `#b` matches red rule.
- `#a` does not match blue rule.

## Universal selector with class

```html
<div class="foo"></div>
<div></div>
```

```css
*.foo { color: red; }
```

Expected: only first div matches.

## Universal selector with attribute

```html
<a href="x"></a>
<a></a>
```

```css
*[href] { color: red; }
```

Expected: only first anchor matches.

---

# Benchmark Plan

Use one or more heavy modern pages with realistic CSS and DOM:

- Blogger page with share buttons.
- A CSS-framework-heavy page such as Bootstrap/Tailwind-like markup.
- A page with many repeated card/list items.
- A page with deep nesting and descendant selectors.

Measure at least:

```text
Total style load/reload time
Total getElementStyles() calls
Average candidate rules per element
Total selector calls
Total matched rules
Global bucket candidate count
Class bucket candidate count
Descendant traversal steps
Sibling traversal steps
Attribute selector calls
mNodeCache hit/miss count
```

Measure after each phase:

```text
baseline
phase 1
phase 2-3 correctness fixes
phase 5 hash matching
phase 6 class index
phase 7 single-rule fast path
```

Do not rely only on microbenchmarks. The class index should be tested on full-page style application.

---

# Expected Priority of Wins

Likely impact ranking:

1. Class-based stylesheet index.
2. Hash-based class/tag/id matching.
3. Correct sibling scan direction, especially avoiding next-sibling scan for `~`.
4. Single-rule selector fast path.
5. Inlining tiny traversal and flag helpers.
6. Attribute selector micro-optimizations.
7. Sorting/cache redesign.

Inlining is worthwhile but not the main algorithmic win. The main problem is candidate explosion caused by class-only selectors being treated as global candidates.

---

# Implementation Notes for the Agent

- Preserve eepp's HTML compatibility design: CSS selector traversal must still see non-HTML eepp GUI widgets embedded inside HTML trees.
- Do not use `UI_HTML_ELEMENT` as a generic selector traversal boundary.
- Use `UI_TYPE_HTML_HTML` specifically as the parent traversal boundary.
- Skip `UI_TYPE_TEXTNODE` / `NODE_FLAG_TEXTNODE` in sibling traversal.
- Keep string fields even after adding hashes to avoid breaking public APIs, debug output, CSS serialization, and tests.
- Prefer minimal changes with tests after each phase.
- Do not implement signature caching or sort redesign until instrumentation proves they are still needed.

