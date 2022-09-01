#include <cmath>
#include <cstdlib>
#include <ctype.h>
#include <eepp/core/string.hpp>
#include <eepp/system/color.hpp>
#include <eepp/system/functionstring.hpp>
#include <iomanip>

namespace EE { namespace System {

namespace {

template <typename T> inline T _round( T r ) {
	return ( r > 0.0f ) ? eefloor( r + 0.5f ) : eeceil( r - 0.5f );
}

} // namespace

std::map<std::string, Color> Color::sColors;

// TODO: Support all CSS3 color keywords.
// Reference: https://www.w3.org/TR/2018/REC-css-color-3-20180619/
const Color Color::Transparent = Color( 0x00000000 );
const Color Color::Black = Color( 0x000000FF );
const Color Color::Silver = Color( 0xC0C0C0FF );
const Color Color::Gray = Color( 0x808080FF );
const Color Color::White = Color( 0xFFFFFFFF );
const Color Color::Maroon = Color( 0x800000FF );
const Color Color::Red = Color( 0xFF0000FF );
const Color Color::Purple = Color( 0x800080FF );
const Color Color::Fuchsia = Color( 0xFF00FFFF );
const Color Color::Green = Color( 0x008000FF );
const Color Color::Lime = Color( 0x00FF00FF );
const Color Color::Olive = Color( 0x808000FF );
const Color Color::Yellow = Color( 0xFFFF00FF );
const Color Color::Navy = Color( 0x000080FF );
const Color Color::Blue = Color( 0x0000FFFF );
const Color Color::Teal = Color( 0x008080FF );
const Color Color::Aqua = Color( 0x00FFFFFF );

RGB::RGB() : tRGB<Uint8>() {}

/** Creates an RGB color from each component.
**	@param r Red component
**	@param g Green component
**	@param b Blue component
*/
RGB::RGB( Uint8 r, Uint8 g, Uint8 b ) : tRGB<Uint8>( r, g, b ) {}

RGB::RGB( const tRGB<Uint8>& color ) : tRGB<Uint8>( color.r, color.g, color.b ) {}

RGB::RGB( Uint32 Col ) {
	Col = BitOp::swapLE32( Col );
	r = static_cast<Uint8>( Col >> 16 );
	g = static_cast<Uint8>( Col >> 8 );
	b = static_cast<Uint8>( Col >> 0 );
}

Color RGB::toColor() {
	return Color( r, g, b );
}

/** Blend a source color to destination color */
ColorAf Color::blend( ColorAf srcf, ColorAf dstf ) {
	Float alpha = srcf.a + dstf.a * ( 1.f - srcf.a );
	Float red = ( srcf.r * srcf.a + dstf.r * dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float green = ( srcf.g * srcf.a + dstf.g * dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float blue = ( srcf.b * srcf.a + dstf.b * dstf.a * ( 1.f - srcf.a ) ) / alpha;

	return ColorAf( red, green, blue, alpha );
}

#define EE_COLOR_BLEND_FTOU8( color ) ( Uint8 )( color == 1.f ? 255 : ( color * 255.99f ) )

/** Blend a source color to destination color */
Color Color::blend( Color src, Color dst ) {
	ColorAf srcf( (Float)src.r / 255.f, (Float)src.g / 255.f, (Float)src.b / 255.f,
				  (Float)src.a / 255.f );
	ColorAf dstf( (Float)dst.r / 255.f, (Float)dst.g / 255.f, (Float)dst.b / 255.f,
				  (Float)dst.a / 255.f );
	Float alpha = srcf.a + dstf.a * ( 1.f - srcf.a );
	Float red = ( srcf.r * srcf.a + dstf.r * dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float green = ( srcf.g * srcf.a + dstf.g * dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float blue = ( srcf.b * srcf.a + dstf.b * dstf.a * ( 1.f - srcf.a ) ) / alpha;

	return Color( EE_COLOR_BLEND_FTOU8( red ), EE_COLOR_BLEND_FTOU8( green ),
				  EE_COLOR_BLEND_FTOU8( blue ), EE_COLOR_BLEND_FTOU8( alpha ) );
}

Color::Color() : tColor<Uint8>() {}

Color::Color( const Color& c ) {
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

Color::Color( const std::string& colorString ) {
	Color c( fromString( colorString ) );
	r = c.r;
	g = c.g;
	b = c.b;
	a = c.a;
}

Color::Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a ) : tColor<Uint8>( r, g, b, a ) {}

Color::Color( const tRGB<Uint8>& Col ) : tColor<Uint8>( Col ) {}

Color::Color( const tRGB<Uint8>& Col, Uint8 a ) : tColor<Uint8>( Col, a ) {}

Color::Color( const tColor<Uint8>& Col, Uint8 a ) : tColor<Uint8>( Col.r, Col.g, Col.b, a ) {}

Color::Color( const tColor<Uint8>& Col ) : tColor<Uint8>( Col.r, Col.g, Col.b, Col.a ) {}

Color::Color( const Uint32& Col ) : tColor<Uint8>( Col ) {}

Color& Color::operator=( const Color& col ) {
	r = col.r;
	g = col.g;
	b = col.b;
	a = col.a;
	return *this;
}

Colorf Color::toHsv() const {
	Colorf hsv;
	Colorf rgba( this->r / 255.f, this->g / 255.f, this->b / 255.f, this->a / 255.f );
	Float rgbMax = eemax( rgba.r, eemax( rgba.g, rgba.b ) );
	Float rgbMin = eemin( rgba.r, eemin( rgba.g, rgba.b ) );
	Float delta = ( rgbMax - rgbMin );

	hsv.hsv.v = rgbMax;
	hsv.hsv.a = rgba.a;

	if ( hsv.hsv.v == 0 ) {
		hsv.hsv.h = 0;
		hsv.hsv.s = 0;
		return hsv;
	}

	hsv.hsv.s = delta / rgbMax;
	if ( hsv.hsv.s == 0 ) {
		hsv.hsv.h = 0;
		return hsv;
	}

	if ( delta == 0 ) {
		hsv.hsv.h = 0;
		return hsv;
	} else if ( rgba.r == rgbMax ) {
		hsv.hsv.h = ( rgba.g - rgba.b ) / delta;
	} else if ( rgba.g == rgbMax ) {
		hsv.hsv.h = 2.f + ( rgba.b - rgba.r ) / delta;
	} else {
		hsv.hsv.h = 4.f + ( rgba.r - rgba.g ) / delta;
	}

	hsv.hsv.h *= 60.f;

	if ( hsv.hsv.h < 0.f )
		hsv.hsv.h += 360.f;

	return hsv;
}

Color Color::fromHsv( const Colorf& hsv ) {
	Color rgba( Color::Transparent );
	Float c = hsv.hsv.v * hsv.hsv.s;
	Float k = hsv.hsv.h / 60.f;
	Float x = c * ( 1 - eeabs( eemod( k, 2 ) - 1 ) );
	Float r1 = 0, g1 = 0, b1 = 0;
	if ( k >= 0 && k <= 1 ) {
		r1 = c;
		g1 = x;
	}
	if ( k > 1 && k <= 2 ) {
		r1 = x;
		g1 = c;
	}
	if ( k > 2 && k <= 3 ) {
		g1 = c;
		b1 = x;
	}
	if ( k > 3 && k <= 4 ) {
		g1 = x;
		b1 = c;
	}
	if ( k > 4 && k <= 5 ) {
		r1 = x;
		b1 = c;
	}
	if ( k > 5 && k <= 6 ) {
		r1 = c;
		b1 = x;
	}
	Float m = hsv.hsv.v - c;
	rgba.r = static_cast<Uint8>( eemax( 0.f, eemin( 255.f, r1 + m ) ) * 255.f + 0.5f );
	rgba.g = static_cast<Uint8>( eemax( 0.f, eemin( 255.f, g1 + m ) ) * 255.f + 0.5f );
	rgba.b = static_cast<Uint8>( eemax( 0.f, eemin( 255.f, b1 + m ) ) * 255.f + 0.5f );
	rgba.a = static_cast<Uint8>( hsv.hsv.a * 255.f + 0.5f );
	return rgba;
}

Colorf Color::toHsl() const {
	Colorf hsl;
	Float r = this->r / 255.f;
	Float g = this->g / 255.f;
	Float b = this->b / 255.f;
	Float a = this->a / 255.f;
	Float max = eemax( r, eemax( g, b ) );
	Float min = eemin( r, eemin( g, b ) );
	Float h, s, l = ( max + min ) / 2.f;

	if ( max == min ) {
		h = s = 0; // achromatic
	} else {
		float d = max - min;

		s = l > 0.5f ? d / ( 2.f - max - min ) : d / ( max + min );

		if ( r > g && r > b ) {
			h = ( g - b ) / d + ( g < b ? 6.f : 0.f );
		} else if ( g > b ) {
			h = ( b - r ) / d + 2.f;
		} else {
			h = ( r - g ) / d + 4.f;
		}

		h /= 6.f;
	}

	hsl.hsl.h = h;
	hsl.hsl.s = s;
	hsl.hsl.l = l;
	hsl.hsl.a = a;

	return hsl;
}

Color Color::clone() const {
	return Color( *this );
}

Color Color::invert() const {
	Uint32 red = ( getValue() >> 24 ) & 0xFF;
	Uint32 green = ( getValue() >> 16 ) & 0xFF;
	Uint32 blue = ( getValue() >> 8 ) & 0xFF;
	Uint32 alpha = ( getValue() >> 0 ) & 0xFF;
	return Color( ( ~red ) & 0xFF, ( ~green ) & 0xFF, ( ~blue ) & 0xFF, alpha );
}

Color Color::div( int divisor, bool divAlpha ) {
	Color c( *this );
	c.r /= divisor;
	c.g /= divisor;
	c.b /= divisor;
	if ( divAlpha )
		c.a /= divisor;
	return c;
}

std::string Color::toHexString( const bool& prependHashtag ) const {
	std::stringstream stream;
	if ( prependHashtag )
		stream << "#";
	stream << std::setfill( '0' ) << std::setw( sizeof( Color ) * 2 ) << std::hex << getValue();
	std::string str = stream.str();
	if ( this->a == 255 )
		return str.substr( 0, prependHashtag ? 7 : 6 );
	return str;
}

std::string Color::toRgbaString() const {
	return String::format( "rgba(%d, %d, %d, %.2f)", r, g, b, a / 255.f );
}

std::string Color::toRgbString() const {
	return String::format( "rgb(%d, %d, %d)", r, g, b );
}

Color& Color::blendAlpha( const Uint8& alpha ) {
	if ( alpha != 255 )
		this->a = static_cast<Uint8>( ( alpha * this->a ) / 255.f );
	return *this;
}

static Float hue2rgb( Float p, Float q, Float t ) {
	if ( t < 0.f )
		t += 1.f;
	if ( t > 1.f )
		t -= 1.f;
	if ( t < 1.f / 6.f )
		return p + ( q - p ) * 6.f * t;
	if ( t < 1.f / 2.f )
		return q;
	if ( t < 2.f / 3.f )
		return p + ( q - p ) * ( 2.f / 3.f - t ) * 6.f;
	return p;
}

Color Color::fromHsl( const Colorf& hsl ) {
	Color rgba;

	if ( hsl.hsl.s == 0 ) {
		rgba.r = rgba.g = rgba.b = (Uint8)_round( hsl.hsl.l ); // achromatic
	} else {
		Float q = hsl.hsl.l < 0.5f ? hsl.hsl.l * ( 1.f + hsl.hsl.s )
								   : hsl.hsl.l + hsl.hsl.s - hsl.hsl.l * hsl.hsl.s;
		Float p = 2.f * hsl.hsl.l - q;
		rgba.r = hue2rgb( p, q, hsl.hsl.h + 1.f / 3.f );
		rgba.g = hue2rgb( p, q, hsl.hsl.h );
		rgba.b = hue2rgb( p, q, hsl.hsl.h - 1.f / 3.f );
	}

	return Color( (Uint8)_round( rgba.r * 255.f ), (Uint8)_round( rgba.g * 255.f ),
				  (Uint8)_round( rgba.b * 255.f ), _round( hsl.hsl.a * 255.f ) );
}

Color Color::fromPointer( void* ptr ) {
	UintPtr val = (UintPtr)ptr;

	// hash the pointer up nicely
	val = ( val + 0x7ed55d16 ) + ( val << 12 );
	val = ( val ^ 0xc761c23c ) ^ ( val >> 19 );
	val = ( val + 0x165667b1 ) + ( val << 5 );
	val = ( val + 0xd3a2646c ) ^ ( val << 9 );
	val = ( val + 0xfd7046c5 ) + ( val << 3 );
	val = ( val ^ 0xb55a4f09 ) ^ ( val >> 16 );

	unsigned char r = ( val >> 0 ) & 0xFF;
	unsigned char g = ( val >> 8 ) & 0xFF;
	unsigned char b = ( val >> 16 ) & 0xFF;

	unsigned char max = r > g ? ( r > b ? r : b ) : ( g > b ? g : b );

	const int mult = 127;
	const int add = 63;
	r = ( r * mult ) / max + add;
	g = ( g * mult ) / max + add;
	b = ( b * mult ) / max + add;

	return Color( r, g, b, 255 );
}

Color Color::fromString( const char* str ) {
	return fromString( std::string( str ) );
}

Color Color::fromString( std::string str ) {
	std::size_t size = str.size();

	if ( 0 == size )
		return Color::Transparent;

	if ( str[0] == '#' ) {
		str = str.substr( 1 );

		size = str.size();

		if ( 0 == size )
			return Color::Transparent;

		if ( size < 6 ) {
			for ( std::size_t i = size; i < 6; i++ )
				str += str[size - 1];

			size = 6;
		}

		if ( 6 == size )
			str += "FF";

		return Color( std::strtoul( str.c_str(), NULL, 16 ) );
	} else if ( String::startsWith( str, "rgba(" ) || String::startsWith( str, "rgb(" ) ) {
		FunctionString functionString = FunctionString::parse( str );

		if ( ( functionString.getName() == "rgba" && functionString.getParameters().size() >= 4 ) ||
			 ( functionString.getName() == "rgb" && functionString.getParameters().size() >= 3 ) ) {
			Color color( Color::Transparent );

			for ( int i = 0; i < 3; i++ ) {
				Float val = 0;
				if ( String::fromString<Float>( val, functionString.getParameters().at( i ) ) ) {
					switch ( i ) {
						case 0:
							color.r = static_cast<Uint8>( eemax( eemin( 255.f, val ), 0.f ) );
							break;
						case 1:
							color.g = static_cast<Uint8>( eemax( eemin( 255.f, val ), 0.f ) );
							break;
						case 2:
							color.b = static_cast<Uint8>( eemax( eemin( 255.f, val ), 0.f ) );
							break;
						default:
							break;
					}
				} else {
					return Color::Transparent;
				}
			}

			Float val = 255;
			if ( functionString.getParameters().size() >= 4 ) {
				if ( String::fromString<Float>( val, functionString.getParameters().at( 3 ) ) ) {
					color.a = static_cast<Uint8>( eemax( eemin( 1.f, val ), 0.f ) * 255.f + 0.5f );
				} else {
					return Color::Transparent;
				}
			} else {
				color.a = val;
			}

			return color;
		} else if ( functionString.getName() == "rgba" &&
					functionString.getParameters().size() >= 2 &&
					isColorString( functionString.getParameters()[0] ) ) {
			// Allow creating rgba( #AABBCC, 0.5 )
			Color color( Color::fromString( functionString.getParameters()[0] ) );
			Float val = 255;
			if ( String::fromString<Float>( val, functionString.getParameters().at( 1 ) ) )
				color.a = static_cast<Uint8>( eemax( eemin( 1.f, val ), 0.f ) * 255.f + 0.5f );

			return color;
		} else {
			return Color::Transparent;
		}
	} else if ( String::startsWith( str, "hsla(" ) || String::startsWith( str, "hsl(" ) ) {
		FunctionString functionString = FunctionString::parse( str );

		if ( ( functionString.getName() == "hsla" && functionString.getParameters().size() >= 4 ) ||
			 ( functionString.getName() == "hsl" && functionString.getParameters().size() >= 3 ) ) {
			Colorf color;

			Float hueVal, saturationVal, lightnessVal;

			int hueIntVal;
			if ( String::fromString<int>( hueIntVal, functionString.getParameters().at( 0 ) ) &&
				 hueIntVal >= 0 && hueIntVal <= 360 ) {
				hueVal = hueIntVal / 360.f;
			} else {
				return Color::Transparent;
			}

			Float saturationFloatVal;
			std::string saturationString( functionString.getParameters().at( 1 ) );
			String::replaceAll( saturationString, "%", "" );
			if ( String::fromString<Float>( saturationFloatVal, saturationString ) &&
				 saturationFloatVal >= 0 && saturationFloatVal <= 100 ) {
				saturationVal = saturationFloatVal / 100.f;
			} else {
				return Color::Transparent;
			}

			Float lightnessFloatVal;
			std::string lightnessString( functionString.getParameters().at( 2 ) );
			String::replaceAll( lightnessString, "%", "" );
			if ( String::fromString<Float>( lightnessFloatVal, lightnessString ) &&
				 lightnessFloatVal >= 0 && lightnessFloatVal <= 100 ) {
				lightnessVal = lightnessFloatVal / 100.f;
			} else {
				return Color::Transparent;
			}

			Float alphaVal = 1;

			if ( functionString.getParameters().size() >= 4 ) {
				if ( String::fromString<Float>( alphaVal,
												functionString.getParameters().at( 3 ) ) ) {
					alphaVal = eemax( eemin( 1.f, alphaVal ), 0.f );
				} else {
					return Color::Transparent;
				}
			}

			color.hsl.h = hueVal;
			color.hsl.s = saturationVal;
			color.hsl.l = lightnessVal;
			color.hsl.a = alphaVal;

			return Color::fromHsl( color );
		} else {
			return Color::Transparent;
		}
	} else if ( String::startsWith( str, "hsva(" ) || String::startsWith( str, "hsv(" ) ) {
		FunctionString functionString = FunctionString::parse( str );

		if ( ( functionString.getName() == "hsva" && functionString.getParameters().size() >= 4 ) ||
			 ( functionString.getName() == "hsv" && functionString.getParameters().size() >= 3 ) ) {
			Colorf color;

			Float hueVal, saturationVal, valueVal;

			int hueIntVal;
			if ( String::fromString<int>( hueIntVal, functionString.getParameters().at( 0 ) ) &&
				 hueIntVal >= 0 && hueIntVal <= 360 ) {
				hueVal = hueIntVal;
			} else {
				return Color::Transparent;
			}

			Float saturationFloatVal;
			std::string saturationString( functionString.getParameters().at( 1 ) );
			String::replaceAll( saturationString, "%", "" );
			if ( String::fromString<Float>( saturationFloatVal, saturationString ) &&
				 saturationFloatVal >= 0 && saturationFloatVal <= 100 ) {
				saturationVal = saturationFloatVal / 100.f;
			} else {
				return Color::Transparent;
			}

			Float valueFloatVal;
			std::string valueString( functionString.getParameters().at( 2 ) );
			String::replaceAll( valueString, "%", "" );
			if ( String::fromString<Float>( valueFloatVal, valueString ) && valueFloatVal >= 0 &&
				 valueFloatVal <= 100 ) {
				valueVal = valueFloatVal / 100.f;
			} else {
				return Color::Transparent;
			}

			Float alphaVal = 1;

			if ( functionString.getParameters().size() >= 4 ) {
				if ( String::fromString<Float>( alphaVal,
												functionString.getParameters().at( 3 ) ) ) {
					alphaVal = eemax( eemin( 1.f, alphaVal ), 0.f );
				} else {
					return Color::Transparent;
				}
			}

			color.hsv.h = hueVal;
			color.hsv.s = saturationVal;
			color.hsv.v = valueVal;
			color.hsv.a = alphaVal;

			return Color::fromHsv( color );
		} else {
			return Color::Transparent;
		}
	} else if ( String::startsWith( str, "@color/" ) ) {
		std::string colorName( String::toLower( str.substr( 7 ) ) );
		const auto& it = sColors.find( colorName );

		if ( it != sColors.end() ) {
			return it->second;
		} else {
			return Color::Transparent;
		}
	} else if ( size >= 3 && isalpha( str[0] ) && isalpha( str[1] ) && isalpha( str[2] ) ) {
		String::toLowerInPlace( str );
		if ( "transparent" == str )
			return Color::Transparent;
		else if ( "black" == str )
			return Color::Black;
		else if ( "silver" == str )
			return Color::Silver;
		else if ( "gray" == str )
			return Color::Gray;
		else if ( "white" == str )
			return Color::White;
		else if ( "maroon" == str )
			return Color::Maroon;
		else if ( "red" == str )
			return Color::Red;
		else if ( "purple" == str )
			return Color::Purple;
		else if ( "fuchsia" == str )
			return Color::Fuchsia;
		else if ( "green" == str )
			return Color::Green;
		else if ( "lime" == str )
			return Color::Lime;
		else if ( "olive" == str )
			return Color::Olive;
		else if ( "yellow" == str )
			return Color::Yellow;
		else if ( "navy" == str )
			return Color::Navy;
		else if ( "blue" == str )
			return Color::Blue;
		else if ( "teal" == str )
			return Color::Teal;
		else if ( "aqua" == str )
			return Color::Aqua;
	} else {
		String::toLowerInPlace( str );
		const auto& it = sColors.find( str );

		if ( it != sColors.end() ) {
			return it->second;
		} else {
			return Color::Transparent;
		}
	}

	return Color::Transparent;
}

bool Color::isColorString( std::string str ) {
	if ( str.empty() )
		return false;

	if ( str[0] == '#' )
		return true;

	String::toLowerInPlace( str );
	if ( "transparent" == str )
		return true;
	else if ( "black" == str )
		return true;
	else if ( "silver" == str )
		return true;
	else if ( "gray" == str )
		return true;
	else if ( "white" == str )
		return true;
	else if ( "maroon" == str )
		return true;
	else if ( "red" == str )
		return true;
	else if ( "purple" == str )
		return true;
	else if ( "fuchsia" == str )
		return true;
	else if ( "green" == str )
		return true;
	else if ( "lime" == str )
		return true;
	else if ( "olive" == str )
		return true;
	else if ( "yellow" == str )
		return true;
	else if ( "navy" == str )
		return true;
	else if ( "blue" == str )
		return true;
	else if ( "teal" == str )
		return true;
	else if ( "aqua" == str )
		return true;
	else if ( String::startsWith( str, "rgb(" ) )
		return true;
	else if ( String::startsWith( str, "rgba(" ) )
		return true;
	else if ( String::startsWith( str, "hsl(" ) )
		return true;
	else if ( String::startsWith( str, "hsla(" ) )
		return true;
	else if ( String::startsWith( str, "hsv(" ) )
		return true;
	else if ( String::startsWith( str, "hsva(" ) )
		return true;
	else if ( String::startsWith( str, "@color/" ) )
		return true;
	else if ( sColors.find( str ) != sColors.end() )
		return true;

	return false;
}

void Color::registerColor( const std::string& name, const Color& color ) {
	sColors[String::toLower( name )] = color;
}

bool Color::unregisterColor( const std::string& name ) {
	return sColors.erase( String::toLower( name ) ) > 0;
}

bool Color::validHexColorString( const std::string& hexColor ) {
	if ( hexColor.size() < 2 || hexColor[0] != '#' )
		return false;

	for ( size_t i = 1; i < hexColor.size(); i++ ) {
		if ( !( String::isNumber( hexColor[i] ) || ( hexColor[i] >= 'a' && hexColor[i] <= 'f' ) ||
				( hexColor[i] >= 'A' && hexColor[i] <= 'F' ) ) ) {
			return false;
		}
	}

	return true;
}

}} // namespace EE::System
