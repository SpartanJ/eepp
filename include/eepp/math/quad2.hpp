#ifndef EE_MATHQUAD2_HPP
#define EE_MATHQUAD2_HPP

#include <eepp/math/vector2.hpp>
#include <eepp/math/rect.hpp>

namespace EE { namespace Math {

/** @brief Utility template class for manipulating quads */
template <typename T>
class Quad2 {
	public:
		/** Default constructor creates an empty quad ( four Vector2(0,0) */
		Quad2();

		/** Creates a quad from a Rectangle */
		Quad2( const tRECT<T>& R );

		/** Creates a quad from four Vector2 */
		Quad2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3, const Vector2<T>& v4 );

		/** @return The vector from the position ( position from 0 to 3 ) */
		const Vector2<T>& operator[] ( const Uint32& Pos ) const;

		Vector2<T> V[4];

		/**
		Vector2<T> V[0]; //! Left - Top Vector2
		Vector2<T> V[1]; //! Left - Bottom Vector2
		Vector2<T> V[2]; //! Right - Bottom Vector2
		Vector2<T> V[3]; //! Right - Top Vector2
		@return The center point of the quad
		*/
		Vector2<T> GetCenter();

		/** @return The Vector2 from the position index ( from 0 to 3 ) */
		Vector2<T>& GetAt( Uint32 Index ) { return V[Index]; }

		/** Creates a quad from a rectangle */
		static Quad2<T> FromAABB( const tRECT<T>& R );

		/** @return The Axis-Aligned bounding box of the Quad */
		tRECT<T> ToAABB( const T& OffsetX = 0, const T& OffsetY = 0 );

		/** Rotates the quad from a rotation center */
		void Rotate( const T& Angle, const Vector2<T>& Center );

		/** Rotates the quad from its rotation center */
		void Rotate( const T& Angle );

		/** Scale the quad from its rotation center */
		void Scale( const T& scale );

		/** Scale the quad from an specified center */
		void Scale( const T& scale, const Vector2<T>& Center );

		/** Scale the quad from its rotation center */
		void Scale( const Vector2<T>& scale );

		/** Scale the quad from an specified center */
		void Scale( const Vector2<T>& scale, const Vector2<T>& Center );

		/** Move the polygon Vector2s, add to every point the distance specified  */
		void Move( Vector2<T> dist );
};

template <typename T>
Quad2<T>::Quad2() {
	V[0] = Vector2<T>();
	V[1] = Vector2<T>();
	V[2] = Vector2<T>();
	V[3] = Vector2<T>();
}

template <typename T>
Quad2<T>::Quad2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3, const Vector2<T>& v4 ) {
	V[0] = v1;
	V[1] = v2;
	V[2] = v3;
	V[3] = v4;
}

template <typename T>
Quad2<T>::Quad2( const tRECT<T>& R ) {
	V[0] = Vector2<T>( R.Left, R.Top );
	V[1] = Vector2<T>( R.Left, R.Bottom );
	V[2] = Vector2<T>( R.Right, R.Bottom );
	V[3] = Vector2<T>( R.Right, R.Top );
}

template <typename T>
void Quad2<T>::Rotate( const T& Angle ) {
	Rotate( Angle, GetCenter() );
}

template <typename T>
void Quad2<T>::Rotate( const T& Angle, const Vector2<T>& Center ) {
	if ( Angle == 0.f )
		return;

	V[0].Rotate( Angle, Center );
	V[1].Rotate( Angle, Center );
	V[2].Rotate( Angle, Center );
	V[3].Rotate( Angle, Center );
}


template <typename T>
void Quad2<T>::Scale( const Vector2<T>& scale, const Vector2<T>& Center ) {
	if ( scale == 1.0f )
		return;

	for ( Uint32 i = 0; i < 4; i++ ) {
		if ( V[i].x < Center.x )
			V[i].x = Center.x - eeabs( Center.x - V[i].x ) * scale.x;
		else
			V[i].x = Center.x + eeabs( Center.x - V[i].x ) * scale.x;

		if ( V[i].y < Center.y )
			V[i].y = Center.y - eeabs( Center.y - V[i].y ) * scale.y;
		else
			V[i].y = Center.y + eeabs( Center.y - V[i].y ) * scale.y;
	}
}

template <typename T>
void Quad2<T>::Scale( const Vector2<T>& scale ) {
	Scale( scale, GetCenter() );
}

template <typename T>
void Quad2<T>::Scale( const T& scale, const Vector2<T>& Center ) {
	Scale( Vector2<T>( scale, scale ), Center );
}

template <typename T>
void Quad2<T>::Scale( const T& scale ) {
	Scale( scale, GetCenter() );
}

template <typename T>
Vector2<T> Quad2<T>::GetCenter() {
	Float MinX = V[0].x, MaxX = V[0].x, MinY = V[0].y, MaxY = V[0].y;

	for (Uint8 i = 1; i < 4; i++ ) {
		if ( MinX > V[i].x ) MinX = V[i].x;
		if ( MaxX < V[i].x ) MaxX = V[i].x;
		if ( MinY > V[i].y ) MinY = V[i].y;
		if ( MaxY < V[i].y ) MaxY = V[i].y;
	}

	return Vector2<T>( MinX + (MaxX - MinX) * 0.5f, MinY + (MaxY - MinX) * 0.5f );
}

template <typename T>
Quad2<T> Quad2<T>::FromAABB( const tRECT<T>& R ) {
	return Quad2<T>( Vector2<T>( R.Left, R.Top ), Vector2<T>( R.Left, R.Bottom ), Vector2<T>( R.Right, R.Bottom ), Vector2<T>( R.Right, R.Top ) );
}

template <typename T>
tRECT<T> Quad2<T>::ToAABB( const T& OffsetX, const T& OffsetY ) {
	tRECT<T> TmpR;

	Float MinX = V[0].x, MaxX = V[0].x, MinY = V[0].y, MaxY = V[0].y;

	for (Uint8 i = 1; i < 4; i++ ) {
		if ( MinX > V[i].x ) MinX = V[i].x;
		if ( MaxX < V[i].x ) MaxX = V[i].x;
		if ( MinY > V[i].y ) MinY = V[i].y;
		if ( MaxY < V[i].y ) MaxY = V[i].y;
	}

	TmpR.Left	= MinX + OffsetX;
	TmpR.Right	= MaxX + OffsetX;
	TmpR.Top	= MinY + OffsetY;
	TmpR.Bottom	= MaxY + OffsetY;

	return TmpR;
}

template <typename T>
const Vector2<T>& Quad2<T>::operator[] ( const Uint32& Pos ) const {
	if ( Pos <= 3 )
		return V[Pos];

	return V[0];
}

template <typename T>
void Quad2<T>::Move( Vector2<T> dist ) {
	if ( dist.x == 0 && dist.y == 0 )
		return;

	for ( Uint32 i = 0; i < 4; i++ ) {
		V[i] += dist;
	}
}

typedef Quad2<Float> Quad2f;

}}

#endif
