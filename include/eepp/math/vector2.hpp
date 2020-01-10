#ifndef EE_MATHVECTOR2_H
#define EE_MATHVECTOR2_H

#include <cmath>
#include <eepp/config.hpp>

namespace EE { namespace Math {

/** @brief Utility template class for manipulating 2-dimensional vectors */
template <typename T> class Vector2 {
  public:
	static const Vector2<T> Zero;

	static const Vector2<T> One;

	/** Default constructor creates Vector2(0,0) */
	Vector2();

	/** Creates a vector from its coordinates */
	Vector2( T X, T Y );

	/** @return A copy of the Vector2 */
	Vector2<T> copy();

	/** @return The Dot product of the 2D vectors. */
	T dot( const Vector2<T>& V2 );

	/** @return The Cross product of the 2D vectors.  */
	T cross( const Vector2<T>& V2 );

	/** @return The perpendicular vector */
	Vector2<T> perp();

	/** @return The reveser perpendicular vector */
	Vector2<T> rPerp();

	/** Uses complex multiplication to rotate self by vec. Scaling will occur if self is not a unit
	 * vector. */
	Vector2<T> rotate( const Vector2<T>& V2 );

	/** Inverse of Vector2::rotate */
	Vector2<T> unrotate( const Vector2<T>& V2 );

	/** @return The vector Length */
	T length();

	/** @return The square of the length of the 2D vector. */
	T lengthSq();

	/** Normalize the vector */
	void normalize();

	/** @return Clamp the vector to a magnitude length. */
	void clamp( T len );

	/** @return The unit length vector for the given angle (radians) */
	Vector2<T> forAngle( const T& a );

	/** @return The angular direction vector is pointing in (radians). */
	T toAngle();

	/** Rotates the vector */
	void rotate( const T& Angle );

	/** Rotates the vector against a defined rotation center */
	void rotate( const T& Angle, const Vector2<T>& RotationCenter );

	/** @return The distance between two vectors */
	T distance( const Vector2<T>& Vec );

	/** @return The square of the distance between two vectors */
	T distanceSq( const Vector2<T>& Vec );

	/** @return True if the distance between the two vectors is less than Dist */
	bool nearDist( const Vector2<T>& Vec, T Dist );

	/** @return The spherical linear interpolation between two 2D vectors. */
	Vector2<T> sphericalLerp( const Vector2<T>& Vec, T Time );

	/** Spherical linear interpolation between two vectors */
	Vector2<T> sphericalLerpConst( const Vector2<T>& Vec, T Angle );

	/** Performs a linear interpolation between two 2D vectors. */
	Vector2<T> lerp( const Vector2<T>& Vec, T Time );

	/**	@return A vector interpolated from self towards Vec with length Dist. */
	Vector2<T> lerpConst( const Vector2<T>& Vec, T Dist );

	/** Scales the vector position against another vector */
	void scale( const Vector2<T>& scale, const Vector2<T>& Center );

	/** Scales the vector position against another vector */
	void scale( const T& scale, const Vector2<T>& Center );

	Vector2<T> ceil();

	Vector2<T> floor();

	Vector2<T> abs();

	Vector2<Float> asFloat();

	Vector2<int> asInt();

	T x;
	T y;

  private:
	T cosAng( const T& Ang );
	T sinAng( const T& Ang );
};

template <typename T> const Vector2<T> Vector2<T>::One = Vector2<T>( 1, 1 );

template <typename T> const Vector2<T> Vector2<T>::Zero = Vector2<T>( 0, 0 );

template <typename T> Vector2<T> Vector2<T>::lerp( const Vector2<T>& Vec, T Time ) {
	return *this * ( 1 - Time ) + Vec * Time;
}

template <typename T> Vector2<T> Vector2<T>::lerpConst( const Vector2<T>& Vec, T Dist ) {
	Vector2<T> t( *this + ( Vec - *this ) );
	t.clamp( Dist );
	return t;
}

template <typename T> Vector2<T> Vector2<T>::sphericalLerp( const Vector2<T>& Vec, T Time ) {
	T omega = eeacos( dot( Vec ) );

	if ( omega ) {
		T denom = 1 / eesin( omega );

		return ( Vector2<T>( x, y ) * ( T )( eesin( ( 1 - Time ) * omega ) * denom ) +
				 Vec * ( T )( eesin( Time * omega ) * denom ) );
	} else {
		return Vector2<T>( x, y );
	}
}

template <typename T> Vector2<T> Vector2<T>::sphericalLerpConst( const Vector2<T>& Vec, T Angle ) {
	T angle = eeacos( dot( Vec ) );
	return lerp( Vec, ( ( Angle < angle ) ? Angle : angle ) / angle );
}

template <typename T> Vector2<T>::Vector2() : x( 0 ), y( 0 ) {}

template <typename T> Vector2<T>::Vector2( T X, T Y ) : x( X ), y( Y ) {}

template <typename T> Vector2<T> operator-( const Vector2<T>& V ) {
	return Vector2<T>( -V.x, -V.y );
}

template <typename T> Vector2<T>& operator+=( Vector2<T>& V1, const Vector2<T>& V2 ) {
	V1.x += V2.x;
	V1.y += V2.y;

	return V1;
}

template <typename T> Vector2<T>& operator-=( Vector2<T>& V1, const Vector2<T>& V2 ) {
	V1.x -= V2.x;
	V1.y -= V2.y;

	return V1;
}

template <typename T> Vector2<T> operator+( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return Vector2<T>( V1.x + V2.x, V1.y + V2.y );
}

template <typename T> Vector2<T> operator+( const Vector2<T>& V1, T X ) {
	return Vector2<T>( V1.x + X, V1.y + X );
}

template <typename T> Vector2<T> operator-( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return Vector2<T>( V1.x - V2.x, V1.y - V2.y );
}

template <typename T> Vector2<T> operator-( const Vector2<T>& V1, T X ) {
	return Vector2<T>( V1.x - X, V1.y - X );
}

template <typename T> Vector2<T> operator*( const Vector2<T>& V, T X ) {
	return Vector2<T>( V.x * X, V.y * X );
}

template <typename T> Vector2<T> operator*( T X, const Vector2<T>& V ) {
	return Vector2<T>( V.x * X, V.y * X );
}

template <typename T> Vector2<T> operator*( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return Vector2<T>( V1.x * V2.x, V1.y * V2.y );
}

template <typename T> Vector2<T>& operator*=( Vector2<T>& V, T X ) {
	V.x *= X;
	V.y *= X;

	return V;
}

template <typename T> Vector2<T>& operator*=( Vector2<T>& V, const Vector2<T>& V2 ) {
	V.x *= V2.x;
	V.y *= V2.y;

	return V;
}

template <typename T> Vector2<T> operator/( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return Vector2<T>( V1.x / V2.x, V1.y / V2.y );
}

template <typename T> Vector2<T> operator/( const Vector2<T>& V, T X ) {
	return Vector2<T>( V.x / X, V.y / X );
}

template <typename T> Vector2<T>& operator/=( Vector2<T>& V, T X ) {
	V.x /= X;
	V.y /= X;

	return V;
}

template <typename T> bool operator==( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return ( V1.x == V2.x ) && ( V1.y == V2.y );
}

template <typename T> bool operator==( const Vector2<T>& V1, const T& V ) {
	return ( V1.x == V ) && ( V1.y == V );
}

template <typename T> bool operator==( const T& V, const Vector2<T>& V1 ) {
	return ( V1.x == V ) && ( V1.y == V );
}

template <typename T> bool operator!=( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return ( V1.x != V2.x ) || ( V1.y != V2.y );
}

template <typename T> bool operator!=( const Vector2<T>& V1, const T& V ) {
	return ( V1.x != V ) || ( V1.y != V );
}

template <typename T> bool operator!=( const T& V, const Vector2<T>& V1 ) {
	return ( V1.x != V ) || ( V1.y != V );
}

template <typename T> bool operator<( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return ( V1.x < V2.x ) && ( V1.y < V2.y );
}

template <typename T> bool operator>( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return ( V1.x > V2.x ) && ( V1.y > V2.y );
}

template <typename T> bool operator<=( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return ( V1.x <= V2.x ) && ( V1.y <= V2.y );
}

template <typename T> bool operator>=( const Vector2<T>& V1, const Vector2<T>& V2 ) {
	return ( V1.x >= V2.x ) && ( V1.y >= V2.y );
}

template <typename T> T Vector2<T>::cosAng( const T& Ang ) {
	return eecos( Ang * EE_PI_180 );
}

template <typename T> T Vector2<T>::sinAng( const T& Ang ) {
	return eesin( Ang * EE_PI_180 );
}

template <typename T> void Vector2<T>::rotate( const T& Angle ) {
	T nx = x * cosAng( Angle ) - y * sinAng( Angle );
	y = y * cosAng( Angle ) + x * sinAng( Angle );
	x = nx;
}

template <typename T> void Vector2<T>::rotate( const T& Angle, const Vector2<T>& RotationCenter ) {
	if ( 1.f != Angle ) {
		x -= RotationCenter.x;
		y -= RotationCenter.y;

		rotate( Angle );

		x += RotationCenter.x;
		y += RotationCenter.y;
	}
}

template <typename T> void Vector2<T>::scale( const Vector2<T>& scale, const Vector2<T>& Center ) {
	if ( 1.f != scale ) {
		if ( x < Center.x )
			x = Center.x - eeabs( Center.x - x ) * scale.x;
		else
			x = Center.x + eeabs( Center.x - x ) * scale.x;

		if ( y < Center.y )
			y = Center.y - eeabs( Center.y - y ) * scale.y;
		else
			y = Center.y + eeabs( Center.y - y ) * scale.y;
	}
}

template <typename T> void Vector2<T>::scale( const T& scale, const Vector2<T>& Center ) {
	scale( Vector2<T>( scale, scale ), Center );
}

template <typename T> T Vector2<T>::dot( const Vector2<T>& V2 ) {
	return x * V2.x + y * V2.y;
}

template <typename T> T Vector2<T>::cross( const Vector2<T>& V2 ) {
	return x * V2.x - y * V2.y;
}

template <typename T> Vector2<T> Vector2<T>::perp() {
	return Vector2<T>( -y, x );
}

template <typename T> Vector2<T> Vector2<T>::rPerp() {
	return Vector2<T>( y, -x );
}

template <typename T> Vector2<T> Vector2<T>::rotate( const Vector2<T>& V2 ) {
	return Vector2<T>( x * V2.x - y * V2.y, x * V2.y + y * V2.x );
}

template <typename T> Vector2<T> Vector2<T>::unrotate( const Vector2<T>& V2 ) {
	return Vector2<T>( x * V2.x - y * V2.y, x * V2.x + y * V2.y );
}

template <typename T> T Vector2<T>::length() {
	return eesqrt( dot( Vector2<T>( x, y ) ) );
}

template <typename T> T Vector2<T>::lengthSq() {
	return dot( Vector2<T>( x, y ) );
}

template <typename T> void Vector2<T>::normalize() {
	T s = eesqrt( x * x + y * y );
	if ( s == 0 ) {
		x = 0;
		y = 0;
	} else {
		x = x / s;
		y = y / s;
	}
}

template <typename T> Vector2<T> Vector2<T>::forAngle( const T& a ) {
	return Vector2<T>( eecos( a ), eesin( a ) );
}

template <typename T> T Vector2<T>::toAngle() {
	return eeatan2( y, x );
}

template <typename T> T Vector2<T>::distance( const Vector2<T>& Vec ) {
	return eesqrt( ( x - Vec.x ) * ( x - Vec.x ) + ( y - Vec.y ) * ( y - Vec.y ) );
}

template <typename T> T Vector2<T>::distanceSq( const Vector2<T>& Vec ) {
	return ( *this - Vec ).lengthSq();
}

template <typename T> void Vector2<T>::clamp( T len ) {
	if ( dot( Vector2<T>( x, y ) ) > len * len ) {
		normalize();

		x *= len;
		y *= len;
	}
}

template <typename T> Vector2<T> Vector2<T>::ceil() {
	return Vector2<T>( eeceil( x ), eeceil( y ) );
}

template <typename T> Vector2<T> Vector2<T>::floor() {
	return Vector2<T>( eefloor( x ), eefloor( y ) );
}

template <typename T> Vector2<T> Vector2<T>::abs() {
	return Vector2<T>( eeabs( x ), eeabs( y ) );
}

template <typename T> Vector2<Float> Vector2<T>::asFloat() {
	return Vector2<Float>( x, y );
}

template <typename T> Vector2<int> Vector2<T>::asInt() {
	return Vector2<int>( x, y );
}

template <typename T> bool Vector2<T>::nearDist( const Vector2<T>& Vec, T Dist ) {
	return 0 != ( distanceSq( Vec ) < Dist * Dist );
}

template <typename T> Vector2<T> Vector2<T>::copy() {
	return Vector2<T>( x, y );
}

typedef Vector2<Int32> Vector2if;
typedef Vector2<int> Vector2i;
typedef Vector2<Float> Vector2f;
typedef Vector2<double> Vector2d;
typedef Vector2<float> Vector2ff;
typedef Vector2<Uint32> Vector2u;

}} // namespace EE::Math

#endif
