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

typedef Vector2<Int32> eeVector2if;
typedef Vector2<eeInt> eeVector2i;
typedef Vector2<eeFloat> eeVector2f;
typedef Vector2<eeDouble> eeVector2d;

}}

#endif
