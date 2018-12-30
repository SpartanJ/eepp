#include <eepp/ui/css/stylesheetselector.hpp>
#include <algorithm>

namespace EE { namespace UI { namespace CSS {


StyleSheetSelector::StyleSheetSelector( const std::string& selectorName ) :
	name( String::toLower( selectorName ) ),
	specificity(0)
{
	auto parts = String::split( name, ' ' );

	for ( auto it = parts.begin(); it != parts.end(); ++it ) {
		parseSelector( *it );
	}
}

Uint32 StyleSheetSelector::getRequiredFlags() {
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

const std::string& StyleSheetSelector::getName() const { return name; };

const std::string& StyleSheetSelector::getTagName() const { return tagName; }

const std::string StyleSheetSelector::getId() const { return id; }

const std::vector<std::string> StyleSheetSelector::getClasses() const { return classes; }

const std::string& StyleSheetSelector::getPseudoClass() const { return pseudoClass; };

bool StyleSheetSelector::hasTagName() { return !tagName.empty(); }

bool StyleSheetSelector::hasId() { return !id.empty(); }

bool StyleSheetSelector::hasClasses() { return !classes.empty(); }

bool StyleSheetSelector::hasClass( std::string cls ) { return std::find(classes.begin(), classes.end(), cls) != classes.end(); }

bool StyleSheetSelector::hasPseudoClass() { return !pseudoClass.empty(); }

Uint32 StyleSheetSelector::getSpecificity() { return specificity; }

void StyleSheetSelector::parseSelector( const std::string& selector ) {
	auto selPseudo = String::split( selector, ':' );

	if ( !selPseudo.empty() ) {
		std::string rselector( selPseudo[0] );

		if ( !rselector.empty() ) {
			if ( rselector[0] == '.' ) {
				classes.push_back( rselector.substr(1) );
				specificity += SpecificityClass;
			} else if ( selector[0] == '#' ) {
				id = rselector.substr(1);
				specificity += SpecificityId;
			} else if ( selector[0] == '*' ) {
				specificity += SpecificityGlobal;
			} else {
				tagName = rselector;
				specificity += SpecificityTag;
			}
		}

		if ( selPseudo.size() > 1 ) {
			pseudoClass = selPseudo[1];
			specificity += SpecificityPseudoClass;
		}
	}
}

}}}
