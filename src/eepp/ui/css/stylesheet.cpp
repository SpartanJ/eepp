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
			for ( auto& pit : node.getProperties() )
				currentNode.setProperty( pit.second );
		}
	}
}

bool StyleSheet::isEmpty() const {
	return mNodes.empty();
}

void StyleSheet::print() {
	for ( auto& it : mNodes ) {
		StyleSheetStyle& style = it.second;

		style.print();
	}
}

void StyleSheet::combineStyleSheet( const StyleSheet& styleSheet ) {
	for ( auto& it : styleSheet.getStyles() ) {
		combineStyle( it.second );
	}
}

StyleSheet::StyleSheetPseudoClassProperties StyleSheet::getElementPropertiesByState( StyleSheetElement * element ) {
	StyleSheetPseudoClassProperties propertiesSelectedByPseudoClass;

	for ( const auto& it : mNodes ) {
		const StyleSheetStyle& node = it.second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.isCacheable() && selector.select( element, false ) ) {
			for ( const auto& pit : node.getProperties() ) {
				StyleSheetProperties& pseudoClassProperties = propertiesSelectedByPseudoClass[selector.getPseudoClass()];
				const StyleSheetProperty& property = pit.second;
				const auto& pcit = pseudoClassProperties.find( property.getName() );
				const StyleSheetProperty& propertyRight = pcit->second;

				if ( pcit == pseudoClassProperties.end() || property.getSpecificity() >= propertyRight.getSpecificity() ) {
					pseudoClassProperties[ property.getName() ] = property;
				}
			}
		}
	}

	return propertiesSelectedByPseudoClass;
}

StyleSheetStyleVector StyleSheet::getElementStyles( StyleSheetElement * element , const bool& applyPseudo ) {
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

StyleSheetStyleVector StyleSheet::getCacheableElementStyles( StyleSheetElement * element, const bool& applyPseudo ) {
	StyleSheetStyleVector styles;

	for ( const auto& it : mNodes ) {
		const StyleSheetStyle& node = it.second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( selector.isCacheable() && selector.select( element, applyPseudo ) ) {
			styles.push_back( node );
		}
	}

	return styles;
}

StyleSheetStyleVector StyleSheet::getNoncacheableElementStyles( StyleSheetElement * element, const bool& applyPseudo ) {
	StyleSheetStyleVector styles;

	for ( const auto& it : mNodes ) {
		const StyleSheetStyle& node = it.second;
		const StyleSheetSelector& selector = node.getSelector();

		if ( !selector.isCacheable() && selector.select( element, applyPseudo ) ) {
			styles.push_back( node );
		}
	}

	return styles;
}

const StyleSheetStyleList& StyleSheet::getStyles() const {
	return mNodes;
}

}}}
