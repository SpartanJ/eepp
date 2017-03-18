#ifndef EE_SYSTEMCCOLORS_H
#define EE_SYSTEMCCOLORS_H

#include <eepp/config.hpp>
#include <eepp/system/bitop.hpp>
#include <string>

namespace EE { namespace System {

/** @brief Template class for a RGB color */
template<typename T>
class tColor {
	public:
		T r;
		T g;
		T b;

		tColor() :
			r(255),
			g(255),
			b(255)
		{
		}

		/** Creates an RGB color from each component.
		**	@param r Red component
		**	@param g Green component
		**	@param b Blue component
		*/
		tColor(T r, T g, T b) :
			r(r),
			g(g),
			b(b)
		{
		}

		/** From 32 bits value with RGB(A) byte order */
		tColor( Uint32 Col )
		{
			Col		= BitOp::swapLE32( Col );
			r	= static_cast<T>( Col >> 16	);
			g	= static_cast<T>( Col >> 8	);
			b	= static_cast<T>( Col >> 0	);
		}

		bool operator==( const tColor<T>& Col ) {
			return ( r == Col.r && g == Col.g && b == Col.b );
		}

		bool operator!=( const tColor<T>& Col ) {
			return !( r == Col.r && g == Col.g && b == Col.b );
		}
};

/** @brief Template class for a RGBA color */
template<typename T>
class tColorA {
	public:
		union {
			Uint32 Value;

			struct
			{
				T r;
				T g;
				T b;
				T a;	//! Alpha color component ( transparency )
			};
		};

		tColorA() :
			r(255),
			g(255),
			b(255),
			a(255)
		{
		}

		/** Creates an RGBA color from each component.
		**	@param r Red component
		**	@param g Green component
		**	@param b Blue component
		**  @param a Alpha component
		*/
		tColorA(T r, T g, T b, T a) :
			r(r),
			g(g),
			b(b),
			a(a)
		{
		}

		/** @brief Creates a RGBA color from a RGB color, the Alpha component is set as non-transparent. */
		tColorA( const tColor<T>& Col ) :
			r( Col.r ),
			g( Col.g ),
			b( Col.b ),
			a( 255 )
		{
		}

		/** @brief Creates a RGBA color from a RGB color.
		**	@param Col The RGB color
		**	@param a The Alpha component value
		*/
		tColorA( const tColor<T>& Col, T a ) :
			r( Col.r ),
			g( Col.g ),
			b( Col.b ),
			a( a )
		{
		}

		tColorA( const tColorA<T>& Col ) :
			Value( Col.Value )
		{
		}

		/** From a 32 bits value with RGBA byte order */
		tColorA( const Uint32& Col ) :
			Value( BitOp::swapBE32( Col ) )
		{
		}

		 //! @return The color represented as an Uint32 ( as 0xAABBGGRR for Little Endian )
		Uint32 getValue() const {
			return Value;
		}

		/** @brief Assign the RGBA colors, from each component. */
		void assign( T r, T g, T b, T a ) {
			this->r = r; this->g = g; this->b = b; this->a = a;
		}

		/** @brief Assign the color value from other RGBA color. */
		void assign( const tColorA<T>& Col ) {
			Value = Col.Value;
		}

		bool operator==( const tColorA<T>& Col ) const {
			return ( r == Col.r && g == Col.g && b == Col.b && a == Col.a );
		}

		bool operator!=( const tColorA<T>& Col ) const {
			return !(*this == Col);
		}

		tColorA<T> operator+( const tColorA<T>& Col ) const {
			return tColorA<T>(	eemin( this->r	+ Col.r	, 255 ),
								eemin( this->g	+ Col.g	, 255 ),
								eemin( this->b	+ Col.b	, 255 ),
								eemin( this->a	+ Col.a	, 255 )
			);
		}

		tColorA<T> operator-( const tColorA<T>& Col ) const {
			return tColorA<T>(	eemax( this->r	- Col.r	, 0 ),
								eemax( this->g	- Col.g	, 0 ),
								eemax( this->b	- Col.b	, 0 ),
								eemax( this->a	- Col.a	, 0 )
			);
		}

		tColorA<T> operator*( const tColorA<T>& Col ) const {
			return tColorA<T>(	( this->r		* Col.r	/ 255 ),
								( this->g	* Col.g	/ 255 ),
								( this->b	* Col.b	/ 255 ),
								( this->a	* Col.a	/ 255 )
			);
		}

		tColor<T> toColor() {
			return tColor<T>( r, g, b );
		}
};

class EE_API ColorA : public tColorA<Uint8>
{
	public:
		ColorA();

		ColorA( Uint8 r, Uint8 g, Uint8 b, Uint8 a );

		ColorA( const tColor<Uint8>& Col );

		ColorA( const tColor<Uint8>& Col, Uint8 a );

		ColorA( const tColorA<Uint8>& Col );

		ColorA( const Uint32& Col );

		static ColorA colorFromPointer( void *ptr );

		static ColorA fromString( const char * str );

		static ColorA fromString( const std::string& str );

		static const ColorA Transparent;
		static const ColorA White;
		static const ColorA Black;
};

typedef tColor<Float>		Colorf;
typedef tColorA<Float>		ColorAf;
typedef tColorA<float>		ColorAff;

//! @brief Small class to help in some color operations
class EE_API Color : public tColor<Uint8> {
public:
	Color();

	/** Creates an RGB color from each component.
	**	@param r Red component
	**	@param g Green component
	**	@param b Blue component
	*/
	Color(Uint8 r, Uint8 g, Uint8 b);

	Color( const tColor<Uint8>& color );

	Color( Uint32 Col );

	/** Blend a source color to destination color */
	static ColorAf blend( ColorAf srcf, ColorAf dstf );

	/** Blend a source color to destination color */
	static ColorA blend( ColorA src, ColorA dst );

	static const Color White;
	static const Color Black;
	static const Color Red;
	static const Color Green;
	static const Color Blue;
	static const Color Yellow;
	static const Color Cyan;
	static const Color Magenta;
	static const Color Silver;
	static const Color Gray;
	static const Color Maroon;
	static const Color Olive;
	static const Color OfficeGreen;
	static const Color Purple;
	static const Color Teal;
	static const Color Navy;
};

typedef Color				RGB;
typedef Colorf				RGBf;
typedef ColorA				RGBA;
typedef ColorAf				RGBAf;

}}

#endif
