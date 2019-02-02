#ifndef EE_UI_CSS_STYLESHEETSELECTOR_HPP
#define EE_UI_CSS_STYLESHEETSELECTOR_HPP

#include <eepp/core.hpp>
#include <eepp/ui/css/stylesheetselectorrule.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

class EE_API StyleSheetSelector {
	public:
		StyleSheetSelector();

		explicit StyleSheetSelector( const std::string& selectorName );

		const std::string& getName() const;

		const std::string& getPseudoClass() const;

		const Uint32& getSpecificity() const;

		bool select( StyleSheetElement * element, const bool& applyPseudo = true ) const;

		const bool& isCacheable() const;

		bool hasPseudoClass(const std::string& cls) const;

		bool hasPseudoClasses() const;
	protected:
		std::string mName;
		std::string mPseudoClass;
		Uint32 mSpecificity;
		std::vector<StyleSheetSelectorRule> mSelectorRules;
		bool mCacheable;

		void addSelectorRule(std::string& buffer, StyleSheetSelectorRule::PatternMatch& curPatternMatch, const StyleSheetSelectorRule::PatternMatch & newPatternMatch );

		void parseSelector( std::string selector );
};

}}}

#endif
