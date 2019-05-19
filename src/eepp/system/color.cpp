#include <eepp/system/color.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/math.hpp>
#include <cstdlib>
#include <ctype.h>
#include <iomanip>

namespace EE { namespace System {

// @TODO: Support all CSS3 color keywords.
// Reference: https://www.w3.org/TR/2018/REC-css-color-3-20180619/
const Color Color::Transparent = Color(0x00000000);
const Color Color::Black = Color(0x000000FF);
const Color Color::Silver = Color(0xC0C0C0FF);
const Color Color::Gray = Color(0x808080FF);
const Color Color::White = Color(0xFFFFFFFF);
const Color Color::Maroon = Color(0x800000FF);
const Color Color::Red = Color(0xFF0000FF);
const Color Color::Purple = Color(0x800080FF);
const Color Color::Fuchsia = Color(0xFF00FFFF);
const Color Color::Green = Color(0x008000FF);
const Color Color::Lime = Color(0x00FF00FF);
const Color Color::Olive = Color(0x808000FF);
const Color Color::Yellow = Color(0xFFFF00FF);
const Color Color::Navy = Color(0x000080FF);
const Color Color::Blue = Color(0x0000FFFF);
const Color Color::Teal = Color(0x008080FF);
const Color Color::Aqua = Color(0x00FFFFFF);

RGB::RGB() : tRGB<Uint8>()
{
}

/** Creates an RGB color from each component.
**	@param r Red component
**	@param g Green component
**	@param b Blue component
*/
RGB::RGB( Uint8 r, Uint8 g, Uint8 b ) : tRGB<Uint8>( r, g, b )
{
}

RGB::RGB( const tRGB<Uint8>& color ) :
	tRGB<Uint8>( color.r, color.g, color.b )
{
}

RGB::RGB( Uint32 Col )
{
	Col	= BitOp::swapLE32( Col );
	r	= static_cast<Uint8>( Col >> 16	);
	g	= static_cast<Uint8>( Col >> 8	);
	b	= static_cast<Uint8>( Col >> 0	);
}

Color RGB::toColor() {
	return Color(r,g,b);
}

/** Blend a source color to destination color */
ColorAf Color::blend( ColorAf srcf, ColorAf dstf ) {
	Float alpha	= srcf.a + dstf.a * ( 1.f - srcf.a );
	Float red	= ( srcf.r	* srcf.a + dstf.r		* dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float green	= ( srcf.g	* srcf.a + dstf.g	* dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float blue	= ( srcf.b	* srcf.a + dstf.b	* dstf.a * ( 1.f - srcf.a ) ) / alpha;

	return ColorAf( red, green, blue, alpha );
}

#define EE_COLOR_BLEND_FTOU8(color) (Uint8)( color == 1.f ? 255 : (color * 255.99f))

/** Blend a source color to destination color */
Color Color::blend( Color src, Color dst ) {
	ColorAf srcf( (Float)src.r / 255.f, (Float)src.g / 255.f, (Float)src.b / 255.f, (Float)src.a / 255.f );
	ColorAf dstf( (Float)dst.r / 255.f, (Float)dst.g / 255.f, (Float)dst.b / 255.f, (Float)dst.a / 255.f );
	Float alpha	= srcf.a + dstf.a * ( 1.f - srcf.a );
	Float red	= ( srcf.r	* srcf.a + dstf.r		* dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float green	= ( srcf.g	* srcf.a + dstf.g	* dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float blue	= ( srcf.b	* srcf.a + dstf.b	* dstf.a * ( 1.f - srcf.a ) ) / alpha;

	return Color( EE_COLOR_BLEND_FTOU8(red), EE_COLOR_BLEND_FTOU8(green), EE_COLOR_BLEND_FTOU8(blue), EE_COLOR_BLEND_FTOU8(alpha) );
}

Color::Color() :
	tColor<Uint8>()
{
}

Color::Color( std::string colorString ) {
	Color c( fromString( colorString ) );
	r = c.r; g = c.g; b = c.b; a = c.a;
}

Color::Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) :
	tColor<Uint8>(r,g,b,a)
{}

Color::Color( const tRGB<Uint8>& Col ) :
	tColor<Uint8>( Col )
{}

Color::Color( const tRGB<Uint8>& Col, Uint8 a ) :
	tColor<Uint8>( Col, a )
{}

Color::Color( const tColor<Uint8> & Col, Uint8 a ) :
	tColor<Uint8>( Col.r, Col.g, Col.b, a )
{}

Color::Color( const tColor<Uint8>& Col ) :
	tColor<Uint8>( Col.Value )
{}

Color::Color( const Uint32& Col ) :
	tColor<Uint8>( Col )
{}

Color Color::toHsv() {
	Color hsv;
	Color rgb( *this );
	unsigned char rgbMin, rgbMax;

	rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b) : (rgb.g < rgb.b ? rgb.g : rgb.b);
	rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b) : (rgb.g > rgb.b ? rgb.g : rgb.b);

	hsv.hsv.a = rgb.a;

	hsv.hsv.v = rgbMax;
	if (hsv.hsv.v == 0){
		hsv.hsv.h = 0;
		hsv.hsv.s = 0;
		return hsv;
	}

	hsv.hsv.s = 255 * long(rgbMax - rgbMin) / hsv.hsv.v;
	if (hsv.hsv.s == 0) {
		hsv.hsv.h = 0;
		return hsv;
	}

	if (rgbMax == rgb.r)
		hsv.hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
	else if (rgbMax == rgb.g)
		hsv.hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
	else
		hsv.hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

	return hsv;
}

Color Color::fromHsv(const Color & hsv) {
	Color rgb;
	unsigned char region, remainder, p, q, t;

	rgb.a = hsv.hsv.a;

	if (hsv.hsv.s == 0) {
		rgb.r = hsv.hsv.v;
		rgb.g = hsv.hsv.v;
		rgb.b = hsv.hsv.v;
		return rgb;
	}

	region = hsv.hsv.h / 43;
	remainder = (hsv.hsv.h - (region * 43)) * 6;

	p = (hsv.hsv.v * (255 - hsv.hsv.s)) >> 8;
	q = (hsv.hsv.v * (255 - ((hsv.hsv.s * remainder) >> 8))) >> 8;
	t = (hsv.hsv.v * (255 - ((hsv.hsv.s * (255 - remainder)) >> 8))) >> 8;

	switch (region) {
		case 0:
			rgb.r = hsv.hsv.v; rgb.g = t; rgb.b = p;
			break;
		case 1:
			rgb.r = q; rgb.g = hsv.hsv.v; rgb.b = p;
			break;
		case 2:
			rgb.r = p; rgb.g = hsv.hsv.v; rgb.b = t;
			break;
		case 3:
			rgb.r = p; rgb.g = q; rgb.b = hsv.hsv.v;
			break;
		case 4:
			rgb.r = t; rgb.g = p; rgb.b = hsv.hsv.v;
			break;
		default:
			rgb.r = hsv.hsv.v; rgb.g = p; rgb.b = q;
			break;
	}

	return rgb;
}

Colorf Color::toHsl() {
	Colorf hsl;
	float r = this->r / 255.f;
	float g = this->g / 255.f;
	float b = this->b / 255.f;
	float a = this->a / 255.f;
	float max = eemax(r, eemax( g, b ) );
	float min = eemin(r, eemin( g, b ) );
	float h, s, l = (max + min) / 2.f;

	if ( max == min ) {
		h = s = 0; // achromatic
	} else {
		float d = max - min;

		s = l > 0.5f ? d / (2.f - max - min) : d / (max + min);

		if ( r > g && r > b ) {
			h = (g - b) / d + (g < b ? 6.f : 0.f);
		} else if ( g > b ) {
			h = (b - r) / d + 2.f;
		} else {
			h = (r - g) / d + 4.f;
		}

		h /= 6.f;
	}

	hsl.hsl.h = h;
	hsl.hsl.s = s;
	hsl.hsl.l = l;
	hsl.hsl.a = a;

	return hsl;
}

std::string Color::toHexString() const {
	std::stringstream stream;
	stream << "#" << std::setfill ('0') << std::setw(sizeof(Color)*2) << std::hex << getValue();
	return stream.str();
}

static Float hue2rgb( Float p, Float q, Float t) {
	if(t < 0.f) t += 1.f;
	if(t > 1.f) t -= 1.f;
	if(t < 1.f/6.f) return p + (q - p) * 6.f * t;
	if(t < 1.f/2.f) return q;
	if(t < 2.f/3.f) return p + (q - p) * (2.f/3.f - t) * 6.f;
	return p;
}

Color Color::fromHsl( const Colorf& hsl ) {
	Color rgba;

	if( hsl.hsl.s == 0  ){
		rgba.r = rgba.g = rgba.b = (Uint8)Math::round( hsl.hsl.l ); // achromatic
	} else {
		Float q = hsl.hsl.l < 0.5f ? hsl.hsl.l * (1.f + hsl.hsl.s) : hsl.hsl.l + hsl.hsl.s - hsl.hsl.l * hsl.hsl.s;
		Float p = 2.f * hsl.hsl.l - q;
		rgba.r = hue2rgb(p, q, hsl.hsl.h + 1.f/3.f);
		rgba.g = hue2rgb(p, q, hsl.hsl.h);
		rgba.b = hue2rgb(p, q, hsl.hsl.h - 1.f/3.f);
	}

	return Color( (Uint8)Math::round(rgba.r * 255.f), (Uint8)Math::round(rgba.g * 255.f), (Uint8)Math::round(rgba.b * 255.f), Math::round( hsl.hsl.a * 255.f ) );
}

Color Color::fromPointer( void *ptr ) {
	unsigned long val = (long)ptr;

	// hash the pointer up nicely
	val = (val+0x7ed55d16) + (val<<12);
	val = (val^0xc761c23c) ^ (val>>19);
	val = (val+0x165667b1) + (val<<5);
	val = (val+0xd3a2646c) ^ (val<<9);
	val = (val+0xfd7046c5) + (val<<3);
	val = (val^0xb55a4f09) ^ (val>>16);

	unsigned char r = (val>>0) & 0xFF;
	unsigned char g = (val>>8) & 0xFF;
	unsigned char b = (val>>16) & 0xFF;

	unsigned char max = r>g ? (r>b ? r : b) : (g>b ? g : b);

	const int mult = 127;
	const int add = 63;
	r = (r*mult)/max + add;
	g = (g*mult)/max + add;
	b = (b*mult)/max + add;

	return Color(r, g, b, 255);
}

Color Color::fromString( const char * str ) {
	return fromString( std::string( str ) );
}

Color Color::fromString( std::string str ) {
	std::size_t size = str.size();

	if ( 0 == size )
		return Color::White;

	if ( str[0] == '#' ) {
		str = str.substr(1);

		size = str.size();

		if ( 0 == size )
			return Color::White;
	} else if ( size >= 3 && isalpha( str[0] ) && isalpha( str[1] ) && isalpha( str[2] ) ) {
		String::toLowerInPlace( str );
		if ( "transparent" == str )			return Color::Transparent;
		else if ( "black" == str )			return Color::Black;
		else if ( "silver" == str )			return Color::Silver;
		else if ( "gray" == str )			return Color::Gray;
		else if ( "white" == str )			return Color::White;
		else if ( "maroon" == str )			return Color::Maroon;
		else if ( "red" == str )			return Color::Red;
		else if ( "purple" == str )			return Color::Purple;
		else if ( "fuchsia" == str )		return Color::Fuchsia;
		else if ( "green" == str )			return Color::Green;
		else if ( "lime" == str )			return Color::Lime;
		else if ( "olive" == str )			return Color::Olive;
		else if ( "yellow" == str )			return Color::Yellow;
		else if ( "navy" == str )			return Color::Navy;
		else if ( "blue" == str )			return Color::Blue;
		else if ( "teal" == str )			return Color::Teal;
		else if ( "aqua" == str )			return Color::Aqua;
	}

	if ( size < 6 ) {
		for ( std::size_t i = size; i < 6; i++ )
			str += str[ size - 1 ];

		size = 6;
	}

	if ( 6 == size )
		str += "FF";

	return Color( std::strtoul( str.c_str(), NULL, 16 ) );
}

bool Color::isColorString( std::string str ) {
	if ( str.empty() )
		return false;

	if (  str[0] == '#' )
		return true;

	String::toLowerInPlace( str );
	if ( "transparent" == str )			return true;
	else if ( "black" == str )			return true;
	else if ( "silver" == str )			return true;
	else if ( "gray" == str )			return true;
	else if ( "white" == str )			return true;
	else if ( "maroon" == str )			return true;
	else if ( "red" == str )			return true;
	else if ( "purple" == str )			return true;
	else if ( "fuchsia" == str )		return true;
	else if ( "green" == str )			return true;
	else if ( "lime" == str )			return true;
	else if ( "olive" == str )			return true;
	else if ( "yellow" == str )			return true;
	else if ( "navy" == str )			return true;
	else if ( "blue" == str )			return true;
	else if ( "teal" == str )			return true;
	else if ( "aqua" == str )			return true;

	return false;
}

}}
