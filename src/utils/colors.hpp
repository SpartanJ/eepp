#ifndef EE_UTILSCCOLORS_H
#define EE_UTILSCCOLORS_H

namespace EE { namespace Utils {

template <typename T>
class tColor {
	public:
		T Red;
		T Green;
		T Blue;

		tColor();
		tColor(T r, T g, T b);

		T R() const;
		T G() const;
		T B() const;
		
		bool operator==(tColor<T>& Col);
		bool operator!=(tColor<T>& Col);

		static const tColor<T> Black;   ///< Black predefined color
};

template <typename T>
tColor<T>::tColor() : Red(255), Green(255), Blue(255) {}

template <typename T>
tColor<T>::tColor(T r, T g, T b) {
	Red = r; Green = g; Blue = b;
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
		using tColor<T>::Red;
		using tColor<T>::Green;
		using tColor<T>::Blue;
		T Alpha;

		tColorA();
		tColorA(T r, T g, T b, T a);
		tColorA( const tColor<T>& Col );
		
		/** ARGB format */
		tColorA( const Uint32& Col );
		
		T A() const;

		bool operator==( const tColorA<T>& Col ) const;
		bool operator!=( const tColorA<T>& Col ) const;

		static const tColorA<T> Black;   ///< Black predefined color
		
		Uint32 GetUint32();
};

template <typename T>
tColorA<T>::tColorA( const Uint32& Col )
{
	Alpha = Red = Green = Blue = 0;
	Alpha	|= Col >> 24;
	Red		|= Col >> 16;
	Green	|= Col >> 8;
	Blue	|= Col;
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
tColorA<T>::tColorA(T r, T g, T b, T a) : tColor<T>(r, g, b) {
	Alpha = a;
}

template <typename T>
tColorA<T>::tColorA( const tColor<T>& Col ) : tColor<T>( Col.R(), Col.G(), Col.B() ), Alpha(255) {}

template <typename T>
const tColorA<T> tColorA<T>::Black = tColorA<T>(0,0,0,0);

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

template <typename T>
class cColor : public tColor<T> {
	public:
		using tColor<T>::Red;
		using tColor<T>::Green;
		using tColor<T>::Blue;
		bool voidRGB;
		
		cColor();
		cColor( T r, T g, T b );
		cColor( bool voidrgb );
		cColor( const tColor<T>& Col );
		
		bool operator==(cColor<T>& Col);
		bool operator!=(cColor<T>& Col);
};

template <typename T>
cColor<T>::cColor( bool voidrgb ) : tColor<T>(), voidRGB(voidrgb) {}

template <typename T>
cColor<T>::cColor() : tColor<T>(), voidRGB(false) {}

template <typename T>
cColor<T>::cColor(T r, T g, T b) : tColor<T>(r, g, b), voidRGB(false) {}

template <typename T>
cColor<T>::cColor( const tColor<T>& Col ) {
	Red = Col.R(); Green = Col.G(); Blue = Col.B(); voidRGB = false;
}

template <typename T>
bool cColor<T>::operator== (cColor<T>& Col) {
	return ( Red == Col.R() && Green == Col.G() && Blue == Col.B() );
}

template <typename T>
bool cColor<T>::operator!= (cColor<T>& Col) {
	return !( Red == Col.R() && Green == Col.G() && Blue == Col.B() );
}

template <typename T>
class cColorA : public tColorA<T> {
	public:
		using tColorA<T>::Red;
		using tColorA<T>::Green;
		using tColorA<T>::Blue;
		using tColorA<T>::Alpha;
		bool voidRGB;

		cColorA();
		cColorA( T r, T g, T b, T a) ;
		cColorA( const tColor<T>& Col );
		cColorA( const tColorA<T>& Col );
		cColorA( const bool& voidrgb );
		cColorA( const Uint32& Color );
		
		bool operator==( cColorA<T>& Col );
		bool operator!=( cColorA<T>& Col );
		
		static const cColorA<T> Black;   ///< Black predefined color
};

template <typename T>
const cColorA<T> cColorA<T>::Black = cColorA<T>(0,0,0,0);

template <typename T>
cColorA<T>::cColorA( const Uint32& Color ) : tColorA<T>( Color ) {
	voidRGB = false;
}

template <typename T>
cColorA<T>::cColorA( const bool& voidrgb ) : tColorA<T>() {
	voidRGB = voidrgb;
}

template <typename T>
cColorA<T>::cColorA() : tColorA<T>(), voidRGB(false) {}

template <typename T>
cColorA<T>::cColorA(T r, T g, T b, T a) : tColorA<T>(r, g, b, a), voidRGB(false) {}

template <typename T>
cColorA<T>::cColorA( const tColor<T>& Col ) {
	Red = Col.R(); Green = Col.G(); Blue = Col.B(); Alpha = 255;
}

template <typename T>
cColorA<T>::cColorA( const tColorA<T>& Col ) {
	Red = Col.R(); Green = Col.G(); Blue = Col.B(); Alpha = Col.A();
}

template <typename T>
bool cColorA<T>::operator== (cColorA<T>& Col) {
	return ( Red == Col.R() && Green == Col.G() && Blue == Col.B() && Alpha == Col.A() );
}

template <typename T>
bool cColorA<T>::operator!= (cColorA<T>& Col) {
	return !( Red == Col.R() && Green == Col.G() && Blue == Col.B() && Alpha == Col.A() );
}

typedef tColor<Uint8> 		eeColor;
typedef tColor<eeFloat> 	eeColorf;
typedef tColorA<Uint8> 		eeColorA;
typedef tColorA<eeFloat> 	eeColorAf;
typedef cColor<Uint8> 		eeRGB;
typedef cColorA<Uint8> 		eeRGBA;

}}

#endif
