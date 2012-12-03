#ifndef EE_SYSTEMCCOLORS_H
#define EE_SYSTEMCCOLORS_H

#include <eepp/declares.hpp>

namespace EE { namespace System {

template <typename T>
class tColor {
	public:
		T Red;		//! Red color component
		T Green;	//! Green color component
		T Blue;		//! Blue color component

		tColor();

		tColor(T r, T g, T b);

		/** ARGB ( Alpha ignored ) */
		tColor( const Uint32& Col );

		T R() const;	//! @return the Red component
		T G() const;	//! @return the Green component
		T B() const;	//! @return the Blue component
		
		bool operator==(tColor<T>& Col);
		bool operator!=(tColor<T>& Col);

		static const tColor<T> Black;   ///< Black predefined color
};

template <typename T>
tColor<T>::tColor() :
	Red(255),
	Green(255),
	Blue(255)
{}

template <typename T>
tColor<T>::tColor( const Uint32& Col ) :
	Red(0),
	Green(0),
	Blue(0)
{
	Red		|= Col >> 16;
	Green	|= Col >> 8;
	Blue	|= Col;
}

template <typename T>
tColor<T>::tColor(T r, T g, T b) :
	Red( r ),
	Green( g ),
	Blue( b )
{
}

template <typename T>
const tColor<T> tColor<T>::Black = tColor<T>(0,0,0);

template <typename T>
T tColor<T>::R() const {
	return Red;
}

template <typename T>
T tColor<T>::G() const {
	return Green;
}

template <typename T>
T tColor<T>::B() const {
	return Blue;
}

template <typename T>
bool tColor<T>::operator== (tColor<T>& Col) {
	return ( Red == Col.R() && Green == Col.G() && Blue == Col.B() );
}

template <typename T>
bool tColor<T>::operator!= (tColor<T>& Col) {
	return !( Red == Col.R() && Green == Col.G() && Blue == Col.B() );
}

template <typename T>
class tColorA : public tColor<T> {
	public:
		static const tColorA<T> Transparent;	///< Transparent predefined color
		static const tColorA<T> Black;			///< Black predefined color

		using tColor<T>::Red;	//! Uses Red Green and Blue component from the RGB color
		using tColor<T>::Green;
		using tColor<T>::Blue;

		T Alpha;	//! Alpha color component ( transparency )

		tColorA();

		tColorA(T r, T g, T b, T a);

		tColorA( const tColor<T>& Col );
		
		/** ARGB format */
		tColorA( const Uint32& Col );

		T A() const;	//! @return the Alpha component

		Uint32 GetUint32();	//! The color represented as an Uint32

		void Assign( T r, T g, T b, T a );

		void Assign( const tColorA<T>& Col );

		bool operator==( const tColorA<T>& Col ) const;

		bool operator!=( const tColorA<T>& Col ) const;

		tColorA<T> operator+( const tColorA<T>& Col ) const;

		tColorA<T> operator-( const tColorA<T>& Col ) const;

		tColorA<T> operator*( const tColorA<T>& Col ) const;
};

template <typename T>
tColorA<T> tColorA<T>::operator+( const tColorA<T>& Col ) const
{
	return tColorA<T>(	eemin( this->Red	+ Col.Red	, 255 ),
						eemin( this->Green	+ Col.Green	, 255 ),
						eemin( this->Blue	+ Col.Blue	, 255 ),
						eemin( this->Alpha	+ Col.Alpha	, 255 )
	);
}

template <typename T>
tColorA<T> tColorA<T>::operator-( const tColorA<T>& Col ) const
{
	return tColorA<T>(	eemax( this->Red	- Col.Red	, 0 ),
						eemax( this->Green	- Col.Green	, 0 ),
						eemax( this->Blue	- Col.Blue	, 0 ),
						eemax( this->Alpha	- Col.Alpha	, 0 )
	);
}

template <typename T>
tColorA<T> tColorA<T>::operator*( const tColorA<T>& Col ) const
{
	return tColorA<T>(	( this->Red		* Col.Red	/ 255 ),
						( this->Green	* Col.Green	/ 255 ),
						( this->Blue	* Col.Blue	/ 255 ),
						( this->Alpha	* Col.Alpha	/ 255 )
	);
}

template <typename T>
tColorA<T>::tColorA( const Uint32& Col ) :
	tColor<T>( 0, 0, 0 ),
	Alpha(0)
{
	Alpha	|= Col >> 24;
	Red		|= Col >> 16;
	Green	|= Col >> 8;
	Blue	|= Col;
}

template <typename T>
void tColorA<T>::Assign( T r, T g, T b, T a ) {
	Red = r;
	Green = g;
	Blue = b;
	Alpha = a;
}

template <typename T>
void tColorA<T>::Assign( const tColorA<T>& Col ) {
	Red = Col.Red;
	Green = Col.Green;
	Blue = Col.Blue;
	Alpha = Col.Alpha;
}

template <typename T>
Uint32 tColorA<T>::GetUint32() {
	Uint32 Col = 0;
	
	Col |= Alpha << 24;
	Col |= Red	 << 16;
	Col |= Green << 8;
	Col |= Blue;
	
	return Col;
}

template <typename T>
tColorA<T>::tColorA() : tColor<T>(), Alpha(255) {}

template <typename T>
tColorA<T>::tColorA(T r, T g, T b, T a) :
	tColor<T>(r, g, b),
	Alpha( a )
{
}

template <typename T>
tColorA<T>::tColorA( const tColor<T>& Col ) : tColor<T>( Col.R(), Col.G(), Col.B() ), Alpha(255) {}

template <typename T>
const tColorA<T> tColorA<T>::Transparent = tColorA<T>(0,0,0,0);

template <typename T>
const tColorA<T> tColorA<T>::Black = tColorA<T>(0,0,0,255);

template <typename T>
T tColorA<T>::A() const {
	return Alpha;
}

template <typename T>
bool tColorA<T>::operator== ( const tColorA<T>& Col ) const {
	return ( Red == Col.R() && Green == Col.G() && Blue == Col.B() && Alpha == Col.A() );
}

template <typename T>
bool tColorA<T>::operator!= ( const tColorA<T>& Col ) const {
	return !( Red == Col.R() && Green == Col.G() && Blue == Col.B() && Alpha == Col.A() );
}

typedef tColor<Uint8> 		eeColor;
typedef tColor<eeFloat> 	eeColorf;
typedef tColorA<Uint8> 		eeColorA;
typedef tColorA<eeFloat> 	eeColorAf;

}}

#endif
