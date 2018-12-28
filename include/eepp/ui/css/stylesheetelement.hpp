#ifndef EE_UI_CSS_STYLESHEETELEMENT_HPP
#define EE_UI_CSS_STYLESHEETELEMENT_HPP

#include <string>
#include <vector>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement {
	public:
		virtual std::string getStyleSheetTag() const = 0;

		virtual const std::string& getStyleSheetId() const;

		virtual const std::vector<std::string>& getStyleSheetClasses() = 0;

		virtual StyleSheetElement * getStyleSheetParentElement() = 0;
};
}}}

#endif
