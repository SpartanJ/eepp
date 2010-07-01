#ifndef EE_UTILSCRECT_H
#define EE_UTILSCRECT_H

#include "vector2.hpp"
#include "size.hpp"

namespace EE { namespace Utils {

template <typename T>
class tRECT {
	public:
		T Left, Right, Top, Bottom;

		tRECT(T left, T top, T right, T bottom);
		tRECT( const Vector2<T>& Pos, const tSize<T>& Size );
		tRECT();

		bool Intersect( const tRECT<T>& RECT );
		bool Contains( const tRECT<T>& RECT );
		bool Contains( const Vector2<T>& Point );

		Vector2<T> Pos();
		tSize<T> Size();
};

template <typename T>
tRECT<T>::tRECT(T left, T top, T right, T bottom) {
	Left = left; Right = right; Top = top; Bottom = bottom;
}

template <typename T>
tRECT<T>::tRECT( const Vector2<T>& Pos, const tSize<T>& Size ) {
	Left = Pos.x;
	Top = Pos.y;
	Right = Left + Size.Width();
	Bottom = Top + Size.Height();
}

template <typename T>
tRECT<T>::tRECT() : Left(0), Right(0), Top(0), Bottom(0) {}

template <typename T>
bool tRECT<T>::Contains( const tRECT<T>& RECT ) {
	return ( Left <= RECT.Left && Right >= RECT.Right && Top <= RECT.Top && Bottom >= RECT.Bottom );
}

template <typename T>
bool tRECT<T>::Intersect( const tRECT<T>& RECT ) {
	return !( Left > RECT.Right || Right < RECT.Left || Top > RECT.Bottom || Bottom < RECT.Top );
}

template <typename T>
bool tRECT<T>::Contains( const Vector2<T>& Point ) {
	return ( Left <= Point.x && Right >= Point.x && Top <= Point.y && Bottom >= Point.y );
}

template <typename T>
Vector2<T> tRECT<T>::Pos() {
	return Vector2<T>( Left, Top );
}

template <typename T>
tSize<T> tRECT<T>::Size() {
	return tSize<T>( Right - Left, Bottom - Top );
}

typedef tRECT<eeUint>		eeRectu;
typedef tRECT<eeFloat>		eeRectf;
typedef tRECT<eeFloat>		eeAABB; // Axis-Aligned Bounding Box
typedef tRECT<eeInt>		eeRecti;

}}

#endif
