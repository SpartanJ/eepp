#ifndef EE_UI_CSS_STYLESHEETSELECTORPARSER_HPP
#define EE_UI_CSS_STYLESHEETSELECTORPARSER_HPP

#include <eepp/ui/css/stylesheetselector.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetSelectorParser {
	public:
		StyleSheetSelectorParser();

		explicit StyleSheetSelectorParser( std::string name );

		std::vector<StyleSheetSelector> selectors;
};

}}}
#endif
