#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/elementdefinition.hpp>
#include <eepp/ui/css/keyframesdefinition.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <memory>
#include <unordered_map>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheet {
  public:
	StyleSheet();

	void addStyle( std::shared_ptr<StyleSheetStyle> node );

	bool isEmpty() const;

	std::string print();

	void combineStyleSheet( const StyleSheet& styleSheet );

	std::shared_ptr<ElementDefinition> getElementStyles( UIWidget* element,
														 const bool& applyPseudo = false ) const;

	const std::vector<std::shared_ptr<StyleSheetStyle>>& getStyles() const;

	std::shared_ptr<StyleSheetStyle> getStyleFromSelector( const std::string& selector ) const;

	bool updateMediaLists( const MediaFeatures& features );

	bool isMediaQueryListEmpty() const;

	StyleSheetStyleVector getStyleSheetStyleByAtRule( const AtRuleType& atRuleType ) const;

	bool isKeyframesDefined( const std::string& keyframesName );

	const KeyframesDefinition& getKeyframesDefinition( const std::string& keyframesName );

	void addKeyframes( const KeyframesDefinition& keyframes );

	void addKeyframes( const KeyframesDefinitionMap& keyframesMap );

	const KeyframesDefinitionMap& getKeyframes() const;

	static size_t nodeHash( const std::string& tag, const std::string& id );

	void invalidateCache();

	const Uint32& getMarker() const;

	void setMarker( const Uint32& marker );

	void removeAllWithMarker( const Uint32& marker );

	bool markerExists( const Uint32& marker ) const;

	std::vector<std::shared_ptr<StyleSheetStyle>>
	findStyleFromSelectorName( const std::string& selector );

	bool refreshCacheFromStyles( const std::vector<std::shared_ptr<StyleSheetStyle>>& styles );

  protected:
	Uint32 mMarker{ 0 };
	std::vector<std::shared_ptr<StyleSheetStyle>> mNodes;
	std::unordered_map<size_t, StyleSheetStyleVector> mNodeIndex;
	MediaQueryList::vector mMediaQueryList;
	KeyframesDefinitionMap mKeyframesMap;
	using ElementDefinitionCache = std::unordered_map<size_t, std::shared_ptr<ElementDefinition>>;
	mutable ElementDefinitionCache mNodeCache;

	void addMediaQueryList( MediaQueryList::ptr list );

	bool addStyleToNodeIndex( StyleSheetStyle* style );
};

}}} // namespace EE::UI::CSS

#endif
