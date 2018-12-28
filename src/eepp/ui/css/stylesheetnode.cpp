#include <eepp/ui/css/stylesheetnode.hpp>
#include <iostream>

namespace EE { namespace UI { namespace CSS {

StyleSheetNode::StyleSheetNode( const std::string& selector, const StyleSheetProperties& properties ) :
	selector( selector ),
	properties( properties )
{}

void StyleSheetNode::print() {
	std::cout << selector.getName() << " {" << std::endl;

	for ( StyleSheetProperties::iterator it = properties.begin(); it != properties.end(); ++it ) {
		StyleSheetProperty& prop = it->second;

		std::cout << "\t" << prop.name << ": " << prop.value << ";" << std::endl;
	}

	std::cout << "}" << std::endl;
}

}}}
