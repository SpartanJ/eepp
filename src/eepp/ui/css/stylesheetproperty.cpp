#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselectorrule.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uihelper.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetProperty::StyleSheetProperty() :
	mSpecificity( 0 ), mVolatile( false ), mImportant( false ) {}

StyleSheetProperty::StyleSheetProperty( const PropertyDefinition* definition,
										const std::string& value, const Uint32& index ) :
	mName( definition->getName() ),
	mNameHash( definition->getId() ),
	mValue( String::trim( value ) ),
	mSpecificity( 0 ),
	mIndex( index ),
	mVolatile( false ),
	mImportant( false ),
	mIsVarValue( String::startsWith( mValue, "var(" ) ),
	mPropertyDefinition( definition ),
	mShorthandDefinition( NULL ) {
	cleanValue();
	checkImportant();
	createIndexed();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		eePRINTL( "Property %s is not defined!", mName.c_str() );
	}
}

StyleSheetProperty::StyleSheetProperty( const bool& isVolatile,
										const PropertyDefinition* definition,
										const std::string& value, const Uint32& specificity,
										const Uint32& index ) :
	mName( definition->getName() ),
	mNameHash( definition->getId() ),
	mValue( String::trim( value ) ),
	mSpecificity( 0 ),
	mIndex( index ),
	mVolatile( false ),
	mImportant( false ),
	mIsVarValue( String::startsWith( mValue, "var(" ) ),
	mPropertyDefinition( definition ),
	mShorthandDefinition( NULL ) {
	cleanValue();
	checkImportant();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		eePRINTL( "Property %s is not defined!", mName.c_str() );
	}
}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value ) :
	mName( String::toLower( String::trim( name ) ) ),
	mNameHash( String::hash( mName ) ),
	mValue( String::trim( value ) ),
	mSpecificity( 0 ),
	mIndex( 0 ),
	mVolatile( false ),
	mImportant( false ),
	mIsVarValue( String::startsWith( mValue, "var(" ) ),
	mPropertyDefinition( StyleSheetSpecification::instance()->getProperty( mNameHash ) ),
	mShorthandDefinition( NULL == mPropertyDefinition
							  ? StyleSheetSpecification::instance()->getShorthand( mNameHash )
							  : NULL ) {
	cleanValue();
	checkImportant();
	createIndexed();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		eePRINTL( "Property %s is not defined!", mName.c_str() );
	}
}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value,
										const Uint32& specificity, const bool& isVolatile,
										const Uint32& index ) :
	mName( String::toLower( String::trim( name ) ) ),
	mNameHash( String::hash( mName ) ),
	mValue( String::trim( value ) ),
	mSpecificity( specificity ),
	mIndex( index ),
	mVolatile( isVolatile ),
	mImportant( false ),
	mIsVarValue( String::startsWith( mValue, "var(" ) ),
	mPropertyDefinition( StyleSheetSpecification::instance()->getProperty( mNameHash ) ),
	mShorthandDefinition( NULL == mPropertyDefinition
							  ? StyleSheetSpecification::instance()->getShorthand( mNameHash )
							  : NULL ) {
	cleanValue();
	checkImportant();
	createIndexed();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		eePRINTL( "Property %s is not defined!" );
	}
}

Uint32 StyleSheetProperty::getId() const {
	return NULL != mPropertyDefinition
			   ? mPropertyDefinition->getId()
			   : ( NULL != mShorthandDefinition ? mShorthandDefinition->getId() : 0 );
}

const std::string& StyleSheetProperty::getName() const {
	return mName;
}

const std::string& StyleSheetProperty::getValue() const {
	return mValue;
}

const std::string& StyleSheetProperty::value() const {
	return mValue;
}

const Uint32& StyleSheetProperty::getSpecificity() const {
	return mSpecificity;
}

void StyleSheetProperty::setSpecificity( const Uint32& specificity ) {
	// Don't allow set specificity if is an important property,
	// force the specificity in this case.
	if ( !mImportant ) {
		mSpecificity = specificity;
	}
}

bool StyleSheetProperty::isEmpty() const {
	return mName.empty();
}

