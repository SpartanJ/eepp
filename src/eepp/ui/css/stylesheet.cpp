#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

void StyleSheet::addNode( const StyleSheetNode& node ) {
	mNodes[ node.getSelector().getName() ] = node;
}

void StyleSheet::combineNode( const StyleSheetNode& node ) {
	auto nodeIt = mNodes.find( node.getSelector().getName() );

	if ( nodeIt == mNodes.end() ) {
		addNode( node );
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
		StyleSheetNode& style = it->second;

		style.print();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto it = styleSheet.getNodes().begin(); it != styleSheet.getNodes().end(); ++it ) {
		combineNode( it->second );
	}
}

StyleSheet::StyleSheetPseudoClassProperties StyleSheet::getElementProperties( StyleSheetElement * element ) {
	StyleSheetPseudoClassProperties propertiesSelectedByPseudoClass;

	for ( auto it = mNodes.begin(); it != mNodes.end(); ++it ) {
		StyleSheetNode& node = it->second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.matches( element ) ) {
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

const StyleSheet::StyleSheetNodeList& StyleSheet::getNodes() const {
	return mNodes;
}

}}}
