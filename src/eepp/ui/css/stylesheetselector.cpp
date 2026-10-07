#include <eepp/core/containers.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uiwidget.hpp>

#include <algorithm>
#include <memory>

namespace EE { namespace UI { namespace CSS {

StyleSheetSelector::StyleSheetSelector() : mName( "*" ), mSpecificity( 0 ) {
	parseSelector( mName );
}

StyleSheetSelector::StyleSheetSelector( const std::string& selectorName ) :
	mName( selectorName ), mSpecificity( 0 ) {
	parseSelector( mName );
}

void StyleSheetSelector::setSpecificity( const Int64& specificity ) {
	mSpecificity = specificity;
}

void removeExtraSpaces( std::string& string ) {
	// TODO: Optimize this
	String::trimInPlace( string );
	String::replaceAll( string, "   ", " " );
	String::replaceAll( string, "  ", " " );
	String::replaceAll( string, " > ", ">" );
	String::replaceAll( string, " | ", "|" );
	String::replaceAll( string, " + ", "+" );
	String::replaceAll( string, " ~ ", "~" );
	String::replaceAll( string, " >", ">" );
	String::replaceAll( string, " |", "|" );
	String::replaceAll( string, " +", "+" );
	String::replaceAll( string, " ~", "~" );
}

void StyleSheetSelector::addSelectorRule(
	std::string& buffer, StyleSheetSelectorRule::PatternMatch& curPatternMatch,
	const StyleSheetSelectorRule::PatternMatch& newPatternMatch ) {
	StyleSheetSelectorRule selectorRule( buffer, curPatternMatch );
	mSelectorRules.push_back( selectorRule );
	curPatternMatch = newPatternMatch;
	buffer.clear();
	mSpecificity += selectorRule.getSpecificity();
}

void StyleSheetSelector::parseSelector( std::string selector ) {
	if ( !selector.empty() ) {
		// Remove spaces that means nothing to the selector logic
		// for example:
		// Element > .class #id
		// should be
		// Element>.class #id
		removeExtraSpaces( selector );

		std::string buffer;
		StyleSheetSelectorRule::PatternMatch curPatternMatch = StyleSheetSelectorRule::ANY;
		bool inAttribute = false;
		char quote = 0;

		for ( auto charIt = selector.rbegin(); charIt != selector.rend(); ++charIt ) {
			char curChar = *charIt;

			if ( quote != 0 ) {
				buffer = curChar + buffer;
				if ( curChar == quote )
					quote = 0;
				continue;
			}

			if ( inAttribute ) {
				buffer = curChar + buffer;
				if ( curChar == '"' || curChar == '\'' )
					quote = curChar;
				else if ( curChar == '[' )
					inAttribute = false;
				continue;
			}

			if ( curChar == ']' ) {
				inAttribute = true;
				buffer = curChar + buffer;
				continue;
			}

			switch ( curChar ) {
				case StyleSheetSelectorRule::DESCENDANT:
					addSelectorRule( buffer, curPatternMatch, StyleSheetSelectorRule::DESCENDANT );
					break;
				case StyleSheetSelectorRule::CHILD:
					addSelectorRule( buffer, curPatternMatch, StyleSheetSelectorRule::CHILD );
					break;
				case StyleSheetSelectorRule::DIRECT_SIBLING:
					addSelectorRule( buffer, curPatternMatch,
									 StyleSheetSelectorRule::DIRECT_SIBLING );
					break;
				case StyleSheetSelectorRule::PREVIOUS_SIBLING:
					addSelectorRule( buffer, curPatternMatch,
									 StyleSheetSelectorRule::PREVIOUS_SIBLING );
					break;
				case StyleSheetSelectorRule::SIBLING:
					addSelectorRule( buffer, curPatternMatch, StyleSheetSelectorRule::SIBLING );
					break;
				default:
					buffer = curChar + buffer;
					break;
			}
		}

		if ( !buffer.empty() ) {
			addSelectorRule( buffer, curPatternMatch, StyleSheetSelectorRule::ANY );
			buffer.clear();
		}

		mCacheable = true;

		if ( !mSelectorRules.empty() ) {
			if ( mSelectorRules[0].hasStructuralPseudoClasses() ) {
				mStructurallyVolatile = true;
				mCacheable = false;
			}
		}

		if ( mCacheable ) {
			for ( size_t i = 1; i < mSelectorRules.size(); i++ ) {
				if ( mSelectorRules[i].hasPseudoClasses() ||
					 mSelectorRules[i].hasStructuralPseudoClasses() ) {
					mCacheable = false;
					break;
				}
			}
		}
	}

	mIsSingleRule = mSelectorRules.size() == 1;
}

bool StyleSheetSelector::isCacheable() const {
	return mCacheable;
}

bool StyleSheetSelector::hasPseudoClasses() const {
	return !mSelectorRules.empty() && mSelectorRules[0].hasPseudoClasses();
}

bool StyleSheetSelector::select( UIWidget* element, const bool& applyPseudo ) const {
	if ( mSelectorRules.empty() )
		return false;

	if ( mIsSingleRule )
		return mSelectorRules[0].matches( element, applyPseudo );

	return selectComplex( element, applyPseudo );
}

namespace {

// Selectors match right to left. A descendant (space) or general sibling (~) relation must try
// other candidates when a matching compound's left-hand chain fails (Selectors 4, section 17.3).
// For example, in "#scope > * a", the closest ancestor matching * may not be a child of #scope,
// while a farther ancestor is. A greedy first match incorrectly rejects the whole selector.
//
// Checkpoints resume the candidate search, not the whole selector. Failure scope prunes them:
// - Exhausting ancestors ends the match: farther ancestors/siblings offer no new ancestors.
// - A parent mismatch or exhausted previous siblings cannot be fixed by earlier sibling candidates
//   with the same parent. Skip those checkpoints and retry the nearest ancestor candidate instead.
// - Consecutive descendant searches need no checkpoint: the next search visits all remaining
//   ancestors, and the scopes above already handle any later failure.
//
// <html> is the one exception to "siblings share ancestors": getStyleSheetParentElement() hides
// its physical parent, while its element siblings keep theirs. A failed parent step that stops at
// such an <html> therefore proves nothing about alternatives that reach its siblings, so it
// retries every checkpoint instead of ending the match. A general-sibling candidate that is such
// an <html> saves no checkpoint before a descendant search; that search fails at its first step
// and resumes the ~ search from the candidate instead. An <html> without element siblings, as in
// ordinary documents, keeps the regular pruning.
struct SelectorRetry {
	UIWidget* element;
	size_t ruleIndex;
};
static_assert( sizeof( SelectorRetry ) == sizeof( UIWidget* ) + sizeof( size_t ) );

// A descendant search followed by another descendant search needs no checkpoint (see above).
inline bool shouldSaveRetry( const std::vector<StyleSheetSelectorRule>& rules, size_t ruleCount,
							 size_t i ) {
	return i + 1 < ruleCount &&
		   rules[i + 1].getPatternMatch() != StyleSheetSelectorRule::DESCENDANT;
}

#if defined( _MSC_VER )
#define EE_SELECTOR_COLD __declspec( noinline )
#else
#define EE_SELECTOR_COLD __attribute__( ( noinline, cold ) )
#endif

// The inline half of the <html> boundary test for an element without a style sheet parent: only
// a hidden parent is a widget, so ordinary roots fail here without a call.
inline bool hasWidgetParent( const UIWidget* element ) {
	const Node* parent = element->getParent();
	return NULL != parent && parent->isWidget();
}

// Only evaluated after hasWidgetParent() on a parent step that found nothing. Kept out of line so
// selectComplex() keeps its original register use and code size on the hot paths.
EE_SELECTOR_COLD bool hidesParentFromSiblings( const UIWidget* element ) {
	return NULL == element->getStyleSheetParentElement() &&
		   ( NULL != element->getStyleSheetPreviousSiblingElement() ||
			 NULL != element->getStyleSheetNextSiblingElement() );
}

inline bool tracksStateChanges( const StyleSheetSelectorRule& rule ) {
	return rule.hasAnyPseudoClasses();
}

// Tracked flags computed once per getRelatedElements() call. The first 64 rules are cached in a
// mask; longer selectors query the remaining rules directly.
struct TrackedRules {
	const std::vector<StyleSheetSelectorRule>& rules;
	Uint64 mask{ 0 };

