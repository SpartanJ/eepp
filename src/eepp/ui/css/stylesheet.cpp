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
		addNode( it->second );
	}
}

StyleSheetProperties StyleSheet::getElementProperties( StyleSheetElement * element, const std::string& pseudoClass ) {
	StyleSheetProperties propertiesSelected;
	Uint32 lastSpecificity = 0;

	for ( auto it = mNodes.begin(); it != mNodes.end(); ++it ) {
		StyleSheetNode& node = it->second;
		const StyleSheetSelector& selector = node.getSelector();

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

		if ( selector.isGlobal() ) {
			if ( !pseudoClass.empty() ) {
				flags |= StyleSheetSelector::PseudoClass;
			}
		} else if ( selector.hasPseudoClass() ) {
			if ( !pseudoClass.empty() && selector.getPseudoClass() == pseudoClass )
				flags |= StyleSheetSelector::PseudoClass;
		} else if ( !selector.hasPseudoClass() && !pseudoClass.empty() ) {
			flags |= StyleSheetSelector::PseudoClass;
		}

		if ( flags == selector.getRequiredFlags() && selector.getSpecificity() > lastSpecificity ) {
			for ( auto pit = node.getProperties().begin(); pit != node.getProperties().end(); ++pit )
				propertiesSelected[ pit->second.getName() ] = pit->second;

			lastSpecificity = selector.getSpecificity();
		}
	}

	return propertiesSelected;
}

const StyleSheet::StyleSheetNodeList& StyleSheet::getNodes() const {
	return mNodes;
}

}}}
