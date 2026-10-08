# eepp CSS Selector Optimization Plan

## Status (updated 2026-08-23)

Phases 1–3 and 5–8 are **implemented and measured**. Their detailed task lists have been
removed; the completed-work record with measurements is kept under "Completed Phases".

Remaining work in this plan:

- Phase 4 — Selector-matching instrumentation (**not started**)
- Phase 9 — Reduce sorting and ElementDefinition rebuild cost (**not started**)
- Phase 10 — Selector signature cache for simple selectors (**not started**)

Per the plan's own gating rule: implement Phase 4 first and let its counters decide whether
Phases 9 and 10 are worth implementing at all.

## Goal

Optimize the CSS selector matching hot path in eepp's HTML compatibility layer and UI styling engine, especially for modern class-heavy pages.

This plan targets the `develop` branch of:

```text
https://github.com/SpartanJ/eepp
```

## Key Files

Primary files to inspect and modify for the remaining phases:

```text
include/eepp/ui/css/stylesheet.hpp
src/eepp/ui/css/stylesheet.cpp
include/eepp/ui/css/stylesheetselector.hpp
src/eepp/ui/css/stylesheetselector.cpp
include/eepp/ui/css/stylesheetselectorrule.hpp
src/eepp/ui/css/stylesheetselectorrule.cpp
include/eepp/ui/css/elementdefinition.hpp
src/eepp/ui/css/elementdefinition.cpp
src/tests/unit_tests/uihtml_tests.cpp
```

Current state after Phases 1–3 and 5–8:

- Candidate collection uses global, tag, ID, tag+ID, and class-hash buckets; each style is owned by exactly one bucket.
- Tag/ID/class matching compares cached hashes instead of strings; widgets maintain sorted unique class hashes.
- Simple single-rule selectors bypass combinator traversal via a direct `matches()` fast path.
- Attribute names resolve through cached `PropertyDefinition`s; data-attribute classification is precomputed.
- Sorting of matched candidates by specificity/source order and the `mNodeCache`
  `ElementDefinition` reuse remain untouched (Phase 9 scope).
- No instrumentation counters exist yet (Phase 4 scope).

---

# Completed Phases

## Phase 1: Inline Tiny Hot-Path Accessors — Implemented

Accessors such as `Node::isWidget()` / `isTextNode()` and the `UIWidget` style-sheet traversal
helpers (`getStyleSheetParentElement()`, `getStyleSheetPreviousSiblingElement()`,
`getStyleSheetNextSiblingElement()`, pseudo-class/tag/class accessors) were moved inline into
headers. Design constraints honored: parent traversal stops specifically at
`UI_TYPE_HTML_HTML`; sibling traversal skips only text nodes; non-HTML eepp GUI widgets inside
HTML trees remain selector-visible.

## Phase 2: Fix Confirmed Sibling Combinator Direction Bug — Implemented

`+` (DIRECT_SIBLING) and `~` (SIBLING) now scan previous element siblings per CSS
right-to-left semantics; eepp's inverse sibling operator keeps next-sibling direction;
`StyleSheetSelector::getRelatedElements()` uses the same semantics so non-cacheable style
subscriptions stay correct.

## Phase 3: Fix Universal Selector Early Return — Implemented

The unsafe early return for `*` was replaced with a fast path that only fires for a pure
universal selector; `*.foo`, `*[href]`, `*:not(...)` still evaluate class, attribute, ID, and
pseudo-class requirements.

## Phase 5: Hash-Based Tag/ID/Class Matching — Implemented

Implementation state:

- Selector rules cache tag, ID, and sorted unique class hashes.
- `UIWidget` caches its tag hash and maintains sorted unique class hashes across every class
  mutation API.
- Widget class hashes use `SmallVector<String::HashType, 1>`: zero/one-class cases remain
  allocation-free, multi-class widgets use indirect storage.
- `matches()` compares tag and ID hashes and checks required class hashes after decoding the
  widget hash storage once per rule match.
- `UINode`/`UIWidget` members were reordered to reclaim alignment holes.
- Focused selector benchmark and class-mutation regression tests cover the implementation.

Measured result:

- Release unit-test workload: 74.5% of destroyed widgets had no classes, 19.4% one, 5.9% two,
  ~0.1% three or more.
- `sizeof( UIWidget )` unchanged at 1144 bytes; `sizeof( UINode )` decreased from 856 to 848
  bytes.
- Focused release selector benchmark: median ~111.4 ms vs Phase 3 baseline ~115.1 ms
  (~3.2% improvement) without net `UIWidget` growth.
- Full release suite passed 734 tests with one skipped.

Future architectural option (out of scope): atomized/interned class names replacing per-widget
class strings and the hash cache; requires explicit ownership, collision, API, and cross-scene
lifetime design.

