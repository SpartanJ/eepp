#ifndef EE_SYSTEMCCOLORS_H
#define EE_SYSTEMCCOLORS_H

#include <eepp/config.hpp>
#include <eepp/system/bitop.hpp>
#include <string>
#include <map>
#if EE_PLATFORM == EE_PLATFORM_WIN
#undef RGB
#endif

namespace EE { namespace System {

/** @brief Template class for a RGB color */
template<typename T>
class tRGB {
	public:
		T r;
		T g;
		T b;

		tRGB() :
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
		tRGB(T r, T g, T b) :
			r(r),
			g(g),
			b(b)
		{
		}

		/** From 32 bits value with RGB(A) byte order */
		tRGB( Uint32 Col )
		{
			Col		= BitOp::swapLE32( Col );
			r	= static_cast<T>( Col >> 16	);
			g	= static_cast<T>( Col >> 8	);
			b	= static_cast<T>( Col >> 0	);
		}

		bool operator==( const tRGB<T>& Col ) {
			return ( r == Col.r && g == Col.g && b == Col.b );
		}

		bool operator!=( const tRGB<T>& Col ) {
			return !( r == Col.r && g == Col.g && b == Col.b );
		}
};

/** @brief Template class for a RGBA color */
template<typename T>
class tColor {
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

			struct
			{
				T h;
				T s;
				T v;
				T a;
			} hsv;

			struct
			{
				T h;
				T s;
				T l;
				T a;
			} hsl;
		};

		tColor() :
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
		tColor(T r, T g, T b, T a) :
			r(r),
			g(g),
			b(b),
			a(a)
		{
		}

		/** @brief Creates a RGBA color from a RGB color, the Alpha component is set as non-transparent. */
		tColor( const tRGB<T>& Col ) :
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
		tColor( const tRGB<T>& Col, T a ) :
			r( Col.r ),
			g( Col.g ),
			b( Col.b ),
			a( a )
		{
		}

		tColor( const tColor<T>& Col ) :
			Value( Col.Value )
		{
		}

		/** From a 32 bits value with RGBA byte order */
		tColor( const Uint32& Col ) :
			Value( BitOp::swapBE32( Col ) )
		{
		}

		 //! @return The color represented as an Uint32 ( as 0xRRGGBBAA for Little Endian )
		Uint32 getValue() const {
			return BitOp::swapBE32( Value );
		}

		/** @brief Assign the RGBA colors, from each component. */
		void assign( T r, T g, T b, T a ) {
			this->r = r; this->g = g; this->b = b; this->a = a;
		}

		/** @brief Assign the color value from other RGBA color. */
		void assign( const tColor<T>& Col ) {
			Value = Col.Value;
		}

		bool operator==( const tColor<T>& Col ) const {
			return ( r == Col.r && g == Col.g && b == Col.b && a == Col.a );
		}

		bool operator!=( const tColor<T>& Col ) const {
			return !(*this == Col);
		}

		tColor<T> operator+( const tColor<T>& Col ) const {
			return tColor<T>(	eemin( this->r	+ Col.r	, 255 ),
								eemin( this->g	+ Col.g	, 255 ),
								eemin( this->b	+ Col.b	, 255 ),
								eemin( this->a	+ Col.a	, 255 )
			);
		}

		tColor<T> operator-( const tColor<T>& Col ) const {
			return tColor<T>(	eemax( this->r	- Col.r	, 0 ),
								eemax( this->g	- Col.g	, 0 ),
								eemax( this->b	- Col.b	, 0 ),
								eemax( this->a	- Col.a	, 0 )
			);
		}

		tColor<T> operator*( const tColor<T>& Col ) const {
			return tColor<T>(	( this->r		* Col.r	/ 255 ),
								( this->g	* Col.g	/ 255 ),
								( this->b	* Col.b	/ 255 ),
								( this->a	* Col.a	/ 255 )
			);
		}

		tRGB<T> toRGB() {
			return tRGB<T>( r, g, b );
		}
};


typedef tColor<Float> ColorAf;
typedef tColor<Float> Colorf;

class EE_API Color : public tColor<Uint8> {
	public:
		Color();

		Color( std::string colorString );

		Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255 );

		Color( const tRGB<Uint8>& Col );

		Color( const tRGB<Uint8>& Col, Uint8 a );

		Color( const tColor<Uint8>& Col, Uint8 a );

		Color( const tColor<Uint8>& Col );

		Color( const Uint32& Col );

		Color toHsv();

		static Color fromHsv( const Color& hsv );

		Colorf toHsl();

		std::string toHexString() const;

		static Color fromHsl( const Colorf& hsl );

		/** Blend a source color to destination color */
		static ColorAf blend( ColorAf srcf, ColorAf dstf );

		/** Blend a source color to destination color */
		static Color blend( Color src, Color dst );

		static Color fromPointer( void *ptr );

		static Color fromString( const char * str );

		static Color fromString( std::string str );

		static bool isColorString( std::string str );

		static void registerColor( const std::string& name, const Color& color );

		static bool unregisterColor( const std::string& name );

		static const Color Transparent;
		static const Color Black;
		static const Color Silver;
		static const Color Gray;
		static const Color White;
		static const Color Maroon;
		static const Color Red;
		static const Color Purple;
		static const Color Fuchsia;
		static const Color Green;
		static const Color Lime;
		static const Color Olive;
		static const Color Yellow;
		static const Color Navy;
		static const Color Blue;
		static const Color Teal;
		static const Color Aqua;
	private:
		static std::map<std::string, Color> sColors;
};

typedef Color ColorA;
typedef tColor<Float>		ColorAf;

//! @brief Small class to help in some color operations
class EE_API RGB : public tRGB<Uint8> {
	public:
		RGB();

		/** Creates an RGB color from each component.
		**	@param r Red component
		**	@param g Green component
		**	@param b Blue component
		*/
		RGB( Uint8 r, Uint8 g, Uint8 b );

		RGB( const tRGB<Uint8>& color );

		RGB( Uint32 Col );

		Color toColor();
};

class EE_API RectColors {
	public:
		RectColors() :
			TopLeft( Color::White ),
			TopRight( Color::White ),
			BottomLeft( Color::White ),
			BottomRight( Color::White )
		{}

		RectColors( const Color& color ) :
			TopLeft( color ),
			TopRight( color ),
			BottomLeft( color ),
			BottomRight( color )
		{}

		RectColors( const Color& topLeft, const Color& topRight, const Color& bottomLeft, const Color& bottomRight ) :
			TopLeft( topLeft ),
			TopRight( topRight ),
			BottomLeft( bottomLeft ),
			BottomRight( bottomRight )
		{}

		Color TopLeft;
		Color TopRight;
		Color BottomLeft;
		Color BottomRight;
};

}}

#endif
