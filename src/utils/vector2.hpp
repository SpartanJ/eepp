#ifndef EE_UTILSVECTOR2_H
#define EE_UTILSVECTOR2_H

namespace EE { namespace Utils {

template <typename T>
class Vector2 {
	public :
		Vector2();
		Vector2(T X, T Y);

		T Dot( const Vector2<T>& V2 );
		T Cross( const Vector2<T>& V2 );
		Vector2<T> Perp();
		Vector2<T> RPerp();
		Vector2<T> Rotate( const Vector2<T>& V2 );
		Vector2<T> UnRotate( const Vector2<T>& V2 );
		T Lenght();
		T LenghtSq();
		void Normalize();
		Vector2<T> forAngle( const T& a );
		T toAngle();

		T x;
		T y;

		void RotateVector( const T& Angle );
		void RotateVectorCentered( const T& Angle, const Vector2<T>& RotationCenter );
		T Distance( const Vector2<T>& Vec );
	private:
		T cosAng( const T& Ang );
		T sinAng( const T& Ang );
};

template <typename T>
Vector2<T>::Vector2() : x(0), y(0) {}

template <typename T>
Vector2<T>::Vector2(T X, T Y) : x(X), y(Y) {}

template <typename T>
Vector2<T> operator -(const Vector2<T>& V) {
	return Vector2<T>(-V.x, -V.y);
}

template <typename T>
Vector2<T>& operator +=(Vector2<T>& V1, const Vector2<T>& V2) {
	V1.x += V2.x;
	V1.y += V2.y;

	return V1;
}

template <typename T>
Vector2<T>& operator -=(Vector2<T>& V1, const Vector2<T>& V2) {
	V1.x -= V2.x;
	V1.y -= V2.y;

	return V1;
}

template <typename T>
Vector2<T> operator +(const Vector2<T>& V1, const Vector2<T>& V2) {
	return Vector2<T>(V1.x + V2.x, V1.y + V2.y);
}

template <typename T>
Vector2<T> operator -(const Vector2<T>& V1, const Vector2<T>& V2) {
	return Vector2<T>(V1.x - V2.x, V1.y - V2.y);
}

template <typename T>
Vector2<T> operator *(const Vector2<T>& V, T X) {
	return Vector2<T>(V.x * X, V.y * X);
}

template <typename T>
Vector2<T> operator *(T X, const Vector2<T>& V) {
	return Vector2<T>(V.x * X, V.y * X);
}

template <typename T>
Vector2<T>& operator *=(Vector2<T>& V, T X) {
	V.x *= X;
	V.y *= X;

	return V;
}

template <typename T>
Vector2<T> operator /(const Vector2<T>& V, T X) {
	return Vector2<T>(V.x / X, V.y / X);
}

template <typename T>
Vector2<T>& operator /=(Vector2<T>& V, T X) {
	V.x /= X;
	V.y /= X;

	return V;
}

template <typename T>
bool operator ==(const Vector2<T>& V1, const Vector2<T>& V2) {
	return (V1.x == V2.x) && (V1.y == V2.y);
}

template <typename T>
bool operator !=(const Vector2<T>& V1, const Vector2<T>& V2) {
	return (V1.x != V2.x) || (V1.y != V2.y);
}

template <typename T>
T Vector2<T>::cosAng( const T& Ang ) {
	return cos(Ang * PId180);
}

template <typename T>
T Vector2<T>::sinAng( const T& Ang ) {
	return sin(Ang * PId180);
}

template <typename T>
void Vector2<T>::RotateVector( const T& Angle ) {
	T nx = x * cosAng(Angle) - y * sinAng(Angle);
	y = y * cosAng(Angle) + x * sinAng(Angle);
	x = nx;
}

template <typename T>
void Vector2<T>::RotateVectorCentered( const T& Angle, const Vector2<T>& RotationCenter ) {
	x -= RotationCenter.x;
	y -= RotationCenter.y;

	RotateVector( Angle );

	x += RotationCenter.x;
	y += RotationCenter.y;
}

template <typename T>
T Vector2<T>::Dot( const Vector2<T>& V2 ) {
	return x * V2.x + y * V2.Y;
}

template <typename T>
T Vector2<T>::Cross( const Vector2<T>& V2 ) {
	return x * V2.x - y * V2.Y;
}

template <typename T>
Vector2<T> Vector2<T>::Perp() {
	return Vector2<T>( -y, x );
}

template <typename T>
Vector2<T> Vector2<T>::RPerp() {
	return Vector2<T>( y, -x );
}

template <typename T>
Vector2<T> Vector2<T>::Rotate( const Vector2<T>& V2 ) {
	return Vector2<T>( x * V2.x - y * V2.y ,  x * V2.y + y * V2.x );
}

template <typename T>
Vector2<T> Vector2<T>::UnRotate( const Vector2<T>& V2 ) {
	return Vector2<T>( x * V2.x - y * V2.y ,  x * V2.x + y * V2.y );
}

template <typename T>
T Vector2<T>::Lenght() {
	return sqrtf( Dot( Vector2<T>( x , y ) ) );
}

template <typename T>
T Vector2<T>::LenghtSq() {
	return Dot( Vector2<T>( x , y ) );
}

template <typename T>
void Vector2<T>::Normalize() {
	T s = sqrt(x * x + y * y);
	if (s == 0) {
		x = 0;
		y = 0;
	} else {
		x = x / s;
		y = y / s;
	}
}

template <typename T>
Vector2<T> Vector2<T>::forAngle( const T& a ) {
	return Vector2<T>( cos(a), sin(a) );
}

template <typename T>
T Vector2<T>::toAngle() {
	return atan2( y, x );
}

template <typename T>
T Vector2<T>::Distance( const Vector2<T>& Vec ) {
	return  sqrt((x - Vec.x) * (x - Vec.x) + (y - Vec.y) * (y - Vec.y));
}

template<typename T>
class tSize : public Vector2<T>
{
	public:
		tSize();
		tSize( const T& Width, const T& Height );
		tSize( const tSize<T>& Size );
		tSize( const Vector2<T>& Vec );

