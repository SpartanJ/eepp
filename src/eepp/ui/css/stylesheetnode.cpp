#include <eepp/ui/css/stylesheetnode.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheetNode::StyleSheetNode()
{}

StyleSheetNode::StyleSheetNode( const std::string& selector, const StyleSheetProperties& properties ) :
	mSelector( selector ),
	mProperties( properties )
{
	for ( auto it = mProperties.begin(); it != mProperties.end(); ++it )
		it->second.setSpecificity( mSelector.getSpecificity() );
}

void StyleSheetNode::print() {
	std::cout << mSelector.getName() << " {" << std::endl;

	for ( StyleSheetProperties::iterator it = mProperties.begin(); it != mProperties.end(); ++it ) {
		StyleSheetProperty& prop = it->second;

		std::cout << "\t" << prop.getName() << ": " << prop.getValue() << ";" << std::endl;
	}

	std::cout << "}" << std::endl;
}

const StyleSheetSelector &StyleSheetNode::getSelector() const {
	return mSelector;
}

const StyleSheetProperties &StyleSheetNode::getProperties() const {
	return mProperties;
}

void StyleSheetNode::setProperty( const StyleSheetProperty & property ) {
	mProperties[ property.getName() ] = property;
}

}}}
