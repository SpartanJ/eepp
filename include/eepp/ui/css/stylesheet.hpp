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
	using SourceOrder = Uint32;

	StyleSheet();

	void clear();

	void addStyle( std::shared_ptr<StyleSheetStyle> node );

	bool isEmpty() const;

	std::string print();

	void combineStyleSheet( const StyleSheet& styleSheet );

	void combineStyleSheet( const StyleSheet& styleSheet, SourceOrder sourceOrder );

	SourceOrder reserveSourceOrder();

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

	void removeAllWithoutMarker( const Uint32& marker );

	void setSelectorSpecificity( const Int64& specificity );

	bool markerExists( const Uint32& marker ) const;

	StyleSheet getAllWithMarker( const Uint32& marker ) const;

	StyleSheet getAllWithMarkers() const;

	std::vector<std::shared_ptr<StyleSheetStyle>>
	findStyleFromSelectorName( const std::string& selector ) const;

	bool refreshCacheFromStyles( const std::vector<std::shared_ptr<StyleSheetStyle>>& styles );

	const Uint64& getVersion() const;

	StyleSheet& operator=( const StyleSheet& other );

  protected:
	struct SourcePosition {
		SourceOrder styleSheet{ 0 };
		Uint32 rule{ 0 };

		bool operator<( const SourcePosition& other ) const {
			if ( styleSheet != other.styleSheet )
				return styleSheet < other.styleSheet;
			return rule < other.rule;
		}
	};
	static_assert( sizeof( SourcePosition ) == sizeof( Uint64 ) );

	Uint64 mVersion{ 1 };
	Uint32 mMarker{ 0 };
	std::vector<std::shared_ptr<StyleSheetStyle>> mNodes;
	UnorderedMap<size_t, StyleSheetStyleVector> mNodeIndex;
	// Class-anchored rules live in one bucket only; selector matching validates tag and other
	// classes.
	UnorderedMap<String::HashType, StyleSheetStyleVector> mClassNodeIndex;
	// Candidate buckets are independent, so retain insertion order explicitly for equal
	// specificity.
	UnorderedMap<const StyleSheetStyle*, SourcePosition> mStyleSourceOrder;
	SourceOrder mNextStyleSourceOrder{ 0 };
	MediaQueryList::vector mMediaQueryList;
	KeyframesDefinitionMap mKeyframesMap;
	using ElementDefinitionCache = UnorderedMap<size_t, std::shared_ptr<ElementDefinition>>;
	mutable ElementDefinitionCache mNodeCache;

	static size_t nodeHash( const std::string& tag, const std::string& id );

	void addMediaQueryList( MediaQueryList::ptr list );

	bool addStyleToNodeIndex( StyleSheetStyle* style );

	void addStyle( std::shared_ptr<StyleSheetStyle> node, SourceOrder sourceOrder,
				   Uint32 ruleOrder );
};

}}} // namespace EE::UI::CSS

#endif
