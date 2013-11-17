#ifndef EE_MATHVECTOR2_H
#define EE_MATHVECTOR2_H

#include <eepp/declares.hpp>

namespace EE { namespace Math {

/** @brief Utility template class for manipulating 2-dimensional vectors */
template <typename T>
class Vector2 {
	public :
		static const Vector2<T> Zero;

		static const Vector2<T> One;

		/** Default constructor creates Vector2(0,0) */
		Vector2();

		/** Creates a vector from its coordinates */
		Vector2(T X, T Y);

		/** @return A copy of the Vector2 */
		Vector2<T> Copy();

		/** @return The Dot product of the 2D vectors. */
		T Dot( const Vector2<T>& V2 );

		/** @return The Cross product of the 2D vectors.  */
		T Cross( const Vector2<T>& V2 );

		/** @return The perpendicular vector */
		Vector2<T> Perp();

		/** @return The reveser perpendicular vector */
		Vector2<T> RPerp();

		/** Uses complex multiplication to rotate self by vec. Scaling will occur if self is not a unit vector. */
		Vector2<T> Rotate( const Vector2<T>& V2 );

		/** Inverse of Vector2::Rotate */
		Vector2<T> UnRotate( const Vector2<T>& V2 );

		/** @return The vector Length */
		T Length();

		/** @return The square of the length of the 2D vector. */
		T LengthSq();

		/** Normalize the vector */
		void Normalize();

		/** @return Clamp the vector to a magnitude length. */
		void Clamp( T len );

		/** @return The unit length vector for the given angle (radians) */
		Vector2<T> ForAngle( const T& a );

		/** @return The angular direction vector is pointing in (radians). */
		T ToAngle();

		/** Rotates the vector */
		void RotateVector( const T& Angle );

		/** Rotates the vector against a defined rotation center */
		void RotateVectorCentered( const T& Angle, const Vector2<T>& RotationCenter );

		/** @return The distance between two vectors */
		T Distance( const Vector2<T>& Vec );

		/** @return The square of the distance between two vectors */
		T DistanceSq( const Vector2<T>& Vec );

		/** @return True if the distance between the two vectors is less than Dist */
		bool Near( const Vector2<T>& Vec, T Dist );

		/** @return The spherical linear interpolation between two 2D vectors. */
		Vector2<T> SphericalLerp( const Vector2<T>& Vec, T Time );

		/** Spherical linear interpolation between two vectors */
		Vector2<T> SphericalLerpConst( const Vector2<T>& Vec, T Angle );

		/** Performs a linear interpolation between two 2D vectors. */
		Vector2<T> Lerp( const Vector2<T>& Vec, T Time );

		/**	@return A vector interpolated from self towards Vec with length Dist. */
		Vector2<T> LerpConst( const Vector2<T>& Vec, T Dist );

