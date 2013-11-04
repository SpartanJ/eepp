#ifndef EE_SYSTEMCCOLORS_H
#define EE_SYSTEMCCOLORS_H

#include <eepp/declares.hpp>
#include <eepp/system/bitop.hpp>

namespace EE { namespace System {

/** @brief Template class for a RGB color */
template<typename T>
class tColor {
	public:
		T Red;
		T Green;
		T Blue;

		tColor() :
			Red(255),
			Green(255),
			Blue(255)
		{
		}

		/** Creates an RGB color from each component.
		**	@param r Red component
		**	@param g Green component
		**	@param b Blue component
		*/
		tColor(T r, T g, T b) :
			Red(r),
			Green(g),
			Blue(b)
		{
		}

		/** From 32 bits value with RGB(A) byte order */
		tColor( Uint32 Col )
		{
			Col		= BitOp::SwapLE32( Col );
			Red		= static_cast<T>( Col >> 16	);
			Green	= static_cast<T>( Col >> 8	);
			Blue	= static_cast<T>( Col >> 0	);
		}

		T R() const { return Red;	}	//! @return the Red component
		T G() const { return Green;	}	//! @return the Green component
		T B() const { return Blue;	}	//! @return the Blue component

		bool operator==(tColor<T>& Col) {
			return ( Red == Col.R() && Green == Col.G() && Blue == Col.B() );
		}

		bool operator!=(tColor<T>& Col) {
			return !( Red == Col.R() && Green == Col.G() && Blue == Col.B() );
		}
};

/** @brief Template class for a RGBA color */
template<typename T>
class tColorA {
	public:
		static const tColorA<T> Transparent;	///< Transparent predefined color
		static const tColorA<T> Black;			///< Black predefined color

		union {
			Uint32 Value;

			struct
			{
				T Red;
				T Green;
				T Blue;
				T Alpha;	//! Alpha color component ( transparency )
			};
		};

		tColorA() :
			Red(255),
			Green(255),
			Blue(255),
			Alpha(255)
		{
		}

		/** Creates an RGBA color from each component.
		**	@param r Red component
		**	@param g Green component
		**	@param b Blue component
		**  @param a Alpha component
		*/
		tColorA(T r, T g, T b, T a) :
			Red(r),
			Green(g),
			Blue(b),
			Alpha(a)
		{
		}

		/** @brief Creates a RGBA color from a RGB color, the Alpha component is set as non-transparent. */
		tColorA( const tColor<T>& Col ) :
			Red( Col.Red ),
			Green( Col.Green ),
			Blue( Col.Blue ),
			Alpha( 255 )
		{
		}

		/** @brief Creates a RGBA color from a RGB color.
		**	@param Col The RGB color
		**	@param a The Alpha component value
		*/
		tColorA( const tColor<T>& Col, T a ) :
			Red( Col.Red ),
			Green( Col.Green ),
			Blue( Col.Blue ),
			Alpha( a )
		{
		}

		tColorA( const tColorA<T>& Col ) :
			Value( Col.Value )
		{
		}

		/** From a 32 bits value with RGBA byte order */
		tColorA( const Uint32& Col ) :
			Value( BitOp::SwapBE32( Col ) )
		{
		}

		T R() const { return Red;	}	//! @return the Red component
		T G() const { return Green;	}	//! @return the Green component
		T B() const { return Blue;	}	//! @return the Blue component
		T A() const { return Alpha;	}	//! @return the Alpha component

		 //! @return The color represented as an Uint32 ( as 0xAABBGGRR for Little Endian )
		Uint32 GetValue() const {
			return Value;
		}

		/** @brief Assign the RGBA colors, from each component. */
		void Assign( T r, T g, T b, T a ) {
			Red = r; Green = g; Blue = b; Alpha = a;
		}

		/** @brief Assign the color value from other RGBA color. */
		void Assign( const tColorA<T>& Col ) {
			Value = Col.Value;
		}

		bool operator==( const tColorA<T>& Col ) const {
			return ( Red == Col.Red && Green == Col.Green && Blue == Col.Blue && Alpha == Col.Alpha );
		}

		bool operator!=( const tColorA<T>& Col ) const {
			return !(*this == Col);
		}

