#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

void StyleSheet::addStyle( const StyleSheetStyle& node ) {
	mNodes[ node.getSelector().getName() ] = node;
}

void StyleSheet::combineStyle( const StyleSheetStyle& node ) {
	auto nodeIt = mNodes.find( node.getSelector().getName() );

	if ( nodeIt == mNodes.end() ) {
		addStyle( node );
	} else {
		auto currentNode = nodeIt->second;

		if ( node.getSelector().getSpecificity() > currentNode.getSelector().getSpecificity() ) {
			for ( auto pit = node.getProperties().begin(); pit != node.getProperties().end(); ++pit )
				currentNode.setProperty( pit->second );
		}
	}
}

bool StyleSheet::isEmpty() const {
	return mNodes.empty();
}

void StyleSheet::print() {
	for ( auto it = mNodes.begin(); it != mNodes.end(); ++it ) {
		StyleSheetStyle& style = it->second;

		style.print();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto it = styleSheet.getStyles().begin(); it != styleSheet.getStyles().end(); ++it ) {
		combineStyle( it->second );
	}
}

StyleSheet::StyleSheetPseudoClassProperties StyleSheet::getElementPropertiesByState( StyleSheetElement * element ) {
	StyleSheetPseudoClassProperties propertiesSelectedByPseudoClass;

	for ( auto it = mNodes.begin(); it != mNodes.end(); ++it ) {
		StyleSheetStyle& node = it->second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.isCacheable() && selector.select( element, false ) ) {
			for ( auto pit = node.getProperties().begin(); pit != node.getProperties().end(); ++pit ) {
				StyleSheetProperties& pseudoClassProperties = propertiesSelectedByPseudoClass[selector.getPseudoClass()];
				auto pcit = pseudoClassProperties.find( pit->second.getName() );

				if ( pcit == pseudoClassProperties.end() || pit->second.getSpecificity() >= pcit->second.getSpecificity() ) {
					pseudoClassProperties[ pit->second.getName() ] = pit->second;
				}
			}
		}
	}

	return propertiesSelectedByPseudoClass;
}

StyleSheet::StyleSheetStyleVector StyleSheet::getElementStyles( StyleSheetElement * element ) {
	StyleSheetStyleVector styles;

	for ( auto it = mNodes.begin(); it != mNodes.end(); ++it ) {
		StyleSheetStyle& node = it->second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.select( element, false ) ) {
			styles.push_back( node );
		}
	}

	return styles;
}

const StyleSheet::StyleSheetStyleList& StyleSheet::getStyles() const {
	return mNodes;
}

}}}
