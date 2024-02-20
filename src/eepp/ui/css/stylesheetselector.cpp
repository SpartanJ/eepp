#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/uiwidget.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetSelector::StyleSheetSelector() : mName( "*" ), mSpecificity( 0 ), mCacheable( true ) {
	parseSelector( mName );
}

StyleSheetSelector::StyleSheetSelector( const std::string& selectorName ) :
	mName( String::toLower( selectorName ) ),
	mSpecificity( 0 ),
	mCacheable( true ),
	mStructurallyVolatile( false ) {
	parseSelector( mName );
}

const std::string& StyleSheetSelector::getName() const {
	return mName;
}

const Uint32& StyleSheetSelector::getSpecificity() const {
	return mSpecificity;
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
		// shold be
		// Element>.class #id
		removeExtraSpaces( selector );

		std::string buffer;
		StyleSheetSelectorRule::PatternMatch curPatternMatch = StyleSheetSelectorRule::ANY;

		for ( auto charIt = selector.rbegin(); charIt != selector.rend(); ++charIt ) {
			char curChar = *charIt;

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

	UIWidget* curElement = element;

	for ( size_t i = 0; i < mSelectorRules.size(); i++ ) {
		const StyleSheetSelectorRule& selectorRule = mSelectorRules[i];

		switch ( selectorRule.getPatternMatch() ) {
			case StyleSheetSelectorRule::ANY: {
				if ( !selectorRule.matches( curElement, applyPseudo ) )
					return false;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::DESCENDANT: {
				bool foundDescendant = false;

				curElement = curElement->getStyleSheetParentElement();

				while ( NULL != curElement && !foundDescendant ) {
					if ( selectorRule.matches( curElement, applyPseudo ) ) {
						foundDescendant = true;
					} else {
						curElement = curElement->getStyleSheetParentElement();
					}
				}

				if ( !foundDescendant )
					return false;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::CHILD: {
				curElement = curElement->getStyleSheetParentElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					return false;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::DIRECT_SIBLING: {
				curElement = curElement->getStyleSheetPreviousSiblingElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					return false;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::PREVIOUS_SIBLING: {
				curElement = curElement->getStyleSheetNextSiblingElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					return false;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::SIBLING: {
				bool foundSibling = false;
				UIWidget* prevSibling = curElement->getStyleSheetPreviousSiblingElement();
				UIWidget* nextSibling = curElement->getStyleSheetNextSiblingElement();

				while ( NULL != prevSibling && !foundSibling ) {
					if ( selectorRule.matches( prevSibling, applyPseudo ) ) {
						foundSibling = true;
					} else {
						prevSibling = prevSibling->getStyleSheetPreviousSiblingElement();
					}
				}

				if ( !foundSibling ) {
					while ( NULL != nextSibling && !foundSibling ) {
						if ( selectorRule.matches( nextSibling, applyPseudo ) ) {
							foundSibling = true;
						} else {
							nextSibling = nextSibling->getStyleSheetNextSiblingElement();
						}
					}
				}

				if ( !foundSibling )
					return false;

				break; // continue evaluating
			}
		}
	}

	return true;
}

std::vector<UIWidget*> StyleSheetSelector::getRelatedElements( UIWidget* element,
															   bool applyPseudo ) const {
	static std::vector<UIWidget*> EMPTY_ELEMENTS;
	std::vector<UIWidget*> elements;
	if ( mSelectorRules.empty() )
		return elements;

	UIWidget* curElement = element;

	for ( size_t i = 0; i < mSelectorRules.size(); i++ ) {
		const StyleSheetSelectorRule& selectorRule = mSelectorRules[i];

		switch ( selectorRule.getPatternMatch() ) {
			case StyleSheetSelectorRule::ANY: {
				if ( !selectorRule.matches( curElement, applyPseudo ) )
					return EMPTY_ELEMENTS;

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::DESCENDANT: {
				bool foundDescendant = false;

				curElement = curElement->getStyleSheetParentElement();

				while ( NULL != curElement && !foundDescendant ) {
					if ( selectorRule.matches( curElement, applyPseudo ) ) {
						foundDescendant = true;
					} else {
						curElement = curElement->getStyleSheetParentElement();
					}
				}

				if ( !foundDescendant )
					return EMPTY_ELEMENTS;

				if ( 0 != i && ( selectorRule.hasPseudoClasses() ||
								 selectorRule.hasStructuralPseudoClasses() ) ) {
					elements.push_back( curElement );
				}

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::CHILD: {
				curElement = curElement->getStyleSheetParentElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					return EMPTY_ELEMENTS;

				if ( 0 != i && ( selectorRule.hasPseudoClasses() ||
								 selectorRule.hasStructuralPseudoClasses() ) ) {
					elements.push_back( curElement );
				}

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::DIRECT_SIBLING: {
				curElement = curElement->getStyleSheetPreviousSiblingElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					return EMPTY_ELEMENTS;

				if ( 0 != i && ( selectorRule.hasPseudoClasses() ||
								 selectorRule.hasStructuralPseudoClasses() ) ) {
					elements.push_back( curElement );
				}

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::PREVIOUS_SIBLING: {
				curElement = curElement->getStyleSheetNextSiblingElement();

				if ( NULL == curElement || !selectorRule.matches( curElement, applyPseudo ) )
					return EMPTY_ELEMENTS;

				if ( 0 != i && ( selectorRule.hasPseudoClasses() ||
								 selectorRule.hasStructuralPseudoClasses() ) ) {
					elements.push_back( curElement );
				}

				break; // continue evaluating
			}
			case StyleSheetSelectorRule::SIBLING: {
				bool foundSibling = false;
				UIWidget* prevSibling = curElement->getStyleSheetPreviousSiblingElement();
				UIWidget* nextSibling = curElement->getStyleSheetNextSiblingElement();

				while ( NULL != prevSibling && !foundSibling ) {
					if ( selectorRule.matches( prevSibling, applyPseudo ) ) {
						foundSibling = true;
						curElement = prevSibling;
					} else {
						prevSibling = prevSibling->getStyleSheetPreviousSiblingElement();
					}
				}

				if ( !foundSibling ) {
					while ( NULL != nextSibling && !foundSibling ) {
						if ( selectorRule.matches( nextSibling, applyPseudo ) ) {
							foundSibling = true;
							curElement = nextSibling;
						} else {
							nextSibling = nextSibling->getStyleSheetNextSiblingElement();
						}
					}
				}

				if ( !foundSibling )
					return EMPTY_ELEMENTS;

				if ( 0 != i && ( selectorRule.hasPseudoClasses() ||
								 selectorRule.hasStructuralPseudoClasses() ) ) {
					elements.push_back( curElement );
				}

				break; // continue evaluating
			}
		}
	}

	return elements;
}

bool StyleSheetSelector::isStructurallyVolatile() const {
	return mStructurallyVolatile;
}

const StyleSheetSelectorRule& StyleSheetSelector::getRule( const Uint32& index ) {
	return mSelectorRules[index];
}

const std::string& StyleSheetSelector::getSelectorId() const {
	return mSelectorRules[0].getId();
}

const std::string& StyleSheetSelector::getSelectorTagName() const {
	return mSelectorRules[0].getTagName();
}

}}} // namespace EE::UI::CSS
