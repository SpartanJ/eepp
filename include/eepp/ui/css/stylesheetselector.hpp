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

	inline const std::string& getName() const { return mName; }

	inline const Int64& getSpecificity() const { return mSpecificity; }

	void setSpecificity( const Int64& specificity );

	bool select( UIWidget* element, const bool& applyPseudo = true ) const;

	bool isCacheable() const;

	bool hasPseudoClasses() const;

	/** @return The elements other than the subject whose pseudo-class state can change whether
	 * this selector matches element: the union of the tracked compounds over every matching
	 * path, each element once. Empty when the selector does not match. */
	SmallVector<UIWidget*, 8> getRelatedElements( UIWidget* element,
												  bool applyPseudo = true ) const;

	bool isStructurallyVolatile() const;

	const StyleSheetSelectorRule& getRule( const Uint32& index ) const;

	const std::string& getSelectorId() const;

	const std::string& getSelectorTagName() const;

  protected:
	std::string mName;
	Int64 mSpecificity;
	std::vector<StyleSheetSelectorRule> mSelectorRules;
	bool mCacheable{ true };
	bool mStructurallyVolatile{ false };
	bool mIsSingleRule{ false };

	void addSelectorRule( std::string& buffer,
						  StyleSheetSelectorRule::PatternMatch& curPatternMatch,
						  const StyleSheetSelectorRule::PatternMatch& newPatternMatch );

	void parseSelector( std::string selector );

	bool selectComplex( UIWidget* element, const bool& applyPseudo ) const;
};

}}} // namespace EE::UI::CSS

#endif
