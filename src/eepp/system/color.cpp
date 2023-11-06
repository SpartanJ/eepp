#include <cmath>
#include <cstdlib>
#include <ctype.h>
#include <eepp/system/color.hpp>
#include <eepp/system/functionstring.hpp>
#include <iomanip>

namespace EE { namespace System {

namespace {

template <typename T> inline T _round( T r ) {
	return ( r > 0.0f ) ? eefloor( r + 0.5f ) : eeceil( r - 0.5f );
}

} // namespace

UnorderedMap<std::string, Color> Color::sColors;
UnorderedMap<String::HashType, Color> Color::sColorMap;

// Keep the old defined colors
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

// Reference: https://www.w3.org/TR/css-color-3/
const Color Color::aliceblue = Color( 0xF0F8FFFF );
const Color Color::antiquewhite = Color( 0xFAEBD7FF );
const Color Color::aqua = Color( 0x00FFFFFF );
const Color Color::aquamarine = Color( 0x7FFFD4FF );
const Color Color::azure = Color( 0xF0FFFFFF );
const Color Color::beige = Color( 0xF5F5DCFF );
const Color Color::bisque = Color( 0xFFE4C4FF );
const Color Color::black = Color( 0x000000FF );
const Color Color::blanchedalmond = Color( 0xFFEBCDFF );
const Color Color::blue = Color( 0x0000FFFF );
const Color Color::blueviolet = Color( 0x8A2BE2FF );
const Color Color::brown = Color( 0xA52A2AFF );
const Color Color::burlywood = Color( 0xDEB887FF );
const Color Color::cadetblue = Color( 0x5F9EA0FF );
const Color Color::chartreuse = Color( 0x7FFF00FF );
const Color Color::chocolate = Color( 0xD2691EFF );
const Color Color::coral = Color( 0xFF7F50FF );
const Color Color::cornflowerblue = Color( 0x6495EDFF );
const Color Color::cornsilk = Color( 0xFFF8DCFF );
const Color Color::crimson = Color( 0xDC143CFF );
const Color Color::cyan = Color( 0x00FFFFFF );
const Color Color::darkblue = Color( 0x00008BFF );
const Color Color::darkcyan = Color( 0x008B8BFF );
const Color Color::darkgoldenrod = Color( 0xB8860BFF );
const Color Color::darkgray = Color( 0xA9A9A9FF );
const Color Color::darkgreen = Color( 0x006400FF );
const Color Color::darkgrey = Color( 0xA9A9A9FF );
const Color Color::darkkhaki = Color( 0xBDB76BFF );
const Color Color::darkmagenta = Color( 0x8B008BFF );
const Color Color::darkolivegreen = Color( 0x556B2FFF );
const Color Color::darkorange = Color( 0xFF8C00FF );
const Color Color::darkorchid = Color( 0x9932CCFF );
const Color Color::darkred = Color( 0x8B0000FF );
const Color Color::darksalmon = Color( 0xE9967AFF );
const Color Color::darkseagreen = Color( 0x8FBC8FFF );
const Color Color::darkslateblue = Color( 0x483D8BFF );
const Color Color::darkslategray = Color( 0x2F4F4FFF );
const Color Color::darkslategrey = Color( 0x2F4F4FFF );
const Color Color::darkturquoise = Color( 0x00CED1FF );
const Color Color::darkviolet = Color( 0x9400D3FF );
const Color Color::deeppink = Color( 0xFF1493FF );
const Color Color::deepskyblue = Color( 0x00BFFFFF );
const Color Color::dimgray = Color( 0x696969FF );
const Color Color::dimgrey = Color( 0x696969FF );
const Color Color::dodgerblue = Color( 0x1E90FFFF );
const Color Color::firebrick = Color( 0xB22222FF );
const Color Color::floralwhite = Color( 0xFFFAF0FF );
const Color Color::forestgreen = Color( 0x228B22FF );
const Color Color::fuchsia = Color( 0xFF00FFFF );
const Color Color::gainsboro = Color( 0xDCDCDCFF );
const Color Color::ghostwhite = Color( 0xF8F8FFFF );
const Color Color::gold = Color( 0xFFD700FF );
const Color Color::goldenrod = Color( 0xDAA520FF );
const Color Color::gray = Color( 0x808080FF );
const Color Color::green = Color( 0x008000FF );
const Color Color::greenyellow = Color( 0xADFF2FFF );
const Color Color::grey = Color( 0x808080FF );
const Color Color::honeydew = Color( 0xF0FFF0FF );
const Color Color::hotpink = Color( 0xFF69B4FF );
const Color Color::indianred = Color( 0xCD5C5CFF );
const Color Color::indigo = Color( 0x4B0082FF );
const Color Color::ivory = Color( 0xFFFFF0FF );
const Color Color::khaki = Color( 0xF0E68CFF );
const Color Color::lavender = Color( 0xE6E6FAFF );
const Color Color::lavenderblush = Color( 0xFFF0F5FF );
const Color Color::lawngreen = Color( 0x7CFC00FF );
const Color Color::lemonchiffon = Color( 0xFFFACDFF );
const Color Color::lightblue = Color( 0xADD8E6FF );
const Color Color::lightcoral = Color( 0xF08080FF );
const Color Color::lightcyan = Color( 0xE0FFFFFF );
const Color Color::lightgoldenrodyellow = Color( 0xFAFAD2FF );
const Color Color::lightgray = Color( 0xD3D3D3FF );
const Color Color::lightgreen = Color( 0x90EE90FF );
const Color Color::lightgrey = Color( 0xD3D3D3FF );
const Color Color::lightpink = Color( 0xFFB6C1FF );
const Color Color::lightsalmon = Color( 0xFFA07AFF );
const Color Color::lightseagreen = Color( 0x20B2AAFF );
const Color Color::lightskyblue = Color( 0x87CEFAFF );
const Color Color::lightslategray = Color( 0x778899FF );
const Color Color::lightslategrey = Color( 0x778899FF );
const Color Color::lightsteelblue = Color( 0xB0C4DEFF );
const Color Color::lightyellow = Color( 0xFFFFE0FF );
const Color Color::lime = Color( 0x00FF00FF );
const Color Color::limegreen = Color( 0x32CD32FF );
const Color Color::linen = Color( 0xFAF0E6FF );
const Color Color::magenta = Color( 0xFF00FFFF );
const Color Color::maroon = Color( 0x800000FF );
const Color Color::mediumaquamarine = Color( 0x66CDAAFF );
const Color Color::mediumblue = Color( 0x0000CDFF );
const Color Color::mediumorchid = Color( 0xBA55D3FF );
const Color Color::mediumpurple = Color( 0x9370DBFF );
const Color Color::mediumseagreen = Color( 0x3CB371FF );
const Color Color::mediumslateblue = Color( 0x7B68EEFF );
const Color Color::mediumspringgreen = Color( 0x00FA9AFF );
const Color Color::mediumturquoise = Color( 0x48D1CCFF );
const Color Color::mediumvioletred = Color( 0xC71585FF );
const Color Color::midnightblue = Color( 0x191970FF );
const Color Color::mintcream = Color( 0xF5FFFAFF );
const Color Color::mistyrose = Color( 0xFFE4E1FF );
const Color Color::moccasin = Color( 0xFFE4B5FF );
const Color Color::navajowhite = Color( 0xFFDEADFF );
const Color Color::navy = Color( 0x000080FF );
const Color Color::oldlace = Color( 0xFDF5E6FF );
const Color Color::olive = Color( 0x808000FF );
const Color Color::olivedrab = Color( 0x6B8E23FF );
const Color Color::orange = Color( 0xFFA500FF );
const Color Color::orangered = Color( 0xFF4500FF );
const Color Color::orchid = Color( 0xDA70D6FF );
const Color Color::palegoldenrod = Color( 0xEEE8AAFF );
const Color Color::palegreen = Color( 0x98FB98FF );
const Color Color::paleturquoise = Color( 0xAFEEEEFF );
const Color Color::palevioletred = Color( 0xDB7093FF );
const Color Color::papayawhip = Color( 0xFFEFD5FF );
const Color Color::peachpuff = Color( 0xFFDAB9FF );
const Color Color::peru = Color( 0xCD853FFF );
const Color Color::pink = Color( 0xFFC0CBFF );
const Color Color::plum = Color( 0xDDA0DDFF );
const Color Color::powderblue = Color( 0xB0E0E6FF );
const Color Color::purple = Color( 0x800080FF );
const Color Color::red = Color( 0xFF0000FF );
const Color Color::rosybrown = Color( 0xBC8F8FFF );
const Color Color::royalblue = Color( 0x4169E1FF );
const Color Color::saddlebrown = Color( 0x8B4513FF );
const Color Color::salmon = Color( 0xFA8072FF );
const Color Color::sandybrown = Color( 0xF4A460FF );
const Color Color::seagreen = Color( 0x2E8B57FF );
const Color Color::seashell = Color( 0xFFF5EEFF );
const Color Color::sienna = Color( 0xA0522DFF );
const Color Color::silver = Color( 0xC0C0C0FF );
const Color Color::skyblue = Color( 0x87CEEBFF );
const Color Color::slateblue = Color( 0x6A5ACDFF );
const Color Color::slategray = Color( 0x708090FF );
const Color Color::slategrey = Color( 0x708090FF );
const Color Color::snow = Color( 0xFFFAFAFF );
const Color Color::springgreen = Color( 0x00FF7FFF );
const Color Color::steelblue = Color( 0x4682B4FF );
const Color Color::tan = Color( 0xD2B48CFF );
const Color Color::teal = Color( 0x008080FF );
const Color Color::thistle = Color( 0xD8BFD8FF );
const Color Color::tomato = Color( 0xFF6347FF );
const Color Color::turquoise = Color( 0x40E0D0FF );
const Color Color::violet = Color( 0xEE82EEFF );
const Color Color::wheat = Color( 0xF5DEB3FF );
const Color Color::white = Color( 0xFFFFFFFF );
const Color Color::whitesmoke = Color( 0xF5F5F5FF );
const Color Color::yellow = Color( 0xFFFF00FF );
const Color Color::yellowgreen = Color( 0x9ACD32FF );

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
	Value = c.Value;
}

Color::Color( const std::string& colorString ) {
	Color c( fromString( colorString ) );
	Value = c.Value;
}

Color::Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a ) : tColor<Uint8>( r, g, b, a ) {}