	bool operator[]( size_t i ) const {
		return i < 64 ? ( ( mask >> i ) & 1 ) != 0 : tracksStateChanges( rules[i] );
	}
};

// Moves across one combinator; descendant and ~ relations yield their first candidate.
inline UIWidget* combinatorStep( UIWidget* element, StyleSheetSelectorRule::PatternMatch pattern ) {
	switch ( pattern ) {
		case StyleSheetSelectorRule::CHILD:
		case StyleSheetSelectorRule::DESCENDANT:
			return element->getStyleSheetParentElement();
		case StyleSheetSelectorRule::DIRECT_SIBLING:
		case StyleSheetSelectorRule::SIBLING:
			return element->getStyleSheetPreviousSiblingElement();
		case StyleSheetSelectorRule::PREVIOUS_SIBLING:
			return element->getStyleSheetNextSiblingElement();
		case StyleSheetSelectorRule::ANY:
			break;
	}
	return NULL;
}

// Paths can revisit an element through | and +, or share it between candidates; report it once.
inline void addRelated( SmallVector<UIWidget*, 8>& related, UIWidget* element ) {
	if ( std::find( related.begin(), related.end(), element ) == related.end() )
		related.push_back( element );
}

// An untracked compound whose candidates all lead to the same next search can take its nearest
// usable match, because the compound itself adds no related element:
// - A descendant search before another descendant search: a farther candidate only leaves fewer
//   ancestors to search. selectComplex() skips its checkpoint for the same reason.
// - A ~ search before a > or descendant step: sibling candidates share their parent. Only an
//   <html> candidate hides it, and such a candidate cannot complete, so it is skipped.
// - The leftmost compound: nothing follows it, so any match completes the selector.
inline bool isNearestMatchEnough( const std::vector<StyleSheetSelectorRule>& rules, size_t i,
								  bool tracked ) {
	if ( tracked )
		return false;
	if ( i + 1 >= rules.size() )
		return true;
	const auto next = rules[i + 1].getPatternMatch();
	switch ( rules[i].getPatternMatch() ) {
		case StyleSheetSelectorRule::DESCENDANT:
			return next == StyleSheetSelectorRule::DESCENDANT;
		case StyleSheetSelectorRule::SIBLING:
			return next == StyleSheetSelectorRule::DESCENDANT ||
				   next == StyleSheetSelectorRule::CHILD;
		default:
			return false;
	}
}

// Matches rules [begin, end) starting from element. Descendant and ~ combinators among them must
// satisfy isNearestMatchEnough(). Returns the last element reached, or NULL when a compound does
// not match. verifySteps = false skips the other compounds when a match is already known.
inline UIWidget* matchFixedRules( const std::vector<StyleSheetSelectorRule>& rules,
								  const TrackedRules& tracked, size_t begin, size_t end,
								  UIWidget* element, bool applyPseudo,
								  SmallVector<UIWidget*, 8>& related, bool verifySteps = true ) {
	for ( size_t i = begin; i < end; i++ ) {
		const auto pattern = rules[i].getPatternMatch();
		if ( pattern == StyleSheetSelectorRule::DESCENDANT ||
			 pattern == StyleSheetSelectorRule::SIBLING ) {
			do {
				element = combinatorStep( element, pattern );
			} while ( NULL != element &&
					  ( !rules[i].matches( element, applyPseudo ) ||
						( pattern == StyleSheetSelectorRule::SIBLING && i + 1 < rules.size() &&
						  NULL == element->getStyleSheetParentElement() ) ) );
		} else {
			element = combinatorStep( element, pattern );
			if ( verifySteps && NULL != element && !rules[i].matches( element, applyPseudo ) )
				element = NULL;
		}
		if ( NULL == element )
			return NULL;
		if ( tracked[i] )
			addRelated( related, element );
	}
	return element;
}

// Selectors whose combinators are all > or descendant place every compound on the subject's
// ancestor chain, so their (compound, ancestor) states fit flat arrays indexed by depth. A forward
// pass marks reachable matches and a backward pass marks those that can complete; running ORs
// stand in for descendant searches. Unlike RelatedElementCollector, it needs no element lookups
// or navigation caches. The subject must already match the first compound.
inline bool collectAncestorPathRelated( const std::vector<StyleSheetSelectorRule>& rules,
										const TrackedRules& tracked, UIWidget* subject,
										bool applyPseudo, SmallVector<UIWidget*, 8>& related ) {
	enum : Uint8 { Reached = 1 << 0, Completes = 1 << 1, Reported = 1 << 2 };
	SmallVector<UIWidget*, 32> chain;
	for ( UIWidget* element = subject; NULL != element;
		  element = element->getStyleSheetParentElement() ) {
		chain.push_back( element );
	}
	const size_t depth = chain.size();
	const size_t ruleCount = rules.size();
	SmallVector<Uint8, 127> states;
	states.resize( ruleCount * depth, 0 );
	auto at = [&]( size_t rule, size_t position ) -> Uint8& {
		return states[rule * depth + position];
	};

	at( 0, 0 ) = Reached;
	for ( size_t rule = 1; rule < ruleCount; rule++ ) {
		const bool descendant = rules[rule].getPatternMatch() == StyleSheetSelectorRule::DESCENDANT;
		bool reachedBelow = false;
		bool any = false;
		for ( size_t position = 1; position < depth; position++ ) {
			const bool fromBelow = ( at( rule - 1, position - 1 ) & Reached ) != 0;
			reachedBelow = reachedBelow || fromBelow;
			if ( ( descendant ? reachedBelow : fromBelow ) &&
				 rules[rule].matches( chain[position], applyPseudo ) ) {
				at( rule, position ) = Reached;
				any = true;
			}
		}
		if ( !any )
			return false;
	}

	for ( size_t position = 0; position < depth; position++ )
		at( ruleCount - 1, position ) |=
			( at( ruleCount - 1, position ) & Reached ) ? Completes : 0;
	for ( size_t rule = ruleCount - 1; rule-- > 0; ) {
		const bool descendant =
			rules[rule + 1].getPatternMatch() == StyleSheetSelectorRule::DESCENDANT;
		bool completesAbove = false;
		for ( size_t position = depth; position-- > 0; ) {
			const bool fromAbove =
				position + 1 < depth && ( at( rule + 1, position + 1 ) & Completes ) != 0;
			completesAbove = completesAbove || fromAbove;
			if ( ( descendant ? completesAbove : fromAbove ) && ( at( rule, position ) & Reached ) )
				at( rule, position ) |= Completes;
		}
	}
	if ( !( at( 0, 0 ) & Completes ) )
		return false;

	for ( size_t rule = 1; rule < ruleCount; rule++ ) {
		if ( !tracked[rule] )
			continue;
		for ( size_t position = 1; position < depth; position++ ) {
			// Each position is a distinct element; its rule 0 slot records that it was reported.
			if ( ( at( rule, position ) & Completes ) && !( at( 0, position ) & Reported ) ) {
				at( 0, position ) |= Reported;
				related.push_back( chain[position] );
			}
		}
	}
	return true;
}

// Collects the union of related elements over every matching path. Selectors 4 (section 17.3)
// accepts any path, so a state change on an element of any of them can change the result.
//
// Paths are never enumerated. A forward pass records each (element, rule) state that matches its
// compound and is reachable from the subject; a backward pass marks the states whose remaining
// selector can complete. Ancestor and previous-sibling walks stop at an element already visited
// for the same rule, because the rest of that chain was handled from there, so both passes are
// linear in the number of states. Both are iterative, so long selectors cannot exhaust the stack.
class RelatedElementCollector {
  public:
	RelatedElementCollector( const std::vector<StyleSheetSelectorRule>& rules, bool applyPseudo ) :
		mRules( rules ),
		mRuleCount( static_cast<Uint32>( rules.size() ) ),
		mApplyPseudo( applyPseudo ) {}

