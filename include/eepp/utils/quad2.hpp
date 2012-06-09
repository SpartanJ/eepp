#ifndef EE_UTILSQUAD2_HPP
#define EE_UTILSQUAD2_HPP

namespace EE { namespace Utils {

template <typename T>
class Quad2 {
	public:
		Quad2();
		Quad2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3, const Vector2<T>& v4 );

		const Vector2<T>& operator[] ( const Uint32& Pos ) const;

		Vector2<T> V[4];
		/**
		Vector2<T> V[0]; //! Left - Top Vector
		Vector2<T> V[1]; //! Left - Bottom Vector
		Vector2<T> V[2]; //! Right - Bottom Vertex
		Vector2<T> V[3]; //! Right - Top Vertex
		*/
		Vector2<T> GetCenter();

		void Rotate( const T& Angle, const Vector2<T>& Center );
		void Rotate( const T& Angle );
		void Scale( const T& scale );
		void Scale( const T& scale, const Vector2<T>& Center );
};

template <typename T>
void Quad2<T>::Rotate( const T& Angle ) {
	Rotate( Angle, GetCenter() );
}

template <typename T>
void Quad2<T>::Rotate( const T& Angle, const Vector2<T>& Center ) {
	if ( Angle == 0.f )
		return;

	V[0].RotateVectorCentered( Angle, Center );
	V[1].RotateVectorCentered( Angle, Center );
	V[2].RotateVectorCentered( Angle, Center );
	V[3].RotateVectorCentered( Angle, Center );
}

template <typename T>
void Quad2<T>::Scale( const T& scale, const Vector2<T>& Center ) {
	if ( scale == 1.0f )
		return;

	for ( Uint32 i = 0; i < 4; i++ ) {
		if ( V[i].x < Center.x )
			V[i].x = Center.x - eeabs( Center.x - V[i].x ) * scale;
		else
			V[i].x = Center.x + eeabs( Center.x - V[i].x ) * scale;

		if ( V[i].y < Center.y )
			V[i].y = Center.y - eeabs( Center.y - V[i].y ) * scale;
		else
			V[i].y = Center.y + eeabs( Center.y - V[i].y ) * scale;
	}
}

template <typename T>
void Quad2<T>::Scale( const T& scale ) {
	Scale( scale, GetCenter() );
}

template <typename T>
Vector2<T> Quad2<T>::GetCenter() {
	eeFloat MinX = V[0].x, MaxX = V[0].x, MinY = V[0].y, MaxY = V[0].y;
	for (Uint8 i = 1; i < 4; i++ ) {
		if ( MinX > V[i].x ) MinX = V[i].x;
		if ( MaxX < V[i].x ) MaxX = V[i].x;
		if ( MinY > V[i].y ) MinY = V[i].y;
		if ( MaxY < V[i].y ) MaxY = V[i].y;
	}
	return Vector2<T>( MinX + (MaxX - MinX) * 0.5f, MinY + (MaxY - MinX) * 0.5f );
}

template <typename T>
const Vector2<T>& Quad2<T>::operator[] ( const Uint32& Pos ) const {
	if ( Pos <= 3 )
		return V[Pos];

	return V[0];
}

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

typedef Quad2<eeFloat> eeQuad2f;

}}

#endif
