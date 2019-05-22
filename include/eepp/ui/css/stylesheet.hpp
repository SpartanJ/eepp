#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/stylesheetstyle.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

class EE_API StyleSheet {
	public:
		typedef std::map<std::string,StyleSheetProperties> StyleSheetPseudoClassProperties;

		StyleSheet();

		void addStyle( const StyleSheetStyle& node );

		void combineStyle( const StyleSheetStyle& node );

		bool isEmpty() const;

		void print();

		void combineStyleSheet( const StyleSheet& styleSheet );

		StyleSheetStyleVector getElementStyles( StyleSheetElement * element, const bool& applyPseudo = false );

		const StyleSheetStyleList& getStyles() const;
	protected:
		StyleSheetStyleList mNodes;
};

}}}

#endif
