#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/keyframesdefinition.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <memory>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheet {
  public:
	StyleSheet();

	void addStyle( std::shared_ptr<StyleSheetStyle> node );

	bool isEmpty() const;

	void print();

	void combineStyleSheet( const StyleSheet& styleSheet );

	StyleSheetStyleVector getElementStyles( UIWidget* element,
											const bool& applyPseudo = false ) const;

	const StyleSheetStyleVector& getStyles() const;

	bool updateMediaLists( const MediaFeatures& features );

	bool isMediaQueryListEmpty() const;

	StyleSheetStyleVector getStyleSheetStyleByAtRule( const AtRuleType& atRuleType ) const;

	bool isKeyframesDefined( const std::string& keyframesName );

	const KeyframesDefinition& getKeyframesDefinition( const std::string& keyframesName );

	void addKeyframes( const KeyframesDefinition& keyframes );

	void addKeyframes( const KeyframesDefinitionMap& keyframesMap );

	const KeyframesDefinitionMap& getKeyframes() const;

  protected:
	StyleSheetStyleVector mNodes;
	MediaQueryList::vector mMediaQueryList;
	KeyframesDefinitionMap mKeyframesMap;

	void addMediaQueryList( MediaQueryList::ptr list );
};

}}} // namespace EE::UI::CSS

#endif
