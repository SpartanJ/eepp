#include <eepp/ui/css/stylesheet.hpp>
#include <eepp/ui/css/stylesheetelement.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselector.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheet::StyleSheet() {}

void StyleSheet::addStyle( const StyleSheetStyle& node ) {
	auto nodeIt = mNodes.find( node.getSelector().getName() );

	if ( nodeIt == mNodes.end() ) {
		mNodes[node.getSelector().getName()] = node;
	} else {
		StyleSheetStyle& currentNode = nodeIt->second;

		for ( auto& pit : node.getProperties() )
			currentNode.setProperty( pit.second );

		for ( auto& vit : node.getVariables() )
			currentNode.setVariable( vit.second );
	}
}

void StyleSheet::combineStyle( const StyleSheetStyle& node ) {
	auto nodeIt = mNodes.find( node.getSelector().getName() );

	if ( nodeIt == mNodes.end() ) {
		addStyle( node );
	} else {
		auto& currentNode = nodeIt->second;

		if ( node.getSelector().getSpecificity() >= currentNode.getSelector().getSpecificity() ) {
			for ( auto& pit : node.getProperties() )
				currentNode.setProperty( pit.second );

			for ( auto& vit : node.getVariables() )
				currentNode.setVariable( vit.second );
		}
	}
}

bool StyleSheet::isEmpty() const {
	return mNodes.empty();
}

void StyleSheet::print() {
	for ( auto& it : mNodes ) {
		StyleSheetStyle& style = it.second;

		std::cout << style.build();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto& it : styleSheet.getStyles() ) {
		combineStyle( it.second );
	}
}

StyleSheetStyleVector StyleSheet::getElementStyles( StyleSheetElement* element,
													const bool& applyPseudo ) {
	StyleSheetStyleVector styles;

	for ( const auto& it : mNodes ) {
		const StyleSheetStyle& node = it.second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.select( element, applyPseudo ) ) {
			styles.push_back( node );
		}
	}

	return styles;
}

const StyleSheetStyleList& StyleSheet::getStyles() const {
	return mNodes;
}

}}} // namespace EE::UI::CSS