void StyleSheetProperty::setName( const std::string& name ) {
	mName = name;
	mNameHash = String::hash( mName );
}

void StyleSheetProperty::setValue( const std::string& value ) {
	mValue = value;
	mIsVarValue = String::startsWith( mValue, "var(" );
	createIndexed();
}

const bool& StyleSheetProperty::isVolatile() const {
	return mVolatile;
}

void StyleSheetProperty::setVolatile( const bool& isVolatile ) {
	mVolatile = isVolatile;
}

bool StyleSheetProperty::operator==( const StyleSheetProperty& property ) const {
	return mNameHash == property.mNameHash && mValue == property.mValue;
}

const Uint32& StyleSheetProperty::getNameHash() const {
	return mNameHash;
}

void StyleSheetProperty::checkImportant() {
	if ( String::endsWith( mValue, "!important" ) ) {
		mImportant = true;
		mSpecificity = StyleSheetSelectorRule::SpecificityImportant;
		mValue = String::trim( mValue.substr( 0, mValue.size() - 10 /*!important*/ ) );
	}
}

void StyleSheetProperty::createIndexed() {
	if ( NULL != mPropertyDefinition && mPropertyDefinition->isIndexed() ) {
		mIndexedProperty.clear();
		auto splitValues = String::split( getValue(), ",", "", "(\"" );
		if ( !splitValues.empty() ) {
			for ( size_t i = 0; i < splitValues.size(); i++ ) {
				mIndexedProperty.emplace_back( StyleSheetProperty(
					isVolatile(), getPropertyDefinition(), splitValues[i], getSpecificity(), i ) );
			}
		}
	}
}

std::string StyleSheetProperty::asString( const std::string& defaultValue ) const {
	return mValue.empty() ? defaultValue : mValue;
}

int StyleSheetProperty::asInt( int defaultValue ) const {
	return asType<int>( defaultValue );
}

unsigned int StyleSheetProperty::asUint( unsigned int defaultValue ) const {
	return asType<unsigned int>( defaultValue );
}

double StyleSheetProperty::asDouble( double defaultValue ) const {
	return asType<double>( defaultValue );
}

float StyleSheetProperty::asFloat( float defaultValue ) const {
	return asType<float>( defaultValue );
}

long long StyleSheetProperty::asLlong( long long defaultValue ) const {
	return asType<long long>( defaultValue );
}

unsigned long long StyleSheetProperty::asUllong( unsigned long long defaultValue ) const {
	return asType<unsigned long long>( defaultValue );
}