Color::Color( const tRGB<Uint8>& Col ) : tColor<Uint8>( Col ) {}

Color::Color( const tRGB<Uint8>& Col, Uint8 a ) : tColor<Uint8>( Col, a ) {}

Color::Color( const tColor<Uint8>& Col, Uint8 a ) : tColor<Uint8>( Col.r, Col.g, Col.b, a ) {}

Color::Color( const tColor<Uint8>& Col ) : tColor<Uint8>( Col.r, Col.g, Col.b, Col.a ) {}

Color::Color( const Uint32& Col ) : tColor<Uint8>( Col ) {}

Color& Color::operator=( const Color& col ) {
	Value = col.Value;
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
		initColorMap();
		auto it = sColorMap.find( String::hash( str ) );
		if ( it != sColorMap.end() )
			return it->second;
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

	initColorMap();
	auto it = sColorMap.find( String::hash( str ) );
	if ( it != sColorMap.end() )
		return true;
	if ( String::startsWith( str, "rgb(" ) )
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

void Color::initColorMap() {
	if ( !sColorMap.empty() )
		return;
	sColorMap.insert( { String::hash( "transparent" ), Color::Transparent } );
	sColorMap.insert( { String::hash( "aliceblue" ), Color::aliceblue } );
	sColorMap.insert( { String::hash( "antiquewhite" ), Color::antiquewhite } );
	sColorMap.insert( { String::hash( "aqua" ), Color::aqua } );
	sColorMap.insert( { String::hash( "aquamarine" ), Color::aquamarine } );
	sColorMap.insert( { String::hash( "azure" ), Color::azure } );
	sColorMap.insert( { String::hash( "beige" ), Color::beige } );
	sColorMap.insert( { String::hash( "bisque" ), Color::bisque } );
	sColorMap.insert( { String::hash( "black" ), Color::black } );
	sColorMap.insert( { String::hash( "blanchedalmond" ), Color::blanchedalmond } );
	sColorMap.insert( { String::hash( "blue" ), Color::blue } );
	sColorMap.insert( { String::hash( "blueviolet" ), Color::blueviolet } );
	sColorMap.insert( { String::hash( "brown" ), Color::brown } );
	sColorMap.insert( { String::hash( "burlywood" ), Color::burlywood } );
	sColorMap.insert( { String::hash( "cadetblue" ), Color::cadetblue } );
	sColorMap.insert( { String::hash( "chartreuse" ), Color::chartreuse } );
	sColorMap.insert( { String::hash( "chocolate" ), Color::chocolate } );
	sColorMap.insert( { String::hash( "coral" ), Color::coral } );
	sColorMap.insert( { String::hash( "cornflowerblue" ), Color::cornflowerblue } );
	sColorMap.insert( { String::hash( "cornsilk" ), Color::cornsilk } );
	sColorMap.insert( { String::hash( "crimson" ), Color::crimson } );
	sColorMap.insert( { String::hash( "cyan" ), Color::cyan } );
	sColorMap.insert( { String::hash( "darkblue" ), Color::darkblue } );
	sColorMap.insert( { String::hash( "darkcyan" ), Color::darkcyan } );
	sColorMap.insert( { String::hash( "darkgoldenrod" ), Color::darkgoldenrod } );
	sColorMap.insert( { String::hash( "darkgray" ), Color::darkgray } );
	sColorMap.insert( { String::hash( "darkgreen" ), Color::darkgreen } );
	sColorMap.insert( { String::hash( "darkgrey" ), Color::darkgrey } );
	sColorMap.insert( { String::hash( "darkkhaki" ), Color::darkkhaki } );
	sColorMap.insert( { String::hash( "darkmagenta" ), Color::darkmagenta } );
	sColorMap.insert( { String::hash( "darkolivegreen" ), Color::darkolivegreen } );
	sColorMap.insert( { String::hash( "darkorange" ), Color::darkorange } );
	sColorMap.insert( { String::hash( "darkorchid" ), Color::darkorchid } );
	sColorMap.insert( { String::hash( "darkred" ), Color::darkred } );
	sColorMap.insert( { String::hash( "darksalmon" ), Color::darksalmon } );
	sColorMap.insert( { String::hash( "darkseagreen" ), Color::darkseagreen } );
	sColorMap.insert( { String::hash( "darkslateblue" ), Color::darkslateblue } );
	sColorMap.insert( { String::hash( "darkslategray" ), Color::darkslategray } );
	sColorMap.insert( { String::hash( "darkslategrey" ), Color::darkslategrey } );
	sColorMap.insert( { String::hash( "darkturquoise" ), Color::darkturquoise } );
	sColorMap.insert( { String::hash( "darkviolet" ), Color::darkviolet } );
	sColorMap.insert( { String::hash( "deeppink" ), Color::deeppink } );
	sColorMap.insert( { String::hash( "deepskyblue" ), Color::deepskyblue } );
	sColorMap.insert( { String::hash( "dimgray" ), Color::dimgray } );
	sColorMap.insert( { String::hash( "dimgrey" ), Color::dimgrey } );
	sColorMap.insert( { String::hash( "dodgerblue" ), Color::dodgerblue } );
	sColorMap.insert( { String::hash( "firebrick" ), Color::firebrick } );
	sColorMap.insert( { String::hash( "floralwhite" ), Color::floralwhite } );
	sColorMap.insert( { String::hash( "forestgreen" ), Color::forestgreen } );
	sColorMap.insert( { String::hash( "fuchsia" ), Color::fuchsia } );
	sColorMap.insert( { String::hash( "gainsboro" ), Color::gainsboro } );
	sColorMap.insert( { String::hash( "ghostwhite" ), Color::ghostwhite } );
	sColorMap.insert( { String::hash( "gold" ), Color::gold } );
	sColorMap.insert( { String::hash( "goldenrod" ), Color::goldenrod } );
	sColorMap.insert( { String::hash( "gray" ), Color::gray } );
	sColorMap.insert( { String::hash( "green" ), Color::green } );
	sColorMap.insert( { String::hash( "greenyellow" ), Color::greenyellow } );
	sColorMap.insert( { String::hash( "grey" ), Color::grey } );
	sColorMap.insert( { String::hash( "honeydew" ), Color::honeydew } );
	sColorMap.insert( { String::hash( "hotpink" ), Color::hotpink } );
	sColorMap.insert( { String::hash( "indianred" ), Color::indianred } );
	sColorMap.insert( { String::hash( "indigo" ), Color::indigo } );
	sColorMap.insert( { String::hash( "ivory" ), Color::ivory } );
	sColorMap.insert( { String::hash( "khaki" ), Color::khaki } );
	sColorMap.insert( { String::hash( "lavender" ), Color::lavender } );
	sColorMap.insert( { String::hash( "lavenderblush" ), Color::lavenderblush } );
	sColorMap.insert( { String::hash( "lawngreen" ), Color::lawngreen } );
	sColorMap.insert( { String::hash( "lemonchiffon" ), Color::lemonchiffon } );
	sColorMap.insert( { String::hash( "lightblue" ), Color::lightblue } );
	sColorMap.insert( { String::hash( "lightcoral" ), Color::lightcoral } );
	sColorMap.insert( { String::hash( "lightcyan" ), Color::lightcyan } );
	sColorMap.insert( { String::hash( "lightgoldenrodyellow" ), Color::lightgoldenrodyellow } );
	sColorMap.insert( { String::hash( "lightgray" ), Color::lightgray } );
	sColorMap.insert( { String::hash( "lightgreen" ), Color::lightgreen } );
	sColorMap.insert( { String::hash( "lightgrey" ), Color::lightgrey } );
	sColorMap.insert( { String::hash( "lightpink" ), Color::lightpink } );
	sColorMap.insert( { String::hash( "lightsalmon" ), Color::lightsalmon } );
	sColorMap.insert( { String::hash( "lightseagreen" ), Color::lightseagreen } );
	sColorMap.insert( { String::hash( "lightskyblue" ), Color::lightskyblue } );
	sColorMap.insert( { String::hash( "lightslategray" ), Color::lightslategray } );
	sColorMap.insert( { String::hash( "lightslategrey" ), Color::lightslategrey } );
	sColorMap.insert( { String::hash( "lightsteelblue" ), Color::lightsteelblue } );
	sColorMap.insert( { String::hash( "lightyellow" ), Color::lightyellow } );
	sColorMap.insert( { String::hash( "lime" ), Color::lime } );
	sColorMap.insert( { String::hash( "limegreen" ), Color::limegreen } );
	sColorMap.insert( { String::hash( "linen" ), Color::linen } );
	sColorMap.insert( { String::hash( "magenta" ), Color::magenta } );
	sColorMap.insert( { String::hash( "maroon" ), Color::maroon } );
	sColorMap.insert( { String::hash( "mediumaquamarine" ), Color::mediumaquamarine } );
	sColorMap.insert( { String::hash( "mediumblue" ), Color::mediumblue } );
	sColorMap.insert( { String::hash( "mediumorchid" ), Color::mediumorchid } );
	sColorMap.insert( { String::hash( "mediumpurple" ), Color::mediumpurple } );
	sColorMap.insert( { String::hash( "mediumseagreen" ), Color::mediumseagreen } );
	sColorMap.insert( { String::hash( "mediumslateblue" ), Color::mediumslateblue } );
	sColorMap.insert( { String::hash( "mediumspringgreen" ), Color::mediumspringgreen } );
	sColorMap.insert( { String::hash( "mediumturquoise" ), Color::mediumturquoise } );
	sColorMap.insert( { String::hash( "mediumvioletred" ), Color::mediumvioletred } );
	sColorMap.insert( { String::hash( "midnightblue" ), Color::midnightblue } );
	sColorMap.insert( { String::hash( "mintcream" ), Color::mintcream } );
	sColorMap.insert( { String::hash( "mistyrose" ), Color::mistyrose } );
	sColorMap.insert( { String::hash( "moccasin" ), Color::moccasin } );
	sColorMap.insert( { String::hash( "navajowhite" ), Color::navajowhite } );
	sColorMap.insert( { String::hash( "navy" ), Color::navy } );
	sColorMap.insert( { String::hash( "oldlace" ), Color::oldlace } );
	sColorMap.insert( { String::hash( "olive" ), Color::olive } );
	sColorMap.insert( { String::hash( "olivedrab" ), Color::olivedrab } );
	sColorMap.insert( { String::hash( "orange" ), Color::orange } );
	sColorMap.insert( { String::hash( "orangered" ), Color::orangered } );
	sColorMap.insert( { String::hash( "orchid" ), Color::orchid } );
	sColorMap.insert( { String::hash( "palegoldenrod" ), Color::palegoldenrod } );
	sColorMap.insert( { String::hash( "palegreen" ), Color::palegreen } );
	sColorMap.insert( { String::hash( "paleturquoise" ), Color::paleturquoise } );
	sColorMap.insert( { String::hash( "palevioletred" ), Color::palevioletred } );
	sColorMap.insert( { String::hash( "papayawhip" ), Color::papayawhip } );
	sColorMap.insert( { String::hash( "peachpuff" ), Color::peachpuff } );
	sColorMap.insert( { String::hash( "peru" ), Color::peru } );
	sColorMap.insert( { String::hash( "pink" ), Color::pink } );
	sColorMap.insert( { String::hash( "plum" ), Color::plum } );
	sColorMap.insert( { String::hash( "powderblue" ), Color::powderblue } );
	sColorMap.insert( { String::hash( "purple" ), Color::purple } );
	sColorMap.insert( { String::hash( "red" ), Color::red } );
	sColorMap.insert( { String::hash( "rosybrown" ), Color::rosybrown } );
	sColorMap.insert( { String::hash( "royalblue" ), Color::royalblue } );
	sColorMap.insert( { String::hash( "saddlebrown" ), Color::saddlebrown } );
	sColorMap.insert( { String::hash( "salmon" ), Color::salmon } );
	sColorMap.insert( { String::hash( "sandybrown" ), Color::sandybrown } );
	sColorMap.insert( { String::hash( "seagreen" ), Color::seagreen } );
	sColorMap.insert( { String::hash( "seashell" ), Color::seashell } );
	sColorMap.insert( { String::hash( "sienna" ), Color::sienna } );
	sColorMap.insert( { String::hash( "silver" ), Color::silver } );
	sColorMap.insert( { String::hash( "skyblue" ), Color::skyblue } );
	sColorMap.insert( { String::hash( "slateblue" ), Color::slateblue } );
	sColorMap.insert( { String::hash( "slategray" ), Color::slategray } );
	sColorMap.insert( { String::hash( "slategrey" ), Color::slategrey } );
	sColorMap.insert( { String::hash( "snow" ), Color::snow } );
	sColorMap.insert( { String::hash( "springgreen" ), Color::springgreen } );
	sColorMap.insert( { String::hash( "steelblue" ), Color::steelblue } );
	sColorMap.insert( { String::hash( "tan" ), Color::tan } );
	sColorMap.insert( { String::hash( "teal" ), Color::teal } );
	sColorMap.insert( { String::hash( "thistle" ), Color::thistle } );
	sColorMap.insert( { String::hash( "tomato" ), Color::tomato } );
	sColorMap.insert( { String::hash( "turquoise" ), Color::turquoise } );
	sColorMap.insert( { String::hash( "violet" ), Color::violet } );
	sColorMap.insert( { String::hash( "wheat" ), Color::wheat } );
	sColorMap.insert( { String::hash( "white" ), Color::white } );
	sColorMap.insert( { String::hash( "whitesmoke" ), Color::whitesmoke } );
	sColorMap.insert( { String::hash( "yellow" ), Color::yellow } );
	sColorMap.insert( { String::hash( "yellowgreen" ), Color::yellowgreen } );
}

}} // namespace EE::System
