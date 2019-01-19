#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace CSS {

static void splitSelectorPseudoClass( const std::string& selector, std::string& realSelector, std::string& realPseudoClass ) {
	if ( !selector.empty() ) {
		bool lastWasColon = false;

		for ( int i = (Int32)selector.size() - 1; i >= 0; i-- ) {
			char curChar = selector[i];

			if ( lastWasColon ) {
				if ( ':' == curChar ) {
					// no pseudo class
					realSelector = selector;
				} else {
					if ( i+2 <= (int)selector.size() ) {
						realSelector = selector.substr(0,i+1);
						realPseudoClass = selector.substr(i+2);
					} else {
						realSelector = selector;
					}
				}

				return;
			} else if ( ':' == curChar ) {
				lastWasColon = true;
			}
		}

		if ( lastWasColon ) {
			if ( selector.size() > 1 )
				realPseudoClass = selector.substr(1);
		} else {
			realSelector = selector;
		}
	}
}

StyleSheetSelector::SelectorRule::SelectorRule( const std::string& selectorFragment, StyleSheetSelector::PatternMatch patternMatch ) :
	specificity(0),
	patternMatch( patternMatch ),
	requirementFlags(0)
{
	parseFragment( selectorFragment );
}

void StyleSheetSelector::SelectorRule::pushSelectorTypeIdentifier( SelectoryTypeIdentifier selectorTypeIdentifier, std::string name ) {
	switch ( selectorTypeIdentifier ) {
		case TAG:
			tagName = name;
			specificity += StyleSheetSelector::SpecificityTag;
			break;
		case CLASS:
			classes.push_back( name );
			specificity += StyleSheetSelector::SpecificityClass;
			break;
		case ID:
			id = name;
			specificity += StyleSheetSelector::SpecificityId;
			break;
		default:
			break;
	}
}

void StyleSheetSelector::SelectorRule::parseFragment( const std::string& selectorFragment ) {
	SelectoryTypeIdentifier curSelectorType = TAG;
	std::string buffer;

	for ( auto charIt = selectorFragment.begin(); charIt != selectorFragment.end(); ++charIt ) {
		char curChar = *charIt;

		switch ( curChar ) {
			case CLASS:
			{
				if ( !buffer.empty() ) {
					pushSelectorTypeIdentifier( curSelectorType, buffer );
					buffer.clear();
				}

				curSelectorType = CLASS;

				break;
			}
			case ID:
			{
				if ( !buffer.empty() ) {
					pushSelectorTypeIdentifier( curSelectorType, buffer );
					buffer.clear();
				}

				curSelectorType = ID;

				break;
			}
			default:
			{
				buffer += curChar;
				break;
			}
		}
	}

	if ( !buffer.empty() ) {
		pushSelectorTypeIdentifier( curSelectorType, buffer );
	}

	if ( !tagName.empty() )
		requirementFlags |= StyleSheetSelector::TagName;

	if ( !id.empty() )
		requirementFlags |= StyleSheetSelector::Id;

	if ( !classes.empty() )
		requirementFlags |= StyleSheetSelector::Class;
}

bool StyleSheetSelector::SelectorRule::hasClass( const std::string& cls ) const {
	return std::find(classes.begin(), classes.end(), cls) != classes.end();
}

bool StyleSheetSelector::SelectorRule::matches( StyleSheetElement * element ) const {
	Uint32 flags = 0;

	if ( tagName == "*" )
		return true;

	if ( !tagName.empty() && !element->getStyleSheetTag().empty() && tagName == element->getStyleSheetTag() ) {
		flags |= StyleSheetSelector::TagName;
	}

	if ( !id.empty() && !element->getStyleSheetId().empty() && id == element->getStyleSheetId() ) {
		flags |= StyleSheetSelector::Id;
	}

	if ( !classes.empty() && !element->getStyleSheetClasses().empty() ) {
		bool hasClasses = true;
		for ( auto cit = element->getStyleSheetClasses().begin(); cit != element->getStyleSheetClasses().end(); ++cit ) {
			if ( !hasClass( *cit ) ) {
				hasClasses = false;
				break;
			}
		}

		if ( hasClasses ) {
			flags |= StyleSheetSelector::Class;
		}
	}

	return requirementFlags == flags;
}

StyleSheetSelector::StyleSheetSelector() :
	mSpecificity(0)
{}

StyleSheetSelector::StyleSheetSelector( const std::string& selectorName ) :
	mName( String::toLower( selectorName ) ),
	mSpecificity(0)
{
	parseSelector( mName );
}