bool StyleSheetProperty::asBool( bool defaultValue ) const {
	if ( mValue.empty() )
		return defaultValue;

	// only look at first char
	char first = mValue[0];

	// 1*, t* (true), T* (True), y* (yes), Y* (YES)
	return ( first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y' );
}

Color StyleSheetProperty::asColor() const {
	return Color::fromString( mValue );
}

Float StyleSheetProperty::asDpDimension( const std::string& defaultValue ) const {
	return PixelDensity::toDpFromString( asString( defaultValue ) );
}

int StyleSheetProperty::asDpDimensionI( const std::string& defaultValue ) const {
	return PixelDensity::toDpFromStringI( asString( defaultValue ) );
}

Uint32 StyleSheetProperty::asDpDimensionUint( const std::string& defaultValue ) const {
	int attrInt = asDpDimensionI( defaultValue );

	return attrInt >= 0 ? attrInt : 0;
}

static OriginPoint toOriginPoint( std::string val ) {
	String::toLowerInPlace( val );

	if ( "center" == val ) {
		return OriginPoint::OriginCenter;
	} else if ( "topleft" == val ) {
		return OriginPoint::OriginTopLeft;
	} else {
		std::vector<std::string> parts = String::split( val, ' ' );

		if ( parts.size() == 2 ) {
			Float x = 0;
			Float y = 0;

			bool Res1 = String::fromString<Float>( x, String::trim( parts[0] ) );
			bool Res2 = String::fromString<Float>( y, String::trim( parts[1] ) );

			if ( Res1 && Res2 ) {
				return OriginPoint( x, y );
			}
		}
	}

	return OriginPoint::OriginCenter;
}

static BlendMode toBlendMode( std::string val ) {
	String::toLowerInPlace( val );

	BlendMode blendMode;

	if ( val == "add" )
		blendMode = BlendAdd;
	else if ( val == "alpha" )
		blendMode = BlendAlpha;
	else if ( val == "multiply" )
		blendMode = BlendMultiply;
	else if ( val == "none" )
		blendMode = BlendNone;

	return blendMode;
}

OriginPoint StyleSheetProperty::asOriginPoint() const {
	return toOriginPoint( mValue );
}

BlendMode StyleSheetProperty::asBlendMode() const {
	return toBlendMode( mValue );
}

Vector2f StyleSheetProperty::asDpDimensionVector2f( const Vector2f& defaultValue ) const {
	if ( !mValue.empty() ) {
		Vector2f vector;
		auto xySplit = String::split( mValue, ' ', true );

		if ( xySplit.size() == 2 ) {
			vector.x = PixelDensity::toDpFromString( "0" );
			vector.y = PixelDensity::toDpFromString( "0" );
			return vector;
		} else if ( xySplit.size() == 1 ) {
			vector.x = vector.y = PixelDensity::toDpFromString( "0" );
			return vector;
		}
	}

	return defaultValue;
}

Vector2i StyleSheetProperty::asDpDimensionVector2i( const Vector2i& defaultValue ) const {
	Vector2f v( asDpDimensionVector2f( Vector2f( defaultValue.x, defaultValue.y ) ) );
	return Vector2i( v.x, v.y );
}

Vector2f StyleSheetProperty::asDpDimensionSizef( const Sizef& defaultValue ) const {
	Vector2f v( asDpDimensionVector2f( defaultValue ) );
	return Sizef( v.x, v.y );
}

Vector2i StyleSheetProperty::asDpDimensionSizei( const Sizei& defaultValue ) const {
	Vector2i v( asDpDimensionVector2i( defaultValue ) );
	return Sizei( v.x, v.y );
}

Vector2f StyleSheetProperty::asVector2f( const Vector2f& defaultValue ) const {
	if ( !mValue.empty() ) {
		Vector2f vector;
		auto xySplit = String::split( mValue, ' ', true );

		if ( xySplit.size() == 2 ) {
			Float val;

			vector.x =
				String::fromString<Float>( val, String::trim( xySplit[0] ) ) ? val : defaultValue.x;
			vector.y =
				String::fromString<Float>( val, String::trim( xySplit[1] ) ) ? val : defaultValue.y;

			return vector;
		} else if ( xySplit.size() == 1 ) {
			Float val;

			vector.x = vector.y =
				String::fromString<Float>( val, xySplit[0] ) ? val : defaultValue.x;

			return vector;
		}
	}

	return defaultValue;
}

Vector2i StyleSheetProperty::asVector2i( const Vector2i& defaultValue ) const {
	if ( !mValue.empty() ) {
		Vector2i vector;
		auto xySplit = String::split( mValue, ' ', true );

		if ( xySplit.size() == 2 ) {
			int val;

			vector.x =
				String::fromString<int>( val, String::trim( xySplit[0] ) ) ? val : defaultValue.x;
			vector.y =
				String::fromString<int>( val, String::trim( xySplit[1] ) ) ? val : defaultValue.y;

			return vector;
		} else if ( xySplit.size() == 1 ) {
			int val;

			vector.x = vector.y = String::fromString<int>( val, xySplit[0] ) ? val : defaultValue.x;

			return vector;
		}
	}

	return defaultValue;
}

Sizef StyleSheetProperty::asSizef( const Sizef& defaultValue ) const {
	return Sizef( asVector2f( defaultValue ) );
}

Sizei StyleSheetProperty::asSizei( const Sizei& defaultValue ) const {
	return Sizei( asVector2i( defaultValue ) );
}

Rect StyleSheetProperty::asRect( const Rect& defaultValue ) const {
	if ( !mValue.empty() ) {
		Rect rect( defaultValue );

		auto ltrbSplit = String::split( mValue, ' ', true );

		if ( ltrbSplit.size() == 4 ) {
			rect.Left = PixelDensity::toDpFromStringI( ltrbSplit[0] );
			rect.Top = PixelDensity::toDpFromStringI( ltrbSplit[1] );
			rect.Right = PixelDensity::toDpFromStringI( ltrbSplit[2] );
			rect.Bottom = PixelDensity::toDpFromStringI( ltrbSplit[3] );
		} else if ( ltrbSplit.size() == 3 ) {
			rect.Left = PixelDensity::toDpFromStringI( ltrbSplit[0] );
			rect.Top = PixelDensity::toDpFromStringI( ltrbSplit[1] );
			rect.Right = PixelDensity::toDpFromStringI( ltrbSplit[2] );
		} else if ( ltrbSplit.size() == 2 ) {
			rect.Left = PixelDensity::toDpFromStringI( ltrbSplit[0] );
			rect.Top = PixelDensity::toDpFromStringI( ltrbSplit[1] );
		} else if ( ltrbSplit.size() == 1 ) {
			rect.Left = rect.Top = rect.Right = rect.Bottom =
				PixelDensity::toDpFromStringI( ltrbSplit[0] );
		}

		return rect;
	}

	return defaultValue;
}

Rectf StyleSheetProperty::asRectf( const Rectf& defaultValue ) const {
	Rectf rect( defaultValue );

	if ( !mValue.empty() ) {
		auto ltrbSplit = String::split( mValue, ' ', true );

		if ( ltrbSplit.size() == 4 ) {
			rect.Left = PixelDensity::toDpFromString( ltrbSplit[0] );
			rect.Top = PixelDensity::toDpFromString( ltrbSplit[1] );
			rect.Right = PixelDensity::toDpFromString( ltrbSplit[2] );
			rect.Bottom = PixelDensity::toDpFromString( ltrbSplit[3] );
		} else if ( ltrbSplit.size() == 3 ) {
			rect.Left = PixelDensity::toDpFromString( ltrbSplit[0] );
			rect.Top = PixelDensity::toDpFromString( ltrbSplit[1] );
			rect.Right = PixelDensity::toDpFromString( ltrbSplit[2] );
		} else if ( ltrbSplit.size() == 2 ) {
			rect.Left = PixelDensity::toDpFromString( ltrbSplit[0] );
			rect.Top = PixelDensity::toDpFromString( ltrbSplit[1] );
		} else if ( ltrbSplit.size() == 1 ) {
			rect.Left = rect.Top = rect.Right = rect.Bottom =
				PixelDensity::toDpFromString( mValue );
		}
	}

	return rect;
}

Uint32 StyleSheetProperty::asFontStyle() const {
	return Text::stringToStyleFlag( getValue() );
}

Time StyleSheetProperty::asTime( const Time& defaultTime ) {
	if ( !mValue.empty() ) {
		if ( mValue[mValue.size() - 1] == 'm' ) {
			return Milliseconds( asFloat() );
		} else {
			return Seconds( asFloat() );
		}
	}

	return defaultTime;
}

Ease::Interpolation
StyleSheetProperty::asInterpolation( const Ease::Interpolation& defaultInterpolation ) {
	return Ease::fromName( mValue, defaultInterpolation );
}

const PropertyDefinition* StyleSheetProperty::getPropertyDefinition() const {
	return mPropertyDefinition;
}

const ShorthandDefinition* StyleSheetProperty::getShorthandDefinition() const {
	return mShorthandDefinition;
}

const bool& StyleSheetProperty::isVarValue() const {
	return mIsVarValue;
}

size_t StyleSheetProperty::getPropertyIndexCount() const {
	return mIndexedProperty.size();
}

StyleSheetProperty& StyleSheetProperty::getPropertyIndex( const Uint32& index ) {
	eeASSERT( index < mIndexedProperty.size() );
	return mIndexedProperty[index];
}

const Uint32& StyleSheetProperty::getIndex() const {
	return mIndex;
}

void StyleSheetProperty::cleanValue() {
	if ( NULL != mPropertyDefinition && mPropertyDefinition->getType() == PropertyType::String ) {
		String::trimInPlace( mValue, '"' );
	}
}

}}} // namespace EE::UI::CSS
