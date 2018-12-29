#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

void StyleSheet::addNode( StyleSheetNode node ) {
	nodes.push_back( node );
}

bool StyleSheet::isEmpty() const {
	return nodes.empty();
}

StyleSheetProperties StyleSheet::getElementProperties( StyleSheetElement * element, const std::string& pseudoClass ) {
	StyleSheetProperties propertiesSelected;
	Uint32 lastSpecificity = 0;

	for ( auto it = nodes.begin(); it != nodes.end(); ++it ) {
		StyleSheetNode& node = *it;
		StyleSheetSelector& selector = node.selector;

		Uint32 flags = 0;

		if ( selector.hasTagName() && !element->getStyleSheetTag().empty() && selector.getTagName() == element->getStyleSheetTag() ) {
			flags |= StyleSheetSelector::TagName;
		}

		if ( selector.hasId() && !element->getStyleSheetId().empty() && selector.getId() == element->getStyleSheetId() ) {
			flags |= StyleSheetSelector::Id;
		}

		if ( selector.hasClasses() && !element->getStyleSheetClasses().empty() ) {
			bool hasClasses = true;
			for ( auto cit = element->getStyleSheetClasses().begin(); cit != element->getStyleSheetClasses().end(); ++cit ) {
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
			for ( auto pit = node.properties.begin(); pit != node.properties.end(); ++pit )
				propertiesSelected[ pit->second.name ] = pit->second;

			lastSpecificity = selector.getSpecificity();
		}
	}

	return propertiesSelected;
}

}}}
