#ifndef EE_UI_CSS_STYLESHEETNODE_HPP
#define EE_UI_CSS_STYLESHEETNODE_HPP

#include <eepp/ui/css/stylesheetselector.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetNode {
	public:
		explicit StyleSheetNode( const std::string& selector, const StyleSheetProperties& properties );

		void print();

		StyleSheetSelector selector;
		StyleSheetProperties properties;
};

}}}

#endif
