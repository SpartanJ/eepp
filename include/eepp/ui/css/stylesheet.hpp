#ifndef EE_UI_CSS_STYLESHEET_HPP
#define EE_UI_CSS_STYLESHEET_HPP

#include <eepp/ui/css/elementdefinition.hpp>
#include <eepp/ui/css/keyframesdefinition.hpp>
#include <eepp/ui/css/mediaquery.hpp>
#include <eepp/ui/css/stylesheetstyle.hpp>
#include <memory>

namespace EE { namespace UI { namespace CSS {

class EE_API StyleSheet {
  public:
	StyleSheet();

	void clear();

	void addStyle( std::shared_ptr<StyleSheetStyle> node );

	bool isEmpty() const;

	std::string print();

	void combineStyleSheet( const StyleSheet& styleSheet );

	std::shared_ptr<ElementDefinition> getElementStyles( UIWidget* element,
														 const bool& applyPseudo = false ) const;

	const std::vector<std::shared_ptr<StyleSheetStyle>>& getStyles() const;

	std::vector<std::shared_ptr<StyleSheetStyle>>
	getStylesFromSelector( const std::string& selector ) const;

	std::shared_ptr<StyleSheetStyle> getStyleFromSelector( const std::string& selector,
														   bool searchBySpecificity = false ) const;

	bool updateMediaLists( const MediaFeatures& features );

	bool isMediaQueryListEmpty() const;

	StyleSheetStyleVector getStyleSheetStyleByAtRule( const AtRuleType& atRuleType ) const;

	bool isKeyframesDefined( const std::string& keyframesName ) const;

	const KeyframesDefinition& getKeyframesDefinition( const std::string& keyframesName ) const;

	void addKeyframes( const KeyframesDefinition& keyframes );

	void addKeyframes( const KeyframesDefinitionMap& keyframesMap );

	const KeyframesDefinitionMap& getKeyframes() const;

	void invalidateCache();

	const Uint32& getMarker() const;

	void setMarker( const Uint32& marker );

	void removeAllWithMarker( const Uint32& marker );

	bool markerExists( const Uint32& marker ) const;

	StyleSheet getAllWithMarker( const Uint32& marker ) const;

	std::vector<std::shared_ptr<StyleSheetStyle>>
	findStyleFromSelectorName( const std::string& selector ) const;

	bool refreshCacheFromStyles( const std::vector<std::shared_ptr<StyleSheetStyle>>& styles );

	const Uint64& getVersion() const;

	StyleSheet& operator=( const StyleSheet& other );

  protected:
	Uint64 mVersion{ 1 };
	Uint32 mMarker{ 0 };
	std::vector<std::shared_ptr<StyleSheetStyle>> mNodes;
	UnorderedMap<size_t, StyleSheetStyleVector> mNodeIndex;
	MediaQueryList::vector mMediaQueryList;
	KeyframesDefinitionMap mKeyframesMap;
	using ElementDefinitionCache = UnorderedMap<size_t, std::shared_ptr<ElementDefinition>>;
	mutable ElementDefinitionCache mNodeCache;

	static size_t nodeHash( const std::string& tag, const std::string& id );

	void addMediaQueryList( MediaQueryList::ptr list );

	bool addStyleToNodeIndex( StyleSheetStyle* style );
};

}}} // namespace EE::UI::CSS

#endif
