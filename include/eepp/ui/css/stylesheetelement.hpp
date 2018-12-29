#ifndef EE_UI_CSS_STYLESHEETELEMENT_HPP
#define EE_UI_CSS_STYLESHEETELEMENT_HPP

#include <string>
#include <vector>
#include <eepp/config.hpp>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetElement {
	public:
		virtual const std::string& getStyleSheetTag() const = 0;

		virtual const std::string& getStyleSheetId() const = 0;

		virtual const std::vector<std::string>& getStyleSheetClasses() const = 0;

		virtual StyleSheetElement * getStyleSheetParentElement() = 0;
};
}}}

#endif
