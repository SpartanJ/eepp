#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/stylesheetnode.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheet {
	public:
		StyleSheet();

		void addNode( StyleSheetNode node );

		StyleSheetProperties find( const std::string& tagName = "", const std::string& id = "", const std::vector<std::string>& classes = {}, const std::string& pseudoClass = "" );

		std::vector<StyleSheetNode> nodes;
};

}}}

#endif