## Phase 6: Add Class-Based Stylesheet Index — Implemented

Implementation state:

- `StyleSheet` maintains a class-hash index (`mClassNodeIndex`) alongside the existing global,
  tag, ID, and tag+ID index.
- Each style is owned by exactly one candidate bucket. Priority: ID/tag+ID, one rightmost class
  anchor, tag, then global, so candidate collection needs no per-element visited allocation.
- Per-stylesheet source-order metadata is the secondary sort key after specificity, preserving
  CSS cascade order across buckets.
- Index state participates in clear, copy assignment, marker-based removal, and normal insertion.
- Focused candidate test covers global, tag, class-only, compound-class, tag+class, ID+class,
  and unrelated-class rules.

Measured result:

- `Benchmark.CSSClassIndexLookup` (1,024 class rules, 64 sharing the target widget's class):
  legacy full scan 124.1 ms median vs indexed lookup 45.7 ms median for 20,000 lookups — a
  63.2% reduction, ~2.7x faster.
- Full release suite passed 735 tests with one skipped.

## Phase 7: Add Single-Rule Selector Fast Path — Implemented

Implementation state:

- `StyleSheetSelector` records whether parsing produced exactly one rule.
- Single-rule selectors call `StyleSheetSelectorRule::matches()` directly; selectors with
  combinators retain the existing traversal logic in `selectComplex()`.
- Focused test covers class, ID, tag+class, and pseudo-class selectors with pseudo matching both
  enabled and disabled.

Measured result:

- `Benchmark.CSSSelectorMatching`: 93.0 ms median for 20.48 million selector calls across seven
  release runs, vs the post-Phase-5 median of ~111.4 ms — an indicative 16.6% reduction.

## Phase 8: Reduce Attribute Selector Cost — Implemented

Implementation state:

- Attribute names are hashed once during parsing to resolve and cache the standard
  `PropertyDefinition`, avoiding repeated specification lookups while matching.
- Data-attribute classification is cached in each parsed attribute selector.
- Existence-only data selectors return after the map lookup without constructing or reading a
  value string; value comparisons use the stored `StyleSheetProperty` string directly.
- Tag, ID, and class checks remain ahead of attribute matching so unrelated elements reject
  early.
- Coverage exercises all supported data-attribute operators including empty-value existence,
  plus standard-property existence, exact matching, and unknown properties.

Measured result:

- `Benchmark.CSSAttributeSelectorMatching` across seven release runs of 5.12 million selector
  calls: median decreased from 155.9 ms to 90.9 ms, a 41.7% reduction.

---

# Phase 4: Add Selector-Matching Instrumentation

**Status: Not started — implement this before Phases 9/10**

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

Note: after Phases 5–8 the bucket distribution changed substantially (class buckets now carry
most candidates). Re-derive the bottleneck picture from scratch rather than assuming the
pre-index findings below.

## Expected Findings (pre-index assumptions, verify against current code)

Likely bottlenecks on modern pages:

- Global bucket candidates reduced by the class index, but descendant selectors may still cause repeated ancestor walks.
- Attribute selectors are less frequent but expensive per check.
- Sorting and `ElementDefinition` rebuild costs become visible once matching is cheap.

Use these counters to decide whether Phases 9 and 10 are justified.

---

# Phase 9: Reduce Sorting and ElementDefinition Rebuild Cost

**Status: Not started**

## Motivation

`StyleSheet::getElementStyles()` sorts `applicableNodes` by specificity every time:

```cpp
std::stable_sort( applicableNodes.begin(), applicableNodes.end(), StyleSheetNodeSort );
```

Then it hashes the matched style pointers and uses `mNodeCache` to reuse `ElementDefinition` objects.

`ElementDefinition::refresh()` then resolves winning properties by specificity while iterating styles.

## Potential Optimizations

Do this only after profiling (Phase 4) shows sorting or definition creation is significant.

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

**Status: Not started**

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

This is more invasive. Do after Phase 4 instrumentation proves selector matching cost is still high despite the class index, hash matching, and single-rule fast path.

---

# Implementation Notes for the Agent

- Preserve eepp's HTML compatibility design: CSS selector traversal must still see non-HTML eepp GUI widgets embedded inside HTML trees.
- Do not use `UI_HTML_ELEMENT` as a generic selector traversal boundary.
- Use `UI_TYPE_HTML_HTML` specifically as the parent traversal boundary.
- Skip `UI_TYPE_TEXTNODE` / `NODE_FLAG_TEXTNODE` in sibling traversal.
- Instrumentation counters must stay disabled by default and be cheap when disabled.
- Prefer minimal changes with tests after each step.
- Do not implement Phase 9 Option C / Phase 10 signature caching until Phase 4 counters prove they are still needed.
