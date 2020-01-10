#ifndef EE_UI_CSS_STYLESHEETELEMENT_HPP
#define EE_UI_CSS_STYLESHEETELEMENT_HPP

#include <eepp/config.hpp>
#include <string>
#include <vector>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetElement {
  public:
	virtual const std::string& getStyleSheetTag() const = 0;

	virtual const std::string& getStyleSheetId() const = 0;

	virtual const std::vector<std::string>& getStyleSheetClasses() const = 0;

	virtual StyleSheetElement* getStyleSheetParentElement() const = 0;

	virtual StyleSheetElement* getStyleSheetPreviousSiblingElement() const = 0;

	virtual StyleSheetElement* getStyleSheetNextSiblingElement() const = 0;

	virtual const std::vector<std::string>& getStyleSheetPseudoClasses() const = 0;
};
}}} // namespace EE::UI::CSS

#endif