		const T& Width() const;
		const T& Height() const;
		void Width( const T& width );
		void Height( const T& height );
};

template <typename T>
tSize<T>::tSize() {
	this->x = 0;
	this->y = 0;
}

template <typename T>
tSize<T>::tSize( const T& Width, const T& Height ) {
	this->x = Width;
	this->y = Height;
}

template <typename T>
tSize<T>::tSize( const tSize<T>& Size ) {
	this->x = Size.Width();
	this->y = Size.Height();
}

template <typename T>
tSize<T>::tSize( const Vector2<T>& Vec ) {
	this->x = Vec.x;
	this->y = Vec.y;
}

template <typename T>
const T& tSize<T>::Width() const {
	return this->x;
}

template <typename T>
const T& tSize<T>::Height() const {
	return this->y;
}

template <typename T>
void tSize<T>::Width( const T& width ) {
	this->x = width;
}

template <typename T>
void tSize<T>::Height( const T& height ) {
	this->y = height;
}

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
		void Scale( const eeFloat& scale );
		void Scale( const eeFloat& scale, const Vector2<T>& Center );
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
void Quad2<T>::Scale( const eeFloat& scale, const Vector2<T>& Center ) {
	if ( scale == 1.0f )
		return;

	for ( Uint32 i = 0; i < 4; i++ ) {
		if ( V[i].x < Center.x )
			V[i].x = Center.x - fabs( Center.x - V[i].x ) * scale;
		else
			V[i].x = Center.x + fabs( Center.x - V[i].x ) * scale;

		if ( V[i].y < Center.y )
			V[i].y = Center.y - fabs( Center.y - V[i].y ) * scale;
		else
			V[i].y = Center.y + fabs( Center.y - V[i].y ) * scale;
	}
}

template <typename T>
void Quad2<T>::Scale( const eeFloat& scale ) {
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

template <typename T>
class Triangle2 {
	public:
		Triangle2();
		Triangle2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3 );
		Vector2<T> V[3];
};

template <typename T>
Triangle2<T>::Triangle2() {
	V[0] = Vector2<T>();
	V[1] = Vector2<T>();
	V[2] = Vector2<T>();
}

template <typename T>
Triangle2<T>::Triangle2( const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3 ) {
	V[0] = v1;
	V[1] = v2;
	V[2] = v3;
}

template <typename T>
class Line2 {
	public:
		Line2();
		Line2( const Vector2<T>& v1, const Vector2<T>& v2 );
		Vector2<T> V[2];

		Vector2<T> GetNormal();
};

template <typename T>
Line2<T>::Line2() {
	V[0] = Vector2<T>();
	V[1] = Vector2<T>();
}

template <typename T>
Line2<T>::Line2( const Vector2<T>& v1, const Vector2<T>& v2 ) {
	V[0] = v1;
	V[1] = v2;
}

template <typename T>
Vector2<T> Line2<T>::GetNormal() {
	Vector2<T> tV = Vector2<T>( -(V[1].y - V[0].y) , V[1].x - V[0].x );
	tV.Normalize();
	return tV;
}

typedef Vector2<Int32> eeVector2if;
typedef Vector2<eeInt> eeVector2i;
typedef Vector2<eeFloat> eeVector2f;
typedef Quad2<eeFloat> eeQuad2f;
typedef Triangle2<eeFloat> eeTriangle2f;
typedef tSize<eeInt> eeSize;

}}

#endif