		T x;
		T y;
	private:
		T cosAng( const T& Ang );
		T sinAng( const T& Ang );
};

template <typename T>
const Vector2<T> Vector2<T>::One = Vector2<T>(1,1);

template <typename T>
const Vector2<T> Vector2<T>::Zero = Vector2<T>(0,0);

template <typename T>
Vector2<T> Vector2<T>::Lerp( const Vector2<T>& Vec, T Time ) {
	return *this * ( 1 - Time ) + Vec * Time;
}

template <typename T>
Vector2<T> Vector2<T>::LerpConst( const Vector2<T>& Vec, T Dist ) {
	Vector2<T> t( *this + ( Vec - *this ) );
	t.Clamp( Dist );
	return t;
}

template <typename T>
Vector2<T> Vector2<T>::SphericalLerp( const Vector2<T>& Vec, T Time ) {
	T omega = eeacos( Dot( Vec ) );

	if( omega ) {
		T denom = 1 / eesin( omega );

		return ( Vector2<T>( x, y ) * (T)( eesin( ( 1 - Time ) * omega ) * denom ) + Vec * (T)( eesin( Time * omega ) * denom ) );
	} else {
		return Vector2<T>( x, y );
	}
}

template <typename T>
Vector2<T> Vector2<T>::SphericalLerpConst( const Vector2<T>& Vec, T Angle ) {
	T angle = eeacos( Dot( Vec ) );
	return Lerp( Vec, ( ( Angle < angle ) ? Angle : angle ) / angle );
}

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
Vector2<T> operator +(const Vector2<T>& V1, T X) {
	return Vector2<T>(V1.x + X, V1.y + X);
}

template <typename T>
Vector2<T> operator -(const Vector2<T>& V1, const Vector2<T>& V2) {
	return Vector2<T>(V1.x - V2.x, V1.y - V2.y);
}

template <typename T>
Vector2<T> operator -(const Vector2<T>& V1, T X) {
	return Vector2<T>(V1.x - X, V1.y - X);
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
Vector2<T> operator *(const Vector2<T>& V1, const Vector2<T>& V2) {
	return Vector2<T>(V1.x * V2.x, V1.y * V2.y);
}

template <typename T>
Vector2<T>& operator *=(Vector2<T>& V, T X) {
	V.x *= X;
	V.y *= X;

	return V;
}

template <typename T>
Vector2<T>& operator *=(Vector2<T>& V, const Vector2<T>& V2) {
	V.x *= V2.x;
	V.y *= V2.y;

	return V;
}

template <typename T>
Vector2<T> operator /(const Vector2<T>& V1, const Vector2<T>& V2) {
	return Vector2<T>(V1.x / V2.x, V1.y / V2.y);
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
bool operator ==(const Vector2<T>& V1, const T& V) {
	return (V1.x == V) && (V1.y == V);
}

template <typename T>
bool operator !=(const Vector2<T>& V1, const Vector2<T>& V2) {
	return (V1.x != V2.x) || (V1.y != V2.y);
}

template <typename T>
bool operator !=(const Vector2<T>& V1, const T& V) {
	return (V1.x != V) || (V1.y != V);
}

template <typename T>
T Vector2<T>::cosAng( const T& Ang ) {
	return eecos(Ang * EE_PI_180);
}

template <typename T>
T Vector2<T>::sinAng( const T& Ang ) {
	return eesin(Ang * EE_PI_180);
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
	return x * V2.x + y * V2.y;
}

template <typename T>
T Vector2<T>::Cross( const Vector2<T>& V2 ) {
	return x * V2.x - y * V2.y;
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
T Vector2<T>::Length() {
	return eesqrt( Dot( Vector2<T>( x , y ) ) );
}

template <typename T>
T Vector2<T>::LengthSq() {
	return Dot( Vector2<T>( x , y ) );
}

template <typename T>
void Vector2<T>::Normalize() {
	T s = eesqrt(x * x + y * y);
	if (s == 0) {
		x = 0;
		y = 0;
	} else {
		x = x / s;
		y = y / s;
	}
}

template <typename T>
Vector2<T> Vector2<T>::ForAngle( const T& a ) {
	return Vector2<T>( eecos(a), eesin(a) );
}

template <typename T>
T Vector2<T>::ToAngle() {
	return eeatan2( y, x );
}

template <typename T>
T Vector2<T>::Distance( const Vector2<T>& Vec ) {
	return  eesqrt( ( x - Vec.x ) * ( x - Vec.x ) + ( y - Vec.y ) * ( y - Vec.y ) );
}

template <typename T>
T Vector2<T>::DistanceSq( const Vector2<T>& Vec ) {
	return ( *this - Vec ).LengthSq();
}

template <typename T>
void Vector2<T>::Clamp( T len ) {
	if ( Dot( Vector2<T>( x, y ) ) > len * len ) {
		Normalize();

		x *= len;
		y *= len;
	}
}

template <typename T>
bool Vector2<T>::Near( const Vector2<T>& Vec, T Dist ) {
	return 0 != ( DistanceSq( Vec ) < Dist * Dist );
}

template <typename T>
Vector2<T> Vector2<T>::Copy() {
	return Vector2<T>( x, y );
}

typedef Vector2<Int32>		eeVector2if;
typedef Vector2<eeInt>		eeVector2i;
typedef Vector2<eeFloat>	eeVector2f;
typedef Vector2<eeDouble>	eeVector2d;
typedef Vector2<float>		eeVector2ff;
typedef Vector2<Uint32>		eeVector2u;

}}

#endif
