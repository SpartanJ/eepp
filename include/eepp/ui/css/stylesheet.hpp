#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>

namespace EE { namespace UI { namespace CSS {

class StyleSheetElement;

class EE_API StyleSheet {
  public:
	StyleSheet();

	void addStyle( const StyleSheetStyle& node );

	bool isEmpty() const;

	void print();

	void combineStyleSheet( const StyleSheet& styleSheet );

	StyleSheetStyleVector getElementStyles( StyleSheetElement* element,
											const bool& applyPseudo = false );

	const StyleSheetStyleVector& getStyles() const;

	bool updateMediaLists( const MediaFeatures& features );

	bool isMediaQueryListEmpty();

  protected:
	StyleSheetStyleVector mNodes;
	MediaQueryList::vector mMediaQueryList;

	void addMediaQueryList( MediaQueryList::ptr list );
};

}}} // namespace EE::UI::CSS

#endif