	void collect( UIWidget* subject, SmallVector<UIWidget*, 8>& related );

  private:
	enum StateFlags : Uint8 {
		Tested = 1 << 0,		 // The compound was evaluated against the element.
		Matches = 1 << 1,		 // The element is reachable and matches the compound.
		Walked = 1 << 2,		 // The forward pass enumerated the candidate chain from here.
		Completes = 1 << 3,		 // The rest of the selector can complete from this match.
		ChainKnown = 1 << 4,	 // ChainCompletes is valid.
		ChainCompletes = 1 << 5, // This element or a later chain candidate completes.
		Reported = 1 << 6,		 // Already returned; kept in the element's rule 0 slot.
	};

	enum Relation : Uint8 { Parent, Previous, Next };

	static constexpr Uint32 None = ~Uint32( 0 );
	static constexpr Uint32 Unknown = None - 1;
	// Up to this many elements, a linear scan is cheaper than building a hash index.
	static constexpr Uint32 LinearLookupLimit = 32;

	// Navigation results cached per element, so each relation is resolved and looked up once.
	struct Links {
		Uint32 to[3]{ Unknown, Unknown, Unknown };
	};

	const std::vector<StyleSheetSelectorRule>& mRules;
	const Uint32 mRuleCount;
	const bool mApplyPseudo;
	SmallVector<UIWidget*, LinearLookupLimit> mElements;
	SmallVector<Links, LinearLookupLimit> mLinks;
	UnorderedMap<UIWidget*, Uint32> mElementIndex;
	SmallVector<Uint8, 127> mStates;	 // mRuleCount flags per element in mElements.
	SmallVector<Uint32, 64> mLayers;	 // Matching element ids grouped by rule.
	SmallVector<Uint32, 16> mLayerBegin; // First mLayers entry of each rule.
	SmallVector<Uint32, 16> mChain;		 // Pending chain ids in chainCompletes().

