#include <eepp/ui/css/stylesheetselector.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace CSS {

StyleSheetSelector::StyleSheetSelector() :
	mSpecificity(0),
	mGlobal(false)
{}

StyleSheetSelector::StyleSheetSelector( const std::string& selectorName ) :
	mName( String::toLower( selectorName ) ),
	mSpecificity(0),
	mGlobal(false)
{
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

}}}
