#include <eepp/core/string.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/system/log.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/css/shorthanddefinition.hpp>
#include <eepp/ui/css/stylesheetlength.hpp>
#include <eepp/ui/css/stylesheetproperty.hpp>
#include <eepp/ui/css/stylesheetselectorrule.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI { namespace CSS {

StyleSheetProperty::StyleSheetProperty() :
	mSpecificity( 0 ), mVolatile( false ), mImportant( false ) {}

StyleSheetProperty::StyleSheetProperty( const PropertyDefinition* definition,
										const std::string& value, const Uint32& index,
										bool trimValue ) :
	mName( definition->getName() ),
	mNameHash( definition->getId() ),
	mValue( trimValue ? String::trim( value ) : value ),
	mValueHash( String::hash( mValue ) ),
	mSpecificity( 0 ),
	mIndex( index ),
	mVolatile( false ),
	mImportant( false ),
	mIsVarValue( false ),
	mPropertyDefinition( definition ),
	mShorthandDefinition( NULL ) {
	if ( trimValue )
		cleanValue();
	checkImportant();
	createIndexed();
	checkVars();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		Log::warning( "Property \"%s\" is not defined!", mName );
	}
}

StyleSheetProperty::StyleSheetProperty( const bool& isVolatile,
										const PropertyDefinition* definition,
										const std::string& value, const Uint32& /*specificity*/,
										const Uint32& index ) :
	mName( definition->getName() ),
	mNameHash( definition->getId() ),
	mValue( String::trim( value ) ),
	mValueHash( String::hash( mValue ) ),
	mSpecificity( 0 ),
	mIndex( index ),
	mVolatile( isVolatile ),
	mImportant( false ),
	mIsVarValue( false ),
	mPropertyDefinition( definition ),
	mShorthandDefinition( NULL ) {
	cleanValue();
	checkImportant();
	checkVars();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		Log::warning( "Property \"%s\" is not defined!", mName );
	}
}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value,
										const bool& trimValue, const Uint32& specificity,
										const Uint32& index ) :
	mName( String::toLower( String::trim( name ) ) ),
	mNameHash( String::hash( mName ) ),
	mValue( trimValue ? String::trim( value ) : value ),
	mValueHash( String::hash( mValue ) ),
	mSpecificity( specificity ),
	mIndex( index ),
	mVolatile( false ),
	mImportant( false ),
	mIsVarValue( false ),
	mPropertyDefinition( StyleSheetSpecification::instance()->getProperty( mNameHash ) ),
	mShorthandDefinition( NULL == mPropertyDefinition
							  ? StyleSheetSpecification::instance()->getShorthand( mNameHash )
							  : NULL ) {
	cleanValue();
	checkImportant();
	createIndexed();
	checkVars();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		Log::warning( "Property \"%s\" is not defined!", mName );
	}
}

