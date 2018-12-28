#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

void StyleSheet::addNode( StyleSheetNode node ) {
	nodes.push_back( node );
}

StyleSheetProperties StyleSheet::find( const std::string& tagName, const std::string& id, const std::vector<std::string>& classes, const std::string& pseudoClass ) {
	StyleSheetProperties propertiesSelected;
	Uint32 lastSpecificity = -1;

	for ( auto it = nodes.begin(); it != nodes.end(); ++it ) {
		StyleSheetNode& node = *it;
		StyleSheetSelector& selector = node.selector;

		Uint32 flags = 0;

		if ( selector.hasTagName() && !tagName.empty() && selector.getTagName() == tagName ) {
			flags |= StyleSheetSelector::TagName;
		}

		if ( selector.hasId() && !id.empty() && selector.getId() == id ) {
			flags |= StyleSheetSelector::Id;
		}

		if ( selector.hasClasses() && !classes.empty() ) {
			bool hasClasses = true;
			for ( auto cit = classes.begin(); cit != classes.end(); ++cit ) {
				if ( !selector.hasClass( *cit ) ) {
					hasClasses = false;
					break;
				}
			}

			if ( hasClasses ) {
				flags |= StyleSheetSelector::Class;
			}
		}

		if ( selector.hasPseudoClass() && !pseudoClass.empty() && selector.getPseudoClass() == pseudoClass ) {
			flags |= StyleSheetSelector::PseudoClass;
		}

		if ( flags == selector.getRequiredFlags() && selector.getSpecificity() > lastSpecificity ) {
			propertiesSelected = node.properties;
			lastSpecificity = selector.getSpecificity();
		}
	}

	return propertiesSelected;
}

}}}
