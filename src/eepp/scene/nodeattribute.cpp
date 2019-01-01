#include <eepp/scene/nodeattribute.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/text.hpp>
#include <eepp/ui/uihelper.hpp>
#include <algorithm>

using namespace EE::Graphics;

namespace EE { namespace Scene {

NodeAttribute::Info::Info( NodeAttribute::AttributeType type, const std::string& name ) :
	type( type ),
	names( { name } )
{}

NodeAttribute::Info::Info( AttributeType type, const std::vector<std::string>& names ) :
	type( type ),
	names( names )
{}

bool NodeAttribute::Info::isName( const std::string& name ) {
	return std::find(names.begin(), names.end(), name) != names.end();
}

const NodeAttribute::AttributeType& NodeAttribute::Info::getType() const {
	return type;
}

const std::vector<std::string>& NodeAttribute::Info::getNames() const {
	return names;
}

NodeAttribute::NodeAttribute()
{}

NodeAttribute::NodeAttribute( std::string name, std::string value ) :
	mName( String::toLower( name ) ),
	mValue( value )
{}

bool NodeAttribute::isEmpty() const {
	return mName.empty();
}

std::string NodeAttribute::getName() const {
	return mName;
}

void NodeAttribute::setName( const std::string& name ) {
	mName = name;
}

const std::string& NodeAttribute::getValue() const {
	return mValue;
}

const std::string& NodeAttribute::value() const {
	return mValue;
}

void NodeAttribute::setValue( const std::string& value ) {
	mValue = value;
}

std::string NodeAttribute::asString( const std::string& defaultValue ) const {
	return mValue.empty() ? defaultValue : mValue;
}

int NodeAttribute::asInt( int defaultValue ) const {
	return asType<int>( defaultValue );
}

unsigned int NodeAttribute::asUint( unsigned int defaultValue ) const {
	return asType<unsigned int>( defaultValue );
}

double NodeAttribute::asDouble( double defaultValue ) const {
	return asType<double>( defaultValue );
}

float NodeAttribute::asFloat(float defaultValue) const {
	return asType<float>( defaultValue );
}

long long NodeAttribute::asLlong(long long defaultValue) const {
	return asType<long long>( defaultValue );
}

unsigned long long NodeAttribute::asUllong(unsigned long long defaultValue) const {
	return asType<unsigned long long>( defaultValue );
}

bool NodeAttribute::asBool( bool defaultValue ) const {
	if ( mValue.empty() )
		return defaultValue;

	// only look at first char
	char first = mValue[0];

	// 1*, t* (true), T* (True), y* (yes), Y* (YES)
	return (first == '1' || first == 't' || first == 'T' || first == 'y' || first == 'Y');
}

Color NodeAttribute::asColor() const {
	return Color::fromString( mValue );
}

Float NodeAttribute::asDpDimension( const std::string& defaultValue ) const {
	return PixelDensity::toDpFromString( asString( defaultValue ) );
}

int NodeAttribute::asDpDimensionI( const std::string& defaultValue ) const {
	return PixelDensity::toDpFromStringI( asString( defaultValue ) );
}

Uint32 NodeAttribute::asDpDimensionUint( const std::string& defaultValue ) const {
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
		std::vector<std::string> parts = String::split( val, ',' );

		if ( parts.size() == 2 ) {
			Float x = 0;
			Float y = 0;

			bool Res1 = String::fromString<Float>( x, parts[0] );
			bool Res2 = String::fromString<Float>( y, parts[1] );

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

	if ( val == "add" ) blendMode = BlendAdd;
	else if ( val == "alpha" ) blendMode = BlendAlpha;
	else if ( val == "multiply" ) blendMode = BlendMultiply;
	else if ( val == "none" ) blendMode = BlendNone;

	return blendMode;
}

OriginPoint NodeAttribute::asOriginPoint() const {
	return toOriginPoint( mValue );
}

BlendMode NodeAttribute::asBlendMode() const {
	return toBlendMode( mValue );
}

Vector2f NodeAttribute::asVector2f( const Vector2f & defaultValue ) const {
	if ( !mValue.empty() && mValue.find( "," ) != std::string::npos ) {
		Vector2f vector;
		auto xySplit = String::split( mValue, ',', true );

		if ( xySplit.size() == 2 ) {
			Float val;

			vector.x = String::fromString<Float>( val, xySplit[0] ) ? val : defaultValue.x;
			vector.y = String::fromString<Float>( val, xySplit[1] ) ? val : defaultValue.y;

			return vector;
		}
	}

	return defaultValue;
}

Vector2i NodeAttribute::asVector2i( const Vector2i & defaultValue ) const {
	if ( !mValue.empty() && mValue.find( "," ) != std::string::npos ) {
		Vector2i vector;
		auto xySplit = String::split( mValue, ',', true );

		if ( xySplit.size() == 2 ) {
			int val;

			vector.x = String::fromString<int>( val, xySplit[0] ) ? val : defaultValue.x;
			vector.y = String::fromString<int>( val, xySplit[1] ) ? val : defaultValue.y;

			return vector;
		}
	}

	return defaultValue;
}

Sizef NodeAttribute::asSizef( const Sizef& defaultValue ) const {
	return Sizef( asVector2f( defaultValue ) );
}

Sizei NodeAttribute::asSizei( const Sizei& defaultValue ) const {
	return Sizei( asVector2i( defaultValue ) );
}

Rect NodeAttribute::asRect( const Rect& defaultValue ) const {
	if ( !mValue.empty() && mValue.find( " " ) != std::string::npos ) {
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
			rect.Left = rect.Top = rect.Right = rect.Bottom = PixelDensity::toDpFromStringI( ltrbSplit[0] );
		}

		return rect;
	}

	return defaultValue;
}

Rectf NodeAttribute::asRectf( const Rectf& defaultValue ) const {
	Rectf rect( defaultValue );

	if ( !mValue.empty() && mValue.find( " " ) != std::string::npos ) {
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
		}
	} else if ( !mValue.empty() ) {
		rect.Left = rect.Top = rect.Right = rect.Bottom = PixelDensity::toDpFromString( mValue );
	}

	return rect;
}

Uint32 NodeAttribute::asFontStyle() const {
	std::string valStr = asString();
	String::toLowerInPlace( valStr );
	std::vector<std::string> strings = String::split( valStr, '|' );
	Uint32 flags = Text::Regular;

	if ( strings.size() ) {
		for ( std::size_t i = 0; i < strings.size(); i++ ) {
			std::string cur = strings[i];
			String::toLowerInPlace( cur );

			if ( "underlined" == cur || "underline" == cur )
				flags |= Text::Underlined;
			else if ( "bold" == cur )
				flags |= Text::Bold;
			else if ( "italic" == cur )
				flags |= Text::Italic;
			else if ( "strikethrough" == cur )
				flags |= Text::StrikeThrough;
			else if ( "shadowed" == cur || "shadow" == cur )
				flags |= Text::Shadow;
			else if ( "wordwrap" == cur )
				flags |= UI::UI_WORD_WRAP;
		}
	}

	return flags;
}

}}