StyleSheetProperty::StyleSheetProperty( const std::string& name, const std::string& value,
										const Uint32& specificity, const bool& isVolatile,
										const Uint32& index ) :
	mName( String::toLower( String::trim( name ) ) ),
	mNameHash( String::hash( mName ) ),
	mValue( String::trim( value ) ),
	mValueHash( String::hash( mValue ) ),
	mSpecificity( specificity ),
	mIndex( index ),
	mVolatile( isVolatile ),
	mImportant( false ),
	mIsVarValue( false ),
	mPropertyDefinition( StyleSheetSpecification::instance()->getProperty( mNameHash ) ),
	mShorthandDefinition( NULL == mPropertyDefinition
							  ? StyleSheetSpecification::instance()->getShorthand( mNameHash )
							  : NULL ) {
	cleanValue();
	checkImportant();
	createIndexed();
	checkVars();

	if ( NULL == mShorthandDefinition && NULL == mPropertyDefinition ) {
		Log::warning( "Property \"%s\" is not defined!", mName );
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

void StyleSheetProperty::setValue( const std::string& value, bool updateHash ) {
	mValue = value;
	if ( updateHash )
		mValueHash = String::hash( value );
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
	return mNameHash == property.mNameHash && mValueHash == property.mValueHash;
}

bool StyleSheetProperty::operator!=( const StyleSheetProperty& property ) const {
	return mNameHash != property.mNameHash || mValueHash != property.mValueHash;
}

const String::HashType& StyleSheetProperty::getNameHash() const {
	return mNameHash;
}

void StyleSheetProperty::checkImportant() {
	if ( String::endsWith( mValue, "!important" ) ) {
		mImportant = true;
		mSpecificity = StyleSheetSelectorRule::SpecificityImportant;
		mValue = String::trim( mValue.substr( 0, mValue.size() - 10 /*!important*/ ) );
		mValueHash = String::hash( mValue );
	}
}

void StyleSheetProperty::createIndexed() {
	if ( NULL != mPropertyDefinition && mPropertyDefinition->isIndexed() ) {
		mIndexedProperty.clear();
		auto splitValues = String::split( getValue(), ",", "", "(\"" );
		if ( !splitValues.empty() ) {
			for ( size_t i = 0; i < splitValues.size(); i++ ) {
				StyleSheetProperty index( mVolatile, mPropertyDefinition, splitValues[i],
										  mSpecificity, i );
				mIndexedProperty.emplace_back( std::move( index ) );
			}
		}
	}
}

void StyleSheetProperty::checkVars() {
	auto varCache( checkVars( mValue ) );
	mIsVarValue = false;
	if ( !varCache.empty() ) {
		mIsVarValue = true;
		mVarCache = std::move( varCache );
	}
}

static void varToVal( VariableFunctionCache& varCache, const std::string& varDef ) {
	FunctionString functionType = FunctionString::parse( varDef );
	if ( !functionType.getParameters().empty() ) {
		for ( auto& val : functionType.getParameters() ) {
			if ( String::startsWith( val, "--" ) ) {
				varCache.variableList.emplace_back( val );
			} else if ( String::startsWith( val, "var(" ) ) {
				varToVal( varCache, val );
			}
		}
	}
}

std::vector<VariableFunctionCache> StyleSheetProperty::checkVars( const std::string& value ) {
	std::vector<VariableFunctionCache> vars;
	std::string::size_type tokenStart = 0;
	std::string::size_type tokenEnd = 0;

	while ( true ) {
		tokenStart = value.find( "var(", tokenStart );
		if ( tokenStart != std::string::npos ) {
			tokenEnd = String::findCloseBracket( value, tokenStart, '(', ')' );
			if ( tokenEnd != std::string::npos ) {
				mIsVarValue = true;
				VariableFunctionCache variableFuncCache;
				variableFuncCache.definition =
					value.substr( tokenStart, tokenEnd + 1 - tokenStart );
				varToVal( variableFuncCache, variableFuncCache.definition );
				tokenStart = tokenEnd;
				vars.emplace_back( std::move( variableFuncCache ) );
			} else {
				break;
			}
		} else {
			break;
		}
	};

	return vars;
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
		blendMode = BlendMode::Add();
	else if ( val == "alpha" )
		blendMode = BlendMode::Alpha();
	else if ( val == "multiply" )
		blendMode = BlendMode::Multiply();
	else if ( val == "none" )
		blendMode = BlendMode::None();

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
			vector.x = PixelDensity::toDpFromString( xySplit[0] );
			vector.y = PixelDensity::toDpFromString( xySplit[1] );
			return vector;
		} else if ( xySplit.size() == 1 ) {
			vector.x = vector.y = PixelDensity::toDpFromString( xySplit[0] );
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
		return Time::fromString( mValue );
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

const StyleSheetProperty& StyleSheetProperty::getPropertyIndex( const Uint32& index ) const {
	eeASSERT( index < mIndexedProperty.size() );
	return mIndexedProperty[index];
}

StyleSheetProperty* StyleSheetProperty::getPropertyIndexRef( const Uint32& index ) {
	eeASSERT( index < mIndexedProperty.size() );
	return &mIndexedProperty[index];
}

const Uint32& StyleSheetProperty::getIndex() const {
	return mIndex;
}

void StyleSheetProperty::cleanValue() {
	if ( NULL != mPropertyDefinition && mPropertyDefinition->getType() == PropertyType::String ) {
		String::trimInPlace( mValue, '"' );
	}
}

Float StyleSheetProperty::asDpDimension( UINode* node, const std::string& defaultValue ) const {
	return node->lengthFromValueAsDp( asString( defaultValue ), CSS::PropertyRelativeTarget::None );
}

int StyleSheetProperty::asDpDimensionI( UINode* node, const std::string& defaultValue ) const {
	return static_cast<int>( asDpDimension( node, defaultValue ) );
}

Uint32 StyleSheetProperty::asDpDimensionUint( UINode* node,
											  const std::string& defaultValue ) const {
	int attrInt = asDpDimensionI( node, defaultValue );
	return attrInt >= 0 ? attrInt : 0;
}

Vector2f StyleSheetProperty::asDpDimensionVector2f( UINode* node,
													const Vector2f& defaultValue ) const {
	if ( !mValue.empty() ) {
		Vector2f vector;
		auto xySplit = String::split( mValue, ' ', true );

		if ( xySplit.size() == 2 ) {
			vector.x = node->lengthFromValueAsDp( xySplit[0], CSS::PropertyRelativeTarget::None,
												  defaultValue.x );
			vector.y = node->lengthFromValueAsDp( xySplit[1], CSS::PropertyRelativeTarget::None,
												  defaultValue.y );
			return vector;
		} else if ( xySplit.size() == 1 ) {
			vector.x = vector.y = node->lengthFromValueAsDp(
				xySplit[0], CSS::PropertyRelativeTarget::None, defaultValue.x );
			return vector;
		}
	}

	return defaultValue;
}

Vector2i StyleSheetProperty::asDpDimensionVector2i( UINode* node,
													const Vector2i& defaultValue ) const {
	Vector2f v( asDpDimensionVector2f( node, Vector2f( defaultValue.x, defaultValue.y ) ) );
	return Vector2i( v.x, v.y );
}

Vector2f StyleSheetProperty::asDpDimensionSizef( UINode* node, const Sizef& defaultValue ) const {
	Vector2f v( asDpDimensionVector2f( node, defaultValue ) );
	return Sizef( v.x, v.y );
}

Vector2i StyleSheetProperty::asDpDimensionSizei( UINode* node, const Sizei& defaultValue ) const {
	Vector2i v( asDpDimensionVector2i( node, defaultValue ) );
	return Sizei( v.x, v.y );
}

Vector2f StyleSheetProperty::asVector2f( UINode* node, const Vector2f& defaultValue ) const {
	if ( !mValue.empty() ) {
		Vector2f vector;
		auto xySplit = String::split( mValue, ' ', true );

		if ( xySplit.size() == 2 ) {
			vector.x = node->lengthFromValue( xySplit[0], CSS::PropertyRelativeTarget::None,
											  defaultValue.x );
			vector.y = node->lengthFromValue( xySplit[1], CSS::PropertyRelativeTarget::None,
											  defaultValue.y );
			return vector;
		} else if ( xySplit.size() == 1 ) {
			vector.x = vector.y = node->lengthFromValue(
				xySplit[0], CSS::PropertyRelativeTarget::None, defaultValue.x );
			return vector;
		}
	}

	return defaultValue;
}

Vector2i StyleSheetProperty::asVector2i( UINode* node, const Vector2i& defaultValue ) const {
	if ( !mValue.empty() ) {
		Vector2f vectorF( asVector2f( node, Vector2f( defaultValue.x, defaultValue.y ) ) );
		return Vector2i( vectorF.x, vectorF.y );
	}
	return defaultValue;
}

Sizef StyleSheetProperty::asSizef( UINode* node, const Sizef& defaultValue ) const {
	return Sizef( asVector2f( node, defaultValue ) );
}

Sizei StyleSheetProperty::asSizei( UINode* node, const Sizei& defaultValue ) const {
	return Sizei( asVector2i( node, defaultValue ) );
}

StyleSheetLength StyleSheetProperty::asStyleSheetLength() const {
	return StyleSheetLength( mValue );
}

const String::HashType& StyleSheetProperty::getValueHash() const {
	return mValueHash;
}

const std::vector<VariableFunctionCache>& StyleSheetProperty::getVarCache() const {
	return mVarCache;
}

}}} // namespace EE::UI::CSS