	static Relation relationOf( StyleSheetSelectorRule::PatternMatch pattern ) {
		switch ( pattern ) {
			case StyleSheetSelectorRule::DIRECT_SIBLING:
			case StyleSheetSelectorRule::SIBLING:
				return Previous;
			case StyleSheetSelectorRule::PREVIOUS_SIBLING:
				return Next;
			default:
				return Parent;
		}
	}

	static bool isChain( StyleSheetSelectorRule::PatternMatch pattern ) {
		return pattern == StyleSheetSelectorRule::DESCENDANT ||
			   pattern == StyleSheetSelectorRule::SIBLING;
	}

	Uint8& state( Uint32 id, Uint32 rule ) { return mStates[id * mRuleCount + rule]; }

	Uint32 elementId( UIWidget* element );

	Uint32 link( Uint32 id, Relation relation );

	void addCandidate( Uint32 id, Uint32 rule );

	bool chainCompletes( Uint32 first, Uint32 rule, Relation relation );
};

Uint32 RelatedElementCollector::elementId( UIWidget* element ) {
	const Uint32 id = static_cast<Uint32>( mElements.size() );
	if ( mElementIndex.empty() ) {
		for ( Uint32 i = 0; i < id; i++ ) {
			if ( mElements[i] == element )
				return i;
		}
		if ( id == LinearLookupLimit ) {
			mElementIndex.reserve( id * 2 );
			for ( Uint32 i = 0; i < id; i++ )
				mElementIndex.emplace( mElements[i], i );
			mElementIndex.emplace( element, id );
		}
	} else {
		auto inserted = mElementIndex.try_emplace( element, id );
		if ( !inserted.second )
			return inserted.first->second;
	}
	mElements.push_back( element );
	mLinks.emplace_back();
	mStates.resize( mStates.size() + mRuleCount, 0 );
	return id;
}

Uint32 RelatedElementCollector::link( Uint32 id, Relation relation ) {
	if ( mLinks[id].to[relation] != Unknown )
		return mLinks[id].to[relation];
	const UIWidget* element = mElements[id];
	UIWidget* target = relation == Parent	  ? element->getStyleSheetParentElement()
					   : relation == Previous ? element->getStyleSheetPreviousSiblingElement()
											  : element->getStyleSheetNextSiblingElement();
	// elementId() may grow mLinks, so store through the index afterwards.
	const Uint32 targetId = NULL != target ? elementId( target ) : None;
	mLinks[id].to[relation] = targetId;
	return targetId;
}

void RelatedElementCollector::addCandidate( Uint32 id, Uint32 rule ) {
	Uint8& flags = state( id, rule );
	if ( flags & Tested )
		return;
	flags |= Tested;
	if ( mRules[rule].matches( mElements[id], mApplyPseudo ) ) {
		flags |= Matches;
		mLayers.push_back( id );
	}
}

bool RelatedElementCollector::chainCompletes( Uint32 first, Uint32 rule, Relation relation ) {
	bool completes = false;
	mChain.clear();
	for ( Uint32 id = first; id != None; id = link( id, relation ) ) {
		const Uint8 flags = state( id, rule );
		if ( flags & ChainKnown ) {
			completes = flags & ChainCompletes;
			break;
		}
		mChain.push_back( id );
	}
	// Resolve from the far end so every visited element memoizes its own chain result.
	for ( Uint32 i = static_cast<Uint32>( mChain.size() ); i-- > 0; ) {
		Uint8& flags = state( mChain[i], rule );
		completes = completes || ( flags & Completes );
		flags |= ChainKnown | ( completes ? ChainCompletes : 0 );
	}
	return completes;
}

void RelatedElementCollector::collect( UIWidget* subject, SmallVector<UIWidget*, 8>& related ) {
	const Uint32 subjectId = elementId( subject );
	addCandidate( subjectId, 0 );
	if ( mLayers.empty() )
		return;
	mLayerBegin.push_back( 0 );

	// Forward: every reachable element matching each compound, each state visited once.
	for ( Uint32 rule = 1; rule < mRuleCount; rule++ ) {
		const Uint32 begin = mLayerBegin.back();
		const Uint32 end = static_cast<Uint32>( mLayers.size() );
		const auto pattern = mRules[rule].getPatternMatch();
		const Relation relation = relationOf( pattern );
		const bool chain = isChain( pattern );
		mLayerBegin.push_back( end );
		for ( Uint32 i = begin; i < end; i++ ) {
			for ( Uint32 id = link( mLayers[i], relation ); id != None;
				  id = link( id, relation ) ) {
				if ( chain ) {
					// Everything past an already walked element was enumerated from there.
					if ( state( id, rule ) & Walked )
						break;
					state( id, rule ) |= Walked;
				}
				addCandidate( id, rule );
				if ( !chain )
					break;
			}
		}
		// No element reaches this compound, so the selector does not match.
		if ( mLayers.size() == end )
			return;
	}
	mLayerBegin.push_back( static_cast<Uint32>( mLayers.size() ) );

	// Backward: a state completes when one of its candidates for the next compound completes.
	for ( Uint32 i = mLayerBegin[mRuleCount - 1]; i < mLayerBegin[mRuleCount]; i++ )
		state( mLayers[i], mRuleCount - 1 ) |= Completes;
	for ( Uint32 rule = mRuleCount - 1; rule-- > 0; ) {
		const Uint32 next = rule + 1;
		const auto pattern = mRules[next].getPatternMatch();
		const Relation relation = relationOf( pattern );
		const bool chain = isChain( pattern );
		for ( Uint32 i = mLayerBegin[rule]; i < mLayerBegin[next]; i++ ) {
			const Uint32 candidate = link( mLayers[i], relation );
			const bool complete =
				candidate != None && ( chain ? chainCompletes( candidate, next, relation )
											 : ( state( candidate, next ) & Completes ) != 0 );
			if ( complete )
				state( mLayers[i], rule ) |= Completes;
		}
	}
	if ( !( state( subjectId, 0 ) & Completes ) )
		return;

	// A reachable state that completes lies on at least one matching path.
	for ( Uint32 rule = 1; rule < mRuleCount; rule++ ) {
		if ( !tracksStateChanges( mRules[rule] ) )
			continue;
		for ( Uint32 i = mLayerBegin[rule]; i < mLayerBegin[rule + 1]; i++ ) {
			const Uint32 id = mLayers[i];
			if ( !( state( id, rule ) & Completes ) || ( state( id, 0 ) & Reported ) )
				continue;
			state( id, 0 ) |= Reported;
			related.push_back( mElements[id] );
		}
	}
}

} // namespace

namespace {

// The complex selector matcher. retries must hold one entry per rule that saves checkpoints: live
// checkpoints have strictly increasing rule indices, since a retry discards every later one, so a
// push needs no capacity check.
inline bool matchComplexSelector( const std::vector<StyleSheetSelectorRule>& rules,
								  UIWidget* element, const bool& applyPseudo,
								  SelectorRetry* retries ) {
	UIWidget* curElement = element;
	const size_t ruleCount = rules.size();
	size_t retryCount = 0;
	size_t i = 0;
	// The rule the last retry re-entered. A rule runs from a checkpoint exactly when this equals
	// its index, because reaching it afresh first retries an earlier rule.
	size_t retriedRule = ruleCount;

matchRules:
	for ( ; i < ruleCount; i++ ) {
		const StyleSheetSelectorRule& selectorRule = rules[i];

		switch ( selectorRule.getPatternMatch() ) {
			case StyleSheetSelectorRule::ANY: {
				if ( !selectorRule.matches( curElement, applyPseudo ) )
					goto retryMatch;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::DESCENDANT: {
				UIWidget* searchStart = curElement;

				curElement = curElement->getStyleSheetParentElement();

				if ( NULL == curElement ) {
					curElement = searchStart;
					goto descendantSearchStartsAtRoot;
				}

				while ( !selectorRule.matches( curElement, applyPseudo ) ) {
					UIWidget* parentElement = curElement->getStyleSheetParentElement();

					// We reached the root. Retrying higher candidates would repeat an exhausted
					// search, unless it stopped at an <html> with element siblings.
					if ( NULL == parentElement )
						goto parentMissing;

					curElement = parentElement;
				}

				if ( shouldSaveRetry( rules, ruleCount, i ) )
					retries[retryCount++] = { curElement, i };

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::CHILD: {
				UIWidget* parentElement = curElement->getStyleSheetParentElement();

				if ( NULL == parentElement )
					goto parentMissing;
				curElement = parentElement;
				// Earlier sibling candidates have this same parent, so none can fix its mismatch.
				if ( !selectorRule.matches( curElement, applyPseudo ) )
					goto retryAncestor;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::PREVIOUS_SIBLING: {
				// eepp's | relation moves forward. An earlier candidate can have a different next
				// sibling, so the previous-sibling pruning used for + and ~ does not apply here.
				curElement = curElement->getStyleSheetNextSiblingElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					goto retryMatch;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::DIRECT_SIBLING: {
				curElement = curElement->getStyleSheetPreviousSiblingElement();

				// There are no previous siblings left for an earlier sibling candidate to use.
				if ( NULL == curElement )
					goto retryAncestor;
				if ( !selectorRule.matches( curElement, applyPseudo ) )
					goto retryMatch;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::SIBLING: {
				bool foundSibling = false;

				for ( UIWidget* sibling = curElement->getStyleSheetPreviousSiblingElement();
					  NULL != sibling; sibling = sibling->getStyleSheetPreviousSiblingElement() ) {
					if ( selectorRule.matches( sibling, applyPseudo ) ) {
						if ( shouldSaveRetry( rules, ruleCount, i ) )
							retries[retryCount++] = { sibling, i };
						curElement = sibling;
						foundSibling = true;
						break;
					}
				}

				// Every previous sibling was checked; only changing the ancestor can help.
				if ( !foundSibling )
					goto retryAncestor;

				break; // continue evaluating
			}
		}
	}

	return true;

	// The rare boundary paths below are shared by the descendant and child steps and kept out of
	// the switch, so the hot cases stay small.
descendantSearchStartsAtRoot:
	// The search of rule i starts at curElement, which has no style sheet parent. When it is an
	// <html> with element siblings and the candidate of a preceding ~ search, which kept no
	// checkpoint (see above), resume that search from it. A start re-entered from this rule's own
	// checkpoint is an ancestor match instead, not a ~ candidate.
	if ( retriedRule != i && rules[i - 1].getPatternMatch() == StyleSheetSelectorRule::SIBLING &&
		 hasWidgetParent( curElement ) && hidesParentFromSiblings( curElement ) ) {
		i--;
		goto matchRules;
	}
parentMissing:
	// curElement has no style sheet parent. Only an <html> with element siblings leaves other
	// candidates worth retrying; otherwise every alternative reaches the same root.
	if ( hasWidgetParent( curElement ) && hidesParentFromSiblings( curElement ) )
		goto retryMatch;
	return false;

retryAncestor:
	// Discard choices sharing the failed parent/sibling search before changing ancestry.
	while ( retryCount > 0 && rules[retries[retryCount - 1].ruleIndex].getPatternMatch() !=
								  StyleSheetSelectorRule::DESCENDANT ) {
		retryCount--;
	}
retryMatch:
	if ( 0 == retryCount )
		return false;
	const SelectorRetry retry = retries[--retryCount];
	// Re-enter the same combinator at its last match; it advances to the next candidate.
	curElement = retry.element;
	i = retry.ruleIndex;
	retriedRule = i;
	goto matchRules;
}

} // namespace

namespace {

// Larger checkpoint arrays for long selectors. Kept out of line so only the selectors that need
// them reserve the bigger frame; the common path keeps its small one.
template <size_t Capacity>
EE_SELECTOR_COLD bool matchWithStackRetries( const std::vector<StyleSheetSelectorRule>& rules,
											 UIWidget* element, const bool& applyPseudo ) {
	SelectorRetry retries[Capacity];
	return matchComplexSelector( rules, element, applyPseudo, retries );
}

} // namespace

bool StyleSheetSelector::selectComplex( UIWidget* element, const bool& applyPseudo ) const {
	// Uninitialized, so entries need no eager clearing.
	constexpr size_t InlineRetryCapacity = 16;
	SelectorRetry retries[InlineRetryCapacity];
	const size_t ruleCount = mSelectorRules.size();
	if ( ruleCount <= InlineRetryCapacity )
		return matchComplexSelector( mSelectorRules, element, applyPseudo, retries );

	// Most calls reject the subject itself; do so before sizing storage for a long selector.
	if ( !mSelectorRules[0].matches( element, applyPseudo ) )
		return false;

	// Only rules that save checkpoints need storage, at most one live entry each.
	size_t capacity = 0;
	for ( size_t i = 1; i < ruleCount; i++ ) {
		const auto pattern = mSelectorRules[i].getPatternMatch();
		if ( ( pattern == StyleSheetSelectorRule::DESCENDANT ||
			   pattern == StyleSheetSelectorRule::SIBLING ) &&
			 shouldSaveRetry( mSelectorRules, ruleCount, i ) ) {
			capacity++;
		}
	}
	if ( capacity <= InlineRetryCapacity )
		return matchComplexSelector( mSelectorRules, element, applyPseudo, retries );
	// The helpers' arrays occupy 512 B and 1 KiB on 64-bit targets, on top of the caller's frame.
	// Matching is iterative, so retry storage does not grow with backtracking depth.
	if ( capacity <= 32 )
		return matchWithStackRetries<32>( mSelectorRules, element, applyPseudo );
	if ( capacity <= 64 )
		return matchWithStackRetries<64>( mSelectorRules, element, applyPseudo );

	// Real style sheets do not reach this.
	std::unique_ptr<SelectorRetry[]> heapRetries( new SelectorRetry[capacity] );
	return matchComplexSelector( mSelectorRules, element, applyPseudo, heapRetries.get() );
}

SmallVector<UIWidget*, 8> StyleSheetSelector::getRelatedElements( UIWidget* element,
																  bool applyPseudo ) const {
	SmallVector<UIWidget*, 8> elements;
	const size_t ruleCount = mSelectorRules.size();
	TrackedRules trackedRules{ mSelectorRules };
	size_t lastTracked = 0;
	size_t firstAmbiguous = ruleCount;
	size_t ambiguousCount = 0;
	size_t lastSearch = 0;
	bool ancestorsOnly = true;

	// Every matching path contributes its related elements (Selectors 4, section 17.3 accepts
	// any of them). The strategy is chosen by how much the paths can differ, cheapest first.
	for ( size_t i = 1; i < ruleCount; i++ ) {
		const bool tracked = tracksStateChanges( mSelectorRules[i] );
		if ( tracked && i < 64 )
			trackedRules.mask |= Uint64( 1 ) << i;
		const auto pattern = mSelectorRules[i].getPatternMatch();
		if ( tracked )
			lastTracked = i;
		ancestorsOnly = ancestorsOnly && ( pattern == StyleSheetSelectorRule::CHILD ||
										   pattern == StyleSheetSelectorRule::DESCENDANT );
		if ( pattern == StyleSheetSelectorRule::DESCENDANT ||
			 pattern == StyleSheetSelectorRule::SIBLING ) {
			lastSearch = i;
		}
		if ( ( pattern == StyleSheetSelectorRule::DESCENDANT ||
			   pattern == StyleSheetSelectorRule::SIBLING ) &&
			 !isNearestMatchEnough( mSelectorRules, i, tracked ) && 0 == ambiguousCount++ ) {
			firstAmbiguous = i;
		}
	}

	// Only compounds after the subject with pseudo-classes make an element related.
	if ( 0 == lastTracked )
		return elements;

	// Tracked compounds all precede the first combinator with alternatives that matter, so every
	// matching path shares the prefix elements.
	if ( lastTracked < firstAmbiguous ) {
		// Without any such combinator, one matching walk both decides the match and collects.
		if ( 0 == ambiguousCount ) {
			if ( !mSelectorRules[0].matches( element, applyPseudo ) ||
				 NULL == matchFixedRules( mSelectorRules, trackedRules, 1, ruleCount, element,
										  applyPseudo, elements ) ) {
				elements.clear();
			}
			return elements;
		}
		// A match exists, so only the nearest-match searches in the prefix need matching.
		if ( selectComplex( element, applyPseudo ) ) {
			matchFixedRules( mSelectorRules, trackedRules, 1, lastTracked + 1, element, applyPseudo,
							 elements, false );
		}
		return elements;
	}

	if ( !mSelectorRules[0].matches( element, applyPseudo ) )
		return elements;

	// A single combinator with alternatives that matter: its candidate fixes the whole path, so
	// one scan over the candidates finds every path. That stays linear only while the rest of the
	// selector is evaluated once, or cheaply and without related elements per candidate:
	// - ~ candidates share their parent, so a rest that starts by climbing has one result for
	//   all of them, and it never reaches the candidates' level again.
	// - Otherwise the rest must neither search nor track: a repeated search, or deduplicating
	//   its related elements against a growing result, would be quadratic. The collectors below
	//   share those states instead.
	// Either way a candidate can only repeat a prefix element.
	if ( 1 == ambiguousCount ) {
		const StyleSheetSelectorRule& rule = mSelectorRules[firstAmbiguous];
		const auto pattern = rule.getPatternMatch();
		const auto nextPattern = firstAmbiguous + 1 < ruleCount
									 ? mSelectorRules[firstAmbiguous + 1].getPatternMatch()
									 : StyleSheetSelectorRule::ANY;
		const bool sharedSuffix = pattern == StyleSheetSelectorRule::SIBLING &&
								  ( nextPattern == StyleSheetSelectorRule::CHILD ||
									nextPattern == StyleSheetSelectorRule::DESCENDANT );
		// Only one search has alternatives that matter, so any later one is a search of the rest.
		const bool suffixSearches = lastSearch > firstAmbiguous;
		const bool suffixTracks = lastTracked > firstAmbiguous;

		if ( sharedSuffix || ( !suffixSearches && !suffixTracks ) ) {
			UIWidget* chainStart = matchFixedRules( mSelectorRules, trackedRules, 1, firstAmbiguous,
													element, applyPseudo, elements );
			if ( NULL == chainStart ) {
				elements.clear();
				return elements;
			}
			const bool tracked = trackedRules[firstAmbiguous];
			const size_t prefixCount = elements.size();
			bool matched = false;
			UIWidget* suffixParent = NULL;
			bool suffixComplete = false;
			SmallVector<UIWidget*, 8> suffix;
			for ( UIWidget* candidate = combinatorStep( chainStart, pattern ); NULL != candidate;
				  candidate = combinatorStep( candidate, pattern ) ) {
				if ( !rule.matches( candidate, applyPseudo ) )
					continue;
				if ( sharedSuffix ) {
					// A candidate with a hidden parent, an <html>, cannot climb at all.
					UIWidget* parent = candidate->getStyleSheetParentElement();
					if ( NULL == parent )
						continue;
					if ( parent != suffixParent ) {
						suffixParent = parent;
						suffix.clear();
						suffixComplete = NULL != matchFixedRules( mSelectorRules, trackedRules,
																  firstAmbiguous + 1, ruleCount,
																  candidate, applyPseudo, suffix );
						// Only prefix elements precede these: no candidate has been added yet.
						if ( suffixComplete ) {
							for ( UIWidget* suffixElement : suffix )
								addRelated( elements, suffixElement );
						}
					}
					if ( !suffixComplete )
						continue;
				} else if ( NULL == matchFixedRules( mSelectorRules, trackedRules,
													 firstAmbiguous + 1, ruleCount, candidate,
													 applyPseudo, suffix ) ) {
					// The rest tracks nothing here, so suffix stays empty.
					continue;
				}
				matched = true;
				const auto prefixEnd = elements.begin() + prefixCount;
				if ( tracked && std::find( elements.begin(), prefixEnd, candidate ) == prefixEnd )
					elements.push_back( candidate );
			}
			if ( !matched )
				elements.clear();
			return elements;
		}
	}

	// Several combinators with alternatives that matter: paths branch repeatedly, so the
	// collectors share their states instead of following each path.
	if ( ancestorsOnly ) {
		if ( !collectAncestorPathRelated( mSelectorRules, trackedRules, element, applyPseudo,
										  elements ) ) {
			elements.clear();
		}
		return elements;
	}
	RelatedElementCollector( mSelectorRules, applyPseudo ).collect( element, elements );
	return elements;
}

bool StyleSheetSelector::isStructurallyVolatile() const {
	return mStructurallyVolatile;
}

const StyleSheetSelectorRule& StyleSheetSelector::getRule( const Uint32& index ) const {
	return mSelectorRules[index];
}

const std::string& StyleSheetSelector::getSelectorId() const {
	return mSelectorRules[0].getId();
}

const std::string& StyleSheetSelector::getSelectorTagName() const {
	return mSelectorRules[0].getTagName();
}

}}} // namespace EE::UI::CSS
