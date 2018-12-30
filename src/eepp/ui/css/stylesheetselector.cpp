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

const Uint32& StyleSheetSelector::getSpecificity() const { return mSpecificity; }

void StyleSheetSelector::parseSelector( const std::string& selector ) {
	auto selPseudo = String::split( selector, ':' );

	if ( !selPseudo.empty() ) {
		std::string rselector( selPseudo[0] );

		if ( !rselector.empty() ) {
			if ( rselector[0] == '.' ) {
				mClasses.push_back( rselector.substr(1) );
				mSpecificity += SpecificityClass;
			} else if ( selector[0] == '#' ) {
				mId = rselector.substr(1);
				mSpecificity += SpecificityId;
			} else if ( selector[0] == '*' ) {
				mSpecificity += SpecificityGlobal;
				mGlobal = true;
			} else {
				mTagName = rselector;
				mSpecificity += SpecificityTag;
			}
		}

		if ( selPseudo.size() > 1 ) {
			mPseudoClass = selPseudo[1];
			mSpecificity += SpecificityPseudoClass;
		}
	}
}

}}}
