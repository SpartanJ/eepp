#ifndef EE_SYSTEMCCOLORS_H
#define EE_SYSTEMCCOLORS_H

#include <eepp/declares.hpp>

namespace EE { namespace System {

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

		tColor(T r, T g, T b) :
			Red(r),
			Green(g),
			Blue(b)
		{
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

		tColorA(T r, T g, T b, T a) :
			Red(r),
			Green(g),
			Blue(b),
			Alpha(a)
		{
		}

		tColorA( const tColor<T>& Col ) :
			Red( Col.Red ),
			Green( Col.Green ),
			Blue( Col.Blue ),
			Alpha( 255 )
		{
		}

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

		/** ARGB format */
		tColorA( const Uint32& Col ) :
			Value( Col )
		{
		}

		T R() const { return Red;	}	//! @return the Red component
		T G() const { return Green;	}	//! @return the Green component
		T B() const { return Blue;	}	//! @return the Blue component
		T A() const { return Alpha;	}	//! @return the Alpha component

		 //! The color represented as an Uint32
		Uint32 GetValue() const {
			return Value;
		}

		void Assign( T r, T g, T b, T a ) {
			Red = r; Green = g; Blue = b; Alpha = a;
		}

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

}}

#endif
