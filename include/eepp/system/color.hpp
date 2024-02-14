#ifndef EE_SYSTEMCCOLORS_H
#define EE_SYSTEMCCOLORS_H

#include <eepp/config.hpp>
#include <eepp/core/containers.hpp>
#include <eepp/core/string.hpp>
#include <eepp/system/bitop.hpp>
#include <string>
#if EE_PLATFORM == EE_PLATFORM_WIN
#undef RGB
#endif

namespace EE { namespace System {

/** @brief Template class for a RGB color */
template <typename T> class tRGB {
  public:
	T r;
	T g;
	T b;

	tRGB() : r( 255 ), g( 255 ), b( 255 ) {}

	/** Creates an RGB color from each component.
	**	@param r Red component
	**	@param g Green component
	**	@param b Blue component
	*/
	tRGB( T r, T g, T b ) : r( r ), g( g ), b( b ) {}

	/** From 32 bits value with RGB(A) byte order */
	tRGB( Uint32 Col ) {
		Col = BitOp::swapLE32( Col );
		r = static_cast<T>( Col >> 16 );
		g = static_cast<T>( Col >> 8 );
		b = static_cast<T>( Col >> 0 );
	}

	bool operator==( const tRGB<T>& Col ) { return ( r == Col.r && g == Col.g && b == Col.b ); }

	bool operator!=( const tRGB<T>& Col ) { return !( r == Col.r && g == Col.g && b == Col.b ); }
};

/** @brief Template class for a RGBA color */
template <typename T> class tColor {
  public:
	union {
		Uint32 Value;

		struct {
			T r;
			T g;
			T b;
			T a; //! Alpha color component ( transparency )
		};

		struct {
			T h;
			T s;
			T v;
			T a;
		} hsv;

		struct {
			T h;
			T s;
			T l;
			T a;
		} hsl;
	};

	tColor() : r( 255 ), g( 255 ), b( 255 ), a( 255 ) {}

	/** Creates an RGBA color from each component.
	**	@param r Red component
	**	@param g Green component
	**	@param b Blue component
	**  @param a Alpha component
	*/
	tColor( T r, T g, T b, T a ) : r( r ), g( g ), b( b ), a( a ) {}

	/** @brief Creates a RGBA color from a RGB color, the Alpha component is set as non-transparent.
	 */
	tColor( const tRGB<T>& Col ) : r( Col.r ), g( Col.g ), b( Col.b ), a( 255 ) {}

	/** @brief Creates a RGBA color from a RGB color.
	**	@param Col The RGB color
	**	@param a The Alpha component value
	*/
	tColor( const tRGB<T>& Col, T a ) : r( Col.r ), g( Col.g ), b( Col.b ), a( a ) {}

	tColor( const tColor<T>& Col ) : Value( Col.Value ) {}

	/** From a 32 bits value with RGBA byte order */
	tColor( const Uint32& Col ) : Value( BitOp::swapBE32( Col ) ) {}

	//! @return The color represented as an Uint32 ( as 0xRRGGBBAA for Little Endian )
	Uint32 getValue() const { return BitOp::swapBE32( Value ); }

	/** @brief Assign the RGBA colors, from each component. */
	void assign( T r, T g, T b, T a ) {
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	/** @brief Assign the color value from other RGBA color. */
	void assign( const tColor<T>& Col ) { Value = Col.Value; }

	bool operator==( const tColor<T>& Col ) const {
		return ( r == Col.r && g == Col.g && b == Col.b && a == Col.a );
	}

	bool operator!=( const tColor<T>& Col ) const { return !( *this == Col ); }

	tColor<T> operator+( const tColor<T>& Col ) const {
		return tColor<T>( std::abs( this->r + Col.r ), std::abs( this->g + Col.g ),
						  std::abs( this->b + Col.b ), std::abs( this->a + Col.a ) );
	}

	tColor<T> operator-( const tColor<T>& Col ) const {
		return tColor<T>( std::abs( this->r - Col.r ), std::abs( this->g - Col.g ),
						  std::abs( this->b - Col.b ), std::abs( this->a - Col.a ) );
	}

	tColor<T> operator*( const tColor<T>& Col ) const {
		return tColor<T>( ( this->r * Col.r / 255 ), ( this->g * Col.g / 255 ),
						  ( this->b * Col.b / 255 ), ( this->a * Col.a / 255 ) );
	}

	tRGB<T> toRGB() { return tRGB<T>( r, g, b ); }
};

typedef tColor<Float> ColorAf;
typedef tColor<Float> Colorf;

class EE_API Color : public tColor<Uint8> {
  public:
	Color();

	Color( const Color& color );

	Color( const std::string& colorString );

	Color( Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255 );

	Color( const tRGB<Uint8>& Col );

	Color( const tRGB<Uint8>& Col, Uint8 a );

	Color( const tColor<Uint8>& Col, Uint8 a );

	Color( const tColor<Uint8>& Col );

	Color( const Uint32& Col );

	Color& operator=( const Color& col );

	Colorf toHsv() const;

	static Color fromHsv( const Colorf& hsv );

	Colorf toHsl() const;

	Color clone() const;

	Color invert() const;

	Color div( int divisor, bool divAlpha = false );

	std::string toHexString( const bool& prependHash = true ) const;

	std::string toRgbaString() const;

	std::string toRgbString() const;

	Color& blendAlpha( const Uint8& alpha );

	static Color fromHsl( const Colorf& hsl );

	/** Blend a source color to destination color */
	static ColorAf blend( ColorAf srcf, ColorAf dstf );

	/** Blend a source color to destination color */
	static Color blend( Color src, Color dst );

	static Color fromPointer( void* ptr );

	static Color fromString( const char* str );

	static Color fromString( std::string str );

	static bool isColorString( std::string str );

	static void registerColor( const std::string& name, const Color& color );

	static bool unregisterColor( const std::string& name );

	static bool validHexColorString( const std::string& hexColor );

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

	static const Color aliceblue;
	static const Color antiquewhite;
	static const Color aqua;
	static const Color aquamarine;
	static const Color azure;
	static const Color beige;
	static const Color bisque;
	static const Color black;
	static const Color blanchedalmond;
	static const Color blue;
	static const Color blueviolet;
	static const Color brown;
	static const Color burlywood;
	static const Color cadetblue;
	static const Color chartreuse;
	static const Color chocolate;
	static const Color coral;
	static const Color cornflowerblue;
	static const Color cornsilk;
	static const Color crimson;
	static const Color cyan;
	static const Color darkblue;
	static const Color darkcyan;
	static const Color darkgoldenrod;
	static const Color darkgray;
	static const Color darkgreen;
	static const Color darkgrey;
	static const Color darkkhaki;
	static const Color darkmagenta;
	static const Color darkolivegreen;
	static const Color darkorange;
	static const Color darkorchid;
	static const Color darkred;
	static const Color darksalmon;
	static const Color darkseagreen;
	static const Color darkslateblue;
	static const Color darkslategray;
	static const Color darkslategrey;
	static const Color darkturquoise;
	static const Color darkviolet;
	static const Color deeppink;
	static const Color deepskyblue;
	static const Color dimgray;
	static const Color dimgrey;
	static const Color dodgerblue;
	static const Color firebrick;
	static const Color floralwhite;
	static const Color forestgreen;
	static const Color fuchsia;
	static const Color gainsboro;
	static const Color ghostwhite;
	static const Color gold;
	static const Color goldenrod;
	static const Color gray;
	static const Color green;
	static const Color greenyellow;
	static const Color grey;
	static const Color honeydew;
	static const Color hotpink;
	static const Color indianred;
	static const Color indigo;
	static const Color ivory;
	static const Color khaki;
	static const Color lavender;
	static const Color lavenderblush;
	static const Color lawngreen;
	static const Color lemonchiffon;
	static const Color lightblue;
	static const Color lightcoral;
	static const Color lightcyan;
	static const Color lightgoldenrodyellow;
	static const Color lightgray;
	static const Color lightgreen;
	static const Color lightgrey;
	static const Color lightpink;
	static const Color lightsalmon;
	static const Color lightseagreen;
	static const Color lightskyblue;
	static const Color lightslategray;
	static const Color lightslategrey;
	static const Color lightsteelblue;
	static const Color lightyellow;
	static const Color lime;
	static const Color limegreen;
	static const Color linen;
	static const Color magenta;
	static const Color maroon;
	static const Color mediumaquamarine;
	static const Color mediumblue;
	static const Color mediumorchid;
	static const Color mediumpurple;
	static const Color mediumseagreen;
	static const Color mediumslateblue;
	static const Color mediumspringgreen;
	static const Color mediumturquoise;
	static const Color mediumvioletred;
	static const Color midnightblue;
	static const Color mintcream;
	static const Color mistyrose;
	static const Color moccasin;
	static const Color navajowhite;
	static const Color navy;
	static const Color oldlace;
	static const Color olive;
	static const Color olivedrab;
	static const Color orange;
	static const Color orangered;
	static const Color orchid;
	static const Color palegoldenrod;
	static const Color palegreen;
	static const Color paleturquoise;
	static const Color palevioletred;
	static const Color papayawhip;
	static const Color peachpuff;
	static const Color peru;
	static const Color pink;
	static const Color plum;
	static const Color powderblue;
	static const Color purple;
	static const Color red;
	static const Color rosybrown;
	static const Color royalblue;
	static const Color saddlebrown;
	static const Color salmon;
	static const Color sandybrown;
	static const Color seagreen;
	static const Color seashell;
	static const Color sienna;
	static const Color silver;
	static const Color skyblue;
	static const Color slateblue;
	static const Color slategray;
	static const Color slategrey;
	static const Color snow;
	static const Color springgreen;
	static const Color steelblue;
	static const Color tan;
	static const Color teal;
	static const Color thistle;
	static const Color tomato;
	static const Color turquoise;
	static const Color violet;
	static const Color wheat;
	static const Color white;
	static const Color whitesmoke;
	static const Color yellow;
	static const Color yellowgreen;

  private:
	static UnorderedMap<std::string, Color> sColors;
	static UnorderedMap<String::HashType, Color> sColorMap;

	static void initColorMap();
};

typedef Color ColorA;
typedef tColor<Float> ColorAf;

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
		BottomRight( Color::White ) {}

	RectColors( const Color& color ) :
		TopLeft( color ), TopRight( color ), BottomLeft( color ), BottomRight( color ) {}

	RectColors( const Color& topLeft, const Color& topRight, const Color& bottomLeft,
				const Color& bottomRight ) :
		TopLeft( topLeft ),
		TopRight( topRight ),
		BottomLeft( bottomLeft ),
		BottomRight( bottomRight ) {}

	Color TopLeft;
	Color TopRight;
	Color BottomLeft;
	Color BottomRight;
};

}} // namespace EE::System

#endif
