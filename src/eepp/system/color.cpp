#include <eepp/system/color.hpp>
#include <cstdlib>

namespace EE { namespace System {

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

const ColorA ColorA::Transparent = ColorA(0,0,0,0);
const ColorA ColorA::White = ColorA(255,255,255,255);
const ColorA ColorA::Black = ColorA(0,0,0,255);

Color::Color() : tColor<Uint8>()
{
}

/** Creates an RGB color from each component.
**	@param r Red component
**	@param g Green component
**	@param b Blue component
*/
Color::Color( Uint8 r, Uint8 g, Uint8 b ) : tColor<Uint8>( r, g, b )
{
}

Color::Color( const tColor<Uint8>& color ) :
	tColor<Uint8>( color.r, color.g, color.b )
{
}

Color::Color( Uint32 Col )
{
	Col	= BitOp::swapLE32( Col );
	r	= static_cast<Uint8>( Col >> 16	);
	g	= static_cast<Uint8>( Col >> 8	);
	b	= static_cast<Uint8>( Col >> 0	);
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
ColorA Color::blend( ColorA src, ColorA dst ) {
	ColorAf srcf( (Float)src.r / 255.f, (Float)src.g / 255.f, (Float)src.b / 255.f, (Float)src.a / 255.f );
	ColorAf dstf( (Float)dst.r / 255.f, (Float)dst.g / 255.f, (Float)dst.b / 255.f, (Float)dst.a / 255.f );
	Float alpha	= srcf.a + dstf.a * ( 1.f - srcf.a );
	Float red	= ( srcf.r	* srcf.a + dstf.r		* dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float green	= ( srcf.g	* srcf.a + dstf.g	* dstf.a * ( 1.f - srcf.a ) ) / alpha;
	Float blue	= ( srcf.b	* srcf.a + dstf.b	* dstf.a * ( 1.f - srcf.a ) ) / alpha;

	return ColorA( EE_COLOR_BLEND_FTOU8(red), EE_COLOR_BLEND_FTOU8(green), EE_COLOR_BLEND_FTOU8(blue), EE_COLOR_BLEND_FTOU8(alpha) );
}

ColorA::ColorA() :
	tColorA<Uint8>()
{}

ColorA::ColorA(Uint8 r, Uint8 g, Uint8 b, Uint8 a) :
	tColorA<Uint8>(r,g,b,a)
{}

ColorA::ColorA( const tColor<Uint8>& Col ) :
	tColorA<Uint8>( Col )
{}

ColorA::ColorA( const tColor<Uint8>& Col, Uint8 a ) :
	tColorA<Uint8>( Col, a )
{}

ColorA::ColorA( const tColorA<Uint8>& Col ) :
	tColorA<Uint8>( Col.Value )
{}

ColorA::ColorA( const Uint32& Col ) :
	tColorA<Uint8>( Col )
{}

ColorA ColorA::colorFromPointer( void *ptr ) {
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

	return ColorA(r, g, b, 255);
}

ColorA ColorA::fromString( const char * str ) {
	return ColorA( std::strtoul( str, NULL, 16 ) );
}

ColorA ColorA::fromString( const std::string& str ) {
	return ColorA( std::strtoul( str.c_str(), NULL, 16 ) );
}

}}