const std::string &StyleSheetSelector::getName() const {
	return mName;
}

const std::string& StyleSheetSelector::getPseudoClass() const {
	return mPseudoClass;
};

const Uint32& StyleSheetSelector::getSpecificity() const {
	return mSpecificity;
}

void removeExtraSpaces( std::string& string ) {
	string = String::trim( string );
	String::replaceAll( string, "   ", " " );
	String::replaceAll( string, "  ", " " );
	String::replaceAll( string, " > ", ">" );
	String::replaceAll( string, " + ", "+" );
	String::replaceAll( string, " ~ ", "~" );
}

void StyleSheetSelector::addSelectorRule(std::string& buffer, PatternMatch& curPatternMatch , const PatternMatch& newPatternMatch ) {
	SelectorRule selectorRule( buffer, curPatternMatch );
	mSelectorRules.push_back( selectorRule );
	curPatternMatch = newPatternMatch;
	buffer.clear();
	mSpecificity += selectorRule.getSpecificity();
}

void StyleSheetSelector::parseSelector( const std::string& selectorString ) {
	if ( !selectorString.empty() ) {
		std::string selector = "";

		// Separates the selector and the dynamic pseudo-class
		splitSelectorPseudoClass( selectorString, selector, mPseudoClass );

		// Remove spaces that means nothing to the selector logic
		// for example:
		// Element > .class #id
		// shold be
		// Element>.class #id
		removeExtraSpaces( selector );

		std::string buffer;
		PatternMatch curPatternMatch = ANY;

		for ( auto charIt = selector.rbegin(); charIt != selector.rend(); ++charIt ) {
			char curChar = *charIt;

			switch ( curChar ) {
				case DESCENDANT:
					addSelectorRule( buffer, curPatternMatch, DESCENDANT );
					break;
				case CHILD:
					addSelectorRule( buffer, curPatternMatch, CHILD );
					break;
				case DIRECT_SIBLING:
					addSelectorRule( buffer, curPatternMatch, DIRECT_SIBLING );
					break;
				case SIBLING:
					addSelectorRule( buffer, curPatternMatch, SIBLING );
					break;
				default:
					buffer = curChar + buffer;
					break;
			}
		}

		if ( !buffer.empty() ) {
			addSelectorRule( buffer, curPatternMatch, ANY );
			buffer.clear();
		}
	}
}

bool StyleSheetSelector::matches( StyleSheetElement * element ) const {
	if ( mSelectorRules.empty() )
		return false;

	StyleSheetElement * curElement = element;

	for ( size_t i = 0; i < mSelectorRules.size(); i++ ) {
		const SelectorRule& selectorRule = mSelectorRules[i];

		switch ( selectorRule.getPatternMatch() ) {
			case ANY:
			{
				if ( !selectorRule.matches( curElement ) )
					return false;

				break; // continue evaluating
			}
			case DESCENDANT:
			{
				bool foundDescendant = false;

				curElement = curElement->getStyleSheetParentElement();

				while ( NULL != curElement && !foundDescendant ) {
					if  ( selectorRule.matches( curElement ) ) {
						foundDescendant = true;
					} else {
						curElement = curElement->getStyleSheetParentElement();
					}
				}

				if ( !foundDescendant )
					return false;

				break; // continue evaluating
			}
			case CHILD:
			{
				curElement = curElement->getStyleSheetParentElement();

				if ( NULL == curElement || !selectorRule.matches( curElement ) )
					return false;

				break; // continue evaluating
			}
			case DIRECT_SIBLING:
			{
				curElement = curElement->getStyleSheetPreviousSiblingElement();

				if ( NULL == curElement || !selectorRule.matches( curElement ) )
					return false;

				break; // continue evaluating
			}
			case SIBLING:
			{
				bool foundSibling = false;
				StyleSheetElement * prevSibling = curElement->getStyleSheetPreviousSiblingElement();
				StyleSheetElement * nextSibling = curElement->getStyleSheetNextSiblingElement();

				while ( NULL != prevSibling && !foundSibling ) {
					if ( selectorRule.matches( prevSibling ) ) {
						foundSibling = true;
					} else {
						prevSibling = prevSibling->getStyleSheetPreviousSiblingElement();
					}
				}

				if ( !foundSibling ) {
					while ( NULL != nextSibling && !foundSibling ) {
						if ( selectorRule.matches( nextSibling ) ) {
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

}}}
