#ifndef EE_MATHLINE2_HPP
#define EE_MATHLINE2_HPP

#include <eepp/math/vector2.hpp>

namespace EE { namespace Math {

template <typename T> class Line2 {
  public:
	Line2();

	Line2( const Vector2<T>& v1, const Vector2<T>& v2 );

	Vector2<T> V[2];

	Vector2<T>& p1() { return V[0]; }

	Vector2<T>& p2() { return V[1]; }

	Vector2<T> getNormal();

	/** @return The angle of the line against the x axis-aligned line */
	T getAngle();

	bool intersect( Line2<T>& line, T* X = NULL, T* Y = NULL );
};

template <typename T> Line2<T>::Line2() {
	V[0] = Vector2<T>();
	V[1] = Vector2<T>();
}

template <typename T> Line2<T>::Line2( const Vector2<T>& v1, const Vector2<T>& v2 ) {
	V[0] = v1;
	V[1] = v2;
}

template <typename T> Vector2<T> Line2<T>::getNormal() {
	Vector2<T> tV = Vector2<T>( -( V[1].y - V[0].y ), V[1].x - V[0].x );
	tV.normalize();
	return tV;
}

template <typename T> T Line2<T>::getAngle() {
	return eeatan2( ( V[1].y - V[0].y ), ( V[1].x - V[0].x ) ) * EE_180_PI;
}

/** Determine if two lines are intersecting
 * @param line The line to intersect
 * @param X Optional Pointer returning the X point position of intersection
 * @param Y Optional Pointer returning the Y point position of intersection
 * @return True if the lines are intersecting
 */
template <typename T> bool Line2<T>::intersect( Line2<T>& line, T* X, T* Y ) {
	T distAB, theCos, theSin, newX, ABpos;

	if ( ( V[0].x == V[1].x && V[0].y == V[1].y ) ||
		 ( line.V[0].x == line.V[1].x && line.V[0].y == line.V[1].y ) )
		return false;

	if ( ( V[0].x == line.V[0].x && V[0].y == line.V[0].y ) ||
		 ( V[1].x == line.V[0].x && V[1].y == line.V[0].y ) ||
		 ( V[0].x == line.V[1].x && V[0].y == line.V[1].y ) ||
		 ( V[1].x == line.V[1].x && V[1].y == line.V[1].y ) ) {
		return false;
	}

	V[1].x -= V[0].x;
	V[1].y -= V[0].y;
	line.V[0].x -= V[0].x;
	line.V[0].y -= V[0].y;
	line.V[1].x -= V[0].x;
	line.V[1].y -= V[0].y;

	distAB = eesqrt( V[1].x * V[1].x + V[1].y * V[1].y );

	theCos = V[1].x / distAB;
	theSin = V[1].y / distAB;
	newX = line.V[0].x * theCos + line.V[0].y * theSin;
	line.V[0].y = line.V[0].y * theCos - line.V[0].x * theSin;
	line.V[0].x = newX;
	newX = line.V[1].x * theCos + line.V[1].y * theSin;
	line.V[1].y = line.V[1].y * theCos - line.V[1].x * theSin;
	line.V[1].x = newX;

	if ( ( line.V[0].y < 0. && line.V[1].y < 0. ) || ( line.V[0].y >= 0. && line.V[1].y >= 0. ) )
		return false;

	ABpos =
		line.V[1].x + ( line.V[0].x - line.V[1].x ) * line.V[1].y / ( line.V[1].y - line.V[0].y );

	if ( ABpos < 0. || ABpos > distAB )
		return false;

	if ( X != NULL && Y != NULL ) {
		*X = V[0].x + ABpos * theCos;
		*Y = V[0].y + ABpos * theSin;
	}
	return true;
}

typedef Line2<Float> Line2f;

}} // namespace EE::Math

#endif
