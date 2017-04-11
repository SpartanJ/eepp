#include <eepp/system/color.hpp>
#include <eepp/core/string.hpp>
#include <cstdlib>

namespace EE { namespace System {

const Color Color::Transparent = Color(0,0,0,0);
const Color Color::White = Color(255,255,255);
const Color Color::Black = Color(0,0,0);
const Color Color::Red = Color(255,0,0);
const Color Color::Green = Color(0,255,0);
const Color Color::Blue = Color(0,0,255);
const Color Color::Yellow = Color(255,255,0);
const Color Color::Cyan = Color(0,255,255);
const Color Color::Magenta = Color(255,0,255);
const Color Color::Silver = Color(192,0,192);
const Color Color::Gray = Color(128,128,128);
const Color Color::Maroon = Color(128,0,0);
const Color Color::Olive = Color(128,128,0);
const Color Color::OfficeGreen = Color(0,128,0);
const Color Color::Purple = Color(128,0,128);
const Color Color::Teal = Color(0,128,128);
const Color Color::Navy = Color(0,0,128);

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
{}

Color::Color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) :
	tColor<Uint8>(r,g,b,a)
{}

Color::Color( const tRGB<Uint8>& Col ) :
	tColor<Uint8>( Col )
{}

Color::Color( const tRGB<Uint8>& Col, Uint8 a ) :
	tColor<Uint8>( Col, a )
{}

Color::Color( const tColor<Uint8>& Col ) :
	tColor<Uint8>( Col.Value )
{}

Color::Color( const Uint32& Col ) :
	tColor<Uint8>( Col )
{}

Color Color::colorFromPointer( void *ptr ) {
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
		else if ( "white" == str )			return Color::White;
		else if ( "black" == str )			return Color::Black;
		else if ( "red" == str )			return Color::Red;
		else if ( "green" == str )			return Color::Green;
		else if ( "blue" == str )			return Color::Blue;
		else if ( "yellow" == str )			return Color::Yellow;
		else if ( "cyan" == str )			return Color::Cyan;
		else if ( "magenta" == str )		return Color::Magenta;
		else if ( "silver" == str )			return Color::Silver;
		else if ( "gray" == str )			return Color::Gray;
		else if ( "maroon" == str )			return Color::Maroon;
		else if ( "olive" == str )			return Color::Olive;
		else if ( "officegreen" == str )	return Color::OfficeGreen;
		else if (  "purple" == str )		return Color::Purple;
		else if ( "teal" == str )			return Color::Teal;
		else if ( "navy" == str )			return Color::Navy;
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

}}