		tColorA<T> operator+( const tColorA<T>& Col ) const {
			return tColorA<T>(	eemin( this->Red	+ Col.Red	, 255 ),
								eemin( this->Green	+ Col.Green	, 255 ),
								eemin( this->Blue	+ Col.Blue	, 255 ),
								eemin( this->Alpha	+ Col.Alpha	, 255 )
			);
		}

		tColorA<T> operator-( const tColorA<T>& Col ) const {
			return tColorA<T>(	eemax( this->Red	- Col.Red	, 0 ),
								eemax( this->Green	- Col.Green	, 0 ),
								eemax( this->Blue	- Col.Blue	, 0 ),
								eemax( this->Alpha	- Col.Alpha	, 0 )
			);
		}

		tColorA<T> operator*( const tColorA<T>& Col ) const {
			return tColorA<T>(	( this->Red		* Col.Red	/ 255 ),
								( this->Green	* Col.Green	/ 255 ),
								( this->Blue	* Col.Blue	/ 255 ),
								( this->Alpha	* Col.Alpha	/ 255 )
			);
		}

		tColor<T> ToColor() {
			return tColor<T>( Red, Green, Blue );
		}
};

template <typename T>
const tColorA<T> tColorA<T>::Transparent = tColorA<T>(0,0,0,0);

template <typename T>
const tColorA<T> tColorA<T>::Black = tColorA<T>(0,0,0,255);

typedef tColor<Uint8> 		eeColor;
typedef tColor<eeFloat> 	eeColorf;
typedef tColorA<Uint8> 		eeColorA;
typedef tColorA<eeFloat> 	eeColorAf;
typedef tColorA<float>		eeColorAff;

//! @brief Small class to help in some color operations
class Color {
public:
	/** Blend a source color to destination color */
	static inline eeColorAf Blend( eeColorAf srcf, eeColorAf dstf ) {
		eeFloat alpha	= srcf.Alpha + dstf.Alpha * ( 1.f - srcf.Alpha );
		eeFloat red		= ( srcf.Red	* srcf.Alpha + dstf.Red		* dstf.Alpha * ( 1.f - srcf.Alpha ) ) / alpha;
		eeFloat green	= ( srcf.Green	* srcf.Alpha + dstf.Green	* dstf.Alpha * ( 1.f - srcf.Alpha ) ) / alpha;
		eeFloat blue	= ( srcf.Blue	* srcf.Alpha + dstf.Blue	* dstf.Alpha * ( 1.f - srcf.Alpha ) ) / alpha;

		return eeColorAf( red, green, blue, alpha );
	}

	#define EE_COLOR_BLEND_FTOU8(color) (Uint8)( color == 1.f ? 255 : (color * 255.99f))

	/** Blend a source color to destination color */
	static inline eeColorA Blend( eeColorA src, eeColorA dst ) {
		eeColorAf srcf( (eeFloat)src.Red / 255.f, (eeFloat)src.Green / 255.f, (eeFloat)src.Blue / 255.f, (eeFloat)src.Alpha / 255.f );
		eeColorAf dstf( (eeFloat)dst.Red / 255.f, (eeFloat)dst.Green / 255.f, (eeFloat)dst.Blue / 255.f, (eeFloat)dst.Alpha / 255.f );
		eeFloat alpha	= srcf.Alpha + dstf.Alpha * ( 1.f - srcf.Alpha );
		eeFloat red		= ( srcf.Red	* srcf.Alpha + dstf.Red		* dstf.Alpha * ( 1.f - srcf.Alpha ) ) / alpha;
		eeFloat green	= ( srcf.Green	* srcf.Alpha + dstf.Green	* dstf.Alpha * ( 1.f - srcf.Alpha ) ) / alpha;
		eeFloat blue	= ( srcf.Blue	* srcf.Alpha + dstf.Blue	* dstf.Alpha * ( 1.f - srcf.Alpha ) ) / alpha;

		return eeColorA( EE_COLOR_BLEND_FTOU8(red), EE_COLOR_BLEND_FTOU8(green), EE_COLOR_BLEND_FTOU8(blue), EE_COLOR_BLEND_FTOU8(alpha) );
	}
};


}}

#endif
