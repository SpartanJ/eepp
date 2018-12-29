#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/stylesheetnode.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

class EE_API StyleSheet {
	public:
		StyleSheet();

		void addNode( StyleSheetNode node );

		bool isEmpty() const;

		StyleSheetProperties getElementProperties( StyleSheetElement * element, const std::string& pseudoClass = "" );

		std::vector<StyleSheetNode> nodes;
};

}}}

#endif
