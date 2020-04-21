#ifndef EE_UI_CSS_STYLESHEETSELECTOR_HPP
#define EE_UI_CSS_STYLESHEETSELECTOR_HPP

#include <eepp/ui/css/stylesheetselectorrule.hpp>

namespace EE { namespace UI {
class UIWidget;
}} // namespace EE::UI

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheetSelector {
  public:
	StyleSheetSelector();

	explicit StyleSheetSelector( const std::string& selectorName );

	const std::string& getName() const;

	const std::string& getPseudoClass() const;

	const Uint32& getSpecificity() const;

	bool select( UIWidget* element, const bool& applyPseudo = true ) const;

	const bool& isCacheable() const;

	bool hasPseudoClass( const std::string& cls ) const;

	bool hasPseudoClasses() const;

	std::vector<UIWidget*> getRelatedElements( UIWidget* element,
											   const bool& applyPseudo = true ) const;

	const bool& isStructurallyVolatile() const;

  protected:
	std::string mName;
	std::string mPseudoClass;
	Uint32 mSpecificity;
	std::vector<StyleSheetSelectorRule> mSelectorRules;
	bool mCacheable;
	bool mStructurallyVolatile;

	void addSelectorRule( std::string& buffer,
						  StyleSheetSelectorRule::PatternMatch& curPatternMatch,
						  const StyleSheetSelectorRule::PatternMatch& newPatternMatch );

	void parseSelector( std::string selector );
};

}}} // namespace EE::UI::CSS

#endif
