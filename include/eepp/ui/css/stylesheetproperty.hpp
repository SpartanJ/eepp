#ifndef EE_UI_CSS_STYLESHEETPROPERTY_HPP
#define EE_UI_CSS_STYLESHEETPROPERTY_HPP

#include <eepp/config.hpp>
#include <eepp/core/string.hpp>
#include <eepp/graphics/blendmode.hpp>
#include <eepp/math/ease.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/system/color.hpp>
#include <eepp/system/time.hpp>
#include <eepp/ui/css/propertyids.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <span>
#include <string>

using namespace EE::System;
using namespace EE::Math;
using namespace EE::Graphics;

namespace EE { namespace UI {
class UINode;
class UIStyle;
}} // namespace EE::UI

namespace EE { namespace UI { namespace CSS {

class PropertyDefinition;
class ShorthandDefinition;

struct VariableFunctionCache {
	std::string definition;
	std::vector<std::string> variableList;

	void clear() { variableList.clear(); }

	void addVariable( std::string_view variable ) { variableList.emplace_back( variable ); }
};

class EE_API StyleSheetProperty {
	friend class EE::UI::UIStyle;

  public:
	StyleSheetProperty();

	explicit StyleSheetProperty( const PropertyDefinition* definition, const std::string& value,
								 const Uint32& index = 0, bool trimValue = true,
								 bool cachedProperty = false );

	explicit StyleSheetProperty( const std::string& name, const std::string& value,
								 bool trimValue = true, const Int64& specificity = 0,
								 const Uint32& index = 0 );

	explicit StyleSheetProperty( const std::string& name, const std::string& value,
								 const Int64& specificity, bool isVolatile = false,
								 const Uint32& index = 0 );

	/**
	 * @brief Returns the stable name hash used as the key in StyleSheetProperties maps.
	 *
	 * This is not a dense PropertyId or ShorthandId. Unknown and data-* properties
	 * also return their own name hash here.
	 */
	Uint32 getId() const;

	/** @brief Returns the dense longhand identity, or PropertyId::Invalid. */
	PropertyId getPropertyId() const;

	/** @brief Returns the dense shorthand identity, or ShorthandId::Invalid. */
	ShorthandId getShorthandId() const;

	const std::string& getName() const;

	/** @brief Returns the stable hash of getName(); this is not a dense ID. */
	const String::HashType& getNameHash() const;

	const std::string& getValue() const;

	const std::string& value() const;

	const Int64& getSpecificity() const;

	void setSpecificity( const Int64& specificity );

	bool isEmpty() const;

	void setName( const std::string& name );

	void setValue( const std::string& value, bool updateHash = false );

	bool isVolatile() const;

	void setVolatile( bool isVolatile );

	bool operator==( const StyleSheetProperty& property ) const;

	bool operator!=( const StyleSheetProperty& property ) const;

	std::string asString( const std::string& defaultValue = "" ) const;

	template <typename Type> Type asType( Type defaultValue ) const {
		Type val = defaultValue;
		return String::fromString( val, mValue ) ? val : defaultValue;
	}

	int asInt( int defaultValue = 0 ) const;

	unsigned int asUint( unsigned int defaultValue = 0 ) const;

	double asDouble( double defaultValue = 0 ) const;

	float asFloat( float defaultValue = 0 ) const;

	long long asLlong( long long defaultValue = 0 ) const;

	unsigned long long asUllong( unsigned long long defaultValue = 0 ) const;

	bool asBool( bool defaultValue = false ) const;

	Color asColor() const;

	Float asDpDimension( const std::string& defaultValue = "" ) const;

	int asDpDimensionI( const std::string& defaultValue = "" ) const;

	Uint32 asDpDimensionUint( const std::string& defaultValue = "" ) const;

	OriginPoint asOriginPoint() const;

	BlendMode asBlendMode() const;

	Vector2f asDpDimensionVector2f( const Vector2f& defaultValue = Vector2f::Zero ) const;

	Vector2i asDpDimensionVector2i( const Vector2i& defaultValue = Vector2i::Zero ) const;

	Vector2f asDpDimensionSizef( const Sizef& defaultValue = Sizef::Zero ) const;

