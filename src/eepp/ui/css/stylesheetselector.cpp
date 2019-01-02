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
	patternMatch( patternMatch )
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
}

void StyleSheetSelector::SelectorRule::match( StyleSheetElement * ) {

}

StyleSheetSelector::StyleSheetSelector() :
	mSpecificity(0),
	mGlobal(false)
{}

StyleSheetSelector::StyleSheetSelector( const std::string& selectorName ) :
	mName( String::toLower( selectorName ) ),
	mSpecificity(0),
	mGlobal(false)
{
	//realParseSelector( mName );

	auto parts = String::split( mName, ' ' );

	for ( auto it = parts.begin(); it != parts.end(); ++it ) {
		parseSelector( *it );
	}
}

Uint32 StyleSheetSelector::getRequiredFlags() const {
	Uint32 flags = 0;

	if ( hasTagName() )
		flags |= TagName;

	if ( hasId() )
		flags |= Id;

	if ( hasClasses() )
		flags |= Class;

	if ( hasPseudoClass() )
		flags |= PseudoClass;

	return flags;
}

const std::string& StyleSheetSelector::getName() const {
	return mName;
};

const std::string& StyleSheetSelector::getTagName() const {
	return mTagName;
}

const std::string StyleSheetSelector::getId() const {
	return mId;
}

const std::vector<std::string> & StyleSheetSelector::getClasses() const {
	return mClasses;
}

const std::string& StyleSheetSelector::getPseudoClass() const {
	return mPseudoClass;
};

bool StyleSheetSelector::hasTagName() const {
	return !mTagName.empty();
}

bool StyleSheetSelector::hasId() const {
	return !mId.empty();
}

bool StyleSheetSelector::hasClasses() const {
	return !mClasses.empty();
}

bool StyleSheetSelector::hasClass( std::string cls ) const {
	return std::find(mClasses.begin(), mClasses.end(), cls) != mClasses.end();
}

bool StyleSheetSelector::hasPseudoClass() const {
	return !mPseudoClass.empty();
}

bool StyleSheetSelector::isGlobal() const {
	return mGlobal;
}

const Uint32& StyleSheetSelector::getSpecificity() const {
	return mSpecificity;
}

void StyleSheetSelector::parseSelector( const std::string& selector ) {
	std::string realSelector = "";
	std::string realPseudoClass = "";

	splitSelectorPseudoClass( selector, realSelector, realPseudoClass );

	if ( !realSelector.empty() ) {
		if ( realSelector[0] == '.' ) {
			mClasses.push_back( realSelector.substr(1) );
			mSpecificity += SpecificityClass;
		} else if ( selector[0] == '#' ) {
			mId = realSelector.substr(1);
			mSpecificity += SpecificityId;
		} else if ( realSelector[0] == '*' ) {
			mSpecificity += SpecificityGlobal;
			mGlobal = true;
		} else {
			mTagName = realSelector;
			mSpecificity += SpecificityTag;
		}
	}

	if ( !realPseudoClass.empty() ) {
		mPseudoClass = realPseudoClass;
		mSpecificity += SpecificityPseudoClass;
	}
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

void StyleSheetSelector::realParseSelector( const std::string& selectorString ) {
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

}}}