	Vector2i asDpDimensionSizei( const Sizei& defaultValue = Sizei::Zero ) const;

	Vector2f asVector2f( const Vector2f& defaultValue = Vector2f::Zero ) const;

	Vector2i asVector2i( const Vector2i& defaultValue = Vector2i::Zero ) const;

	Sizef asSizef( const Sizef& defaultValue = Sizef::Zero ) const;

	Sizei asSizei( const Sizei& defaultValue = Sizei::Zero ) const;

	Rect asRect( const Rect& defaultValue = Rect() ) const;

	Rectf asRectf( const Rectf& defaultValue = Rectf::Zero ) const;

	Uint32 asTextDecoration() const;

	Uint32 asFontStyle() const;

	Time asTime( const Time& defaultTime = Seconds( 0 ) );

	Ease::Interpolation
	asInterpolation( const Ease::Interpolation& defaultInterpolation = Ease::Linear );

	const PropertyDefinition* getPropertyDefinition() const;

	const ShorthandDefinition* getShorthandDefinition() const;

	bool isVarValue() const;

	bool isLightDarkValue() const;

	bool needsValueSubstitution() const;

	size_t getPropertyIndexCount() const;

	const StyleSheetProperty& getPropertyIndex( const Uint32& index ) const;

	StyleSheetProperty* getPropertyIndexRef( const Uint32& index );

	const Uint32& getIndex() const;

	Float asDpDimension( UINode* node, const std::string& defaultValue = "" ) const;

	int asDpDimensionI( UINode* node, const std::string& defaultValue = "" ) const;

	Uint32 asDpDimensionUint( UINode* node, const std::string& defaultValue = "" ) const;

	Vector2f asDpDimensionVector2f( UINode* node,
									const Vector2f& defaultValue = Vector2f::Zero ) const;

	Vector2i asDpDimensionVector2i( UINode* node,
									const Vector2i& defaultValue = Vector2i::Zero ) const;

	Vector2f asDpDimensionSizef( UINode* node, const Sizef& defaultValue = Sizef::Zero ) const;

	Vector2i asDpDimensionSizei( UINode* node, const Sizei& defaultValue = Sizei::Zero ) const;

	Vector2f asVector2f( UINode* node, const Vector2f& defaultValue = Vector2f::Zero ) const;

	Vector2i asVector2i( UINode* node, const Vector2i& defaultValue = Vector2i::Zero ) const;

	Sizef asSizef( UINode* node, const Sizef& defaultValue = Sizef::Zero ) const;

	Sizei asSizei( UINode* node, const Sizei& defaultValue = Sizei::Zero ) const;

	StyleSheetLength asStyleSheetLength() const;

	const String::HashType& getValueHash() const;

	std::span<const VariableFunctionCache> getVarCache() const;

	StyleSheetProperty& setCachedProperty( bool cached );

	bool isCachedProperty() const;

	void setImportant( bool important );

  protected:
	bool hasSameResolutionSource( const StyleSheetProperty& property ) const;

	std::string mName;
	String::HashType mNameHash{ 0 };
	std::string mValue;
	String::HashType mValueHash{ 0 };
	Int64 mSpecificity{ 0 };
	Uint32 mIndex{ 0 };
	bool mVolatile : 1 { false };
	bool mImportant : 1 { false };
	bool mIsVarValue : 1 { false };
	bool mIsLightDarkValue : 1 { false };
	bool mCachedProperty : 1 { false };
	const PropertyDefinition* mPropertyDefinition{ nullptr };
	const ShorthandDefinition* mShorthandDefinition{ nullptr };
	std::vector<StyleSheetProperty> mIndexedProperty;
	std::vector<VariableFunctionCache> mVarCache;
	size_t mVarCacheSize{ 0 };

	explicit StyleSheetProperty( bool isVolatile, const PropertyDefinition* definition,
								 const std::string& value, const Int64& specificity = 0,
								 const Uint32& index = 0 );

	void cleanValue();
	void checkImportant();
	void createIndexed();
	void checkVars();
	std::string& mutableValue() { return mValue; }
};

typedef UnorderedMap<Uint32, StyleSheetProperty> StyleSheetProperties;

}}} // namespace EE::UI::CSS

#endif
