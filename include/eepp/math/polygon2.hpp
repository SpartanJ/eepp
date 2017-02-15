#ifndef EE_MATHPOLIGON2_H
#define EE_MATHPOLIGON2_H

#include <eepp/math/triangle2.hpp>
#include <eepp/math/quad2.hpp>
#include <eepp/math/line2.hpp>
#include <eepp/math/rect.hpp>
#include <vector>

namespace EE { namespace Math {

/** @brief Utility template class for manipulating polygons */
template <typename T>
class Polygon2 {
	public:
		/** Default constructor. Creates an empty object. */
		Polygon2();

		~Polygon2();

		/** Creates a polygon from other polygon */
		Polygon2( const Polygon2<T>& fromPoly );

		/** Creates a polygon from a triangle */
		Polygon2( const Triangle2<T>& fromTrig );

		/** Creates a polygon from a quad */
		Polygon2( const Quad2<T>& fromQuad );

		/** Creates a polygon from a rectangle */
		Polygon2( const tRECT<T>& fromRect );

		/** Creates a polygon from a vector of Vector2 ( 2D vector ) */
		Polygon2( const std::vector< Vector2<T> >& theVecs );

		/** Adds a new Vector2 to the polygon */
		Uint32 pushBack( const Vector2<T>& V );

		/** Removes the las Vector2 from the polygon */
		void popBack();

		/** Clear the polygon vectors */
		void clear();

		/** @return The polygon Vector2 from the position ( from 0 to polygon size -1 ) */
		const Vector2<T>& operator[] ( const Uint32& Pos ) const;

		/** @return The polygon Vector2 from the position ( from 0 to polygon size -1 ) */
		Vector2<T>& getAt( const Uint32& Pos )
		{
			return Vector[Pos];
		}

		/** Change the polygon vector in the position specified
		**	@return The Vector2 changed */
		Vector2<T>& setAt( const Uint32& Pos, Vector2<T> newPos )
		{
			Vector[Pos] = newPos;
			return Vector[Pos];
		}

		/** @return The number of vectors of the polygon */
		std::size_t size() const;

		/** @return The position of the polygon ( also known as the offset of the polygon ) */
		Vector2<T> position() { return Vector2<T>(OffsetX, OffsetY); }

		/** Move the polygon Vector2s, add to every point the distance specified  */
		void move( Vector2<T> dist );

		/** @return The X position of the polygon ( the X-axis Offset ) */
		T x() const { return OffsetX; }

		/** @return The Y position of the polygon ( the Y-axis Offset ) */
		T y() const { return OffsetY; }

		/** @return The position of the polygon  ( the offset )*/
		void position( const Vector2<T>& V ) { OffsetX = V.x; OffsetY = V.y; }

		/** Set the new position of the x-axis ( the x-axis offset ) */
		T x( const T& x ) { OffsetX = x; }

		/** Set the new position of the y-axis ( the y-axis offset ) */
		T y( const T& y ) { OffsetY = y; }

		/** @return True if the polygons intersect */
		bool intersect( const Polygon2<T>& p1 );

		/** Rotates the polygon from a rotation center */
		void rotate( const T& angle, const Vector2<T>& center );

		/** Scale the polygon from a center point */
		void scale( const T& scale, const Vector2<T>& center );

		/** Scale the polygon from a center point */
		void scale( const Vector2<T>& scale, const Vector2<T>& center );

		/** @return True if the point is inside the polygon */
		bool pointInside( const Vector2<T>& point );

		/** @return The polygon axis-aligned bounding box */
		tRECT<T> toAABB();

		/** Creates a rounded rectangle polygon */
		static Polygon2<T> createRoundedRectangle( const T& x, const T& y, const T& width, const T& height, const unsigned int& Radius = 8 );

		/** @brief Intersect to Quads
		**	Convert the two quads in two polygons, and execute a polygon to polygon collition.
		**	@param q0 First quad
		**	@param q1 Second quad
		**	@param q0Pos The q0 quad polygon offset
		**	@param q1Pos The q1 quad polygon offset */
		static bool intersectQuad2( const Quad2<T>& q0, const Quad2<T>& q1, const Vector2<T>& q0Pos = Vector2<T>(0,0), const Vector2<T>& q1Pos = Vector2<T>(0,0) );

		/** @return Th closest polygon point to the point (to).
		**	@param to The point
		**	@param distance A pointer that returns the distance between the point and the closest point to the point */
		Uint32 closestPoint( const Vector2<T> &to, T * distance = NULL );
	private:
		std::vector< Vector2<T> > Vector;
		T OffsetX, OffsetY;
};

template <typename T>
Polygon2<T>::Polygon2() :
	OffsetX(0),
	OffsetY(0)
{
	clear();
}

template <typename T>
Polygon2<T>::Polygon2( const Polygon2<T>& fromPoly ) :
	Vector( fromPoly.Vector ),
	OffsetX( fromPoly.OffsetX ),
	OffsetY( fromPoly.OffsetY )
{
}

template <typename T>
Polygon2<T>::Polygon2( const std::vector< Vector2<T> >& theVecs ) : OffsetX(0), OffsetY(0) {
	for (Uint32 i = 0; i < theVecs.size(); i++)
		pushBack ( theVecs[i] );
}

template <typename T>
Polygon2<T>::Polygon2( const Triangle2<T>& fromTrig ) : OffsetX(0), OffsetY(0) {
	for (Uint8 i = 0; i < 3; i++)
		pushBack ( fromTrig.V[i] );
}

template <typename T>
Polygon2<T>::Polygon2( const Quad2<T>& fromQuad ) : OffsetX(0), OffsetY(0) {
	for (Uint8 i = 0; i < 4; i++)
		pushBack ( fromQuad.V[i] );
}

template<typename T>
Polygon2<T>::Polygon2( const tRECT<T>& fromRect ) : OffsetX(0), OffsetY(0) {
	Vector.push_back( Vector2<T>( fromRect.Left, fromRect.Top ) );
	Vector.push_back( Vector2<T>( fromRect.Left, fromRect.Bottom ) );
	Vector.push_back( Vector2<T>( fromRect.Right, fromRect.Bottom ) );
	Vector.push_back( Vector2<T>( fromRect.Right, fromRect.Top ) );
}

template <typename T>
Polygon2<T>::~Polygon2() {
	clear();
}

template <typename T>
void Polygon2<T>::clear() {
	Vector.clear();
}

template <typename T>
Uint32 Polygon2<T>::pushBack( const Vector2<T>& V ) {
	Vector.push_back( V );
	return (Uint32)Vector.size() - 1;
}

template <typename T>
void Polygon2<T>::popBack() {
	Vector.pop_back();
}

template <typename T>
const Vector2<T>& Polygon2<T>::operator[] ( const Uint32& Pos ) const {
	if ( Vector.size() > 0 && Pos < Vector.size() )
		return Vector[Pos];
	return Vector[0];
}

template <typename T>
std::size_t Polygon2<T>::size() const {
	return Vector.size();
}

template <typename T>
void Polygon2<T>::rotate(const T& angle, const Vector2<T>& center ) {
	if ( angle == 0.f )
		return;

	for ( unsigned int i = 0; i < Vector.size(); i++ )
		Vector[ i ].rotate( angle, center );
}

template <typename T>
void Polygon2<T>::scale(const Vector2<T>& scale, const Vector2<T>& center ) {
	if ( scale == 1.0f )
		return;

	for ( Uint32 i = 0; i < Vector.size(); i++ ) {
		if ( Vector[i].x < center.x )
			Vector[i].x = center.x - eeabs( center.x - Vector[i].x ) * scale.x;
		else
			Vector[i].x = center.x + eeabs( center.x - Vector[i].x ) * scale.x;

		if ( Vector[i].y < center.y )
			Vector[i].y = center.y - eeabs( center.y - Vector[i].y ) * scale.y;
		else
			Vector[i].y = center.y + eeabs( center.y - Vector[i].y ) * scale.y;
	}
}

template <typename T>
void Polygon2<T>::scale(const T& scale, const Vector2<T>& center ) {
	scale( Vector2<T>( scale, scale ), center );
}

template<typename T>
Polygon2<T> Polygon2<T>::createRoundedRectangle( const T& x, const T& y, const T& width, const T& height, const unsigned int& Radius ) {
	T PI05 = (T)EE_PI * 0.5f;
	T PI15 = (T)EE_PI * 1.5f;
	T PI20 = (T)EE_PI2;
	T sx, sy;
	T t;

	Polygon2<T> Poly;

	Poly.pushBack( Vector2<T>( x, y + height - Radius) );
	Poly.pushBack( Vector2<T>( x, y + Radius ) );

	for( t = (T)EE_PI; t < PI15; t += 0.1f ) {
		sx = x + Radius + (Float)cosf(t) * Radius;
		sy = y + Radius + (Float)sinf(t) * Radius;

		Poly.pushBack( Vector2<T> (sx, sy) );
	}

	Poly.pushBack( Vector2<T>( x + Radius, y ) );
	Poly.pushBack( Vector2<T>( x + width - Radius, y ) );

	for( t = PI15; t < PI20; t += 0.1f ) {
		sx = x + width - Radius + (Float)cosf(t) * Radius;
		sy = y + Radius + (Float)sinf(t) * Radius;

		Poly.pushBack( Vector2<T> (sx, sy) );
	}

	Poly.pushBack( Vector2<T> ( x + width, y + Radius ) );
	Poly.pushBack( Vector2<T> ( x + width, y + height - Radius ) );

	for( t = 0; t < PI05; t += 0.1f ){
		sx = x + width - Radius + (Float)cosf(t) * Radius;
		sy = y + height -Radius + (Float)sinf(t) * Radius;

		Poly.pushBack( Vector2<T> (sx, sy) );
	}

	Poly.pushBack( Vector2<T> ( x + width - Radius, y + height ) );
	Poly.pushBack( Vector2<T> ( x + Radius, y + height ) );

	for( t = PI05; t < (T)EE_PI; t += 0.1f ) {
		sx = x + Radius + (Float)cosf(t) * Radius;
		sy = y + height - Radius + (Float)sinf(t) * Radius;

		Poly.pushBack( Vector2<T> (sx, sy) );
	}

	return Poly;
}


/**
Copyright (c) 1970-2003, Wm. Randolph Franklin

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimers.
Redistributions in binary form must reproduce the above copyright notice in the documentation and/or other materials provided with the distribution.
The name of W. Randolph Franklin may not be used to endorse or promote products derived from this Software without specific prior written permission.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
template<typename T>
bool Polygon2<T>::pointInside( const Vector2<T>& point ) {
	int i, j, c = 0;
	int nvert = (int)size();

	for ( i = 0, j = nvert - 1; i < nvert; j = i++ ) {
		if ( ( ( Vector[i].y > point.y ) != ( Vector[j].y > point.y ) ) &&
			 ( point.x < ( Vector[j].x - Vector[i].x ) * ( point.y - Vector[i].y ) / ( Vector[j].y - Vector[i].y ) + Vector[i].x ) )
		{
			c = !c;
		}
	}

	return 0 != c;
}

/** Polygon Polygon Collision ( SAT ) */
template <typename T>
bool Polygon2<T>::intersect( const Polygon2<T>& p1 ) {
	T min0, max0, min1, max1, sOffset, t;
	Vector2<T> vAxis, vOffset;
	unsigned int i = 0, j = 0, n, size = this->size();

	vOffset = Vector2<T>( x() - p1.x(), y() - p1.y() );

	for (i = 0; i < size; i++) {
		n = i + 1;
		if ( n >= this->size() ) n = 0;

		vAxis = Line2<T>( Vector[i], Vector[n] ).getNormal();

		min0 = vAxis.Dot( Vector[0] );
		max0 = min0;
		for (j = 1; j < this->size(); j++) {
			t = vAxis.Dot( Vector[j] );
			if (t < min0) min0 = t;
			if (t > max0) max0 = t;
		}

		min1 = vAxis.Dot( p1[0] );
		max1 = min1;
		for (j = 1; j < p1.size(); j++) {
			t = vAxis.Dot( p1[j] );
			if (t < min1) min1 = t;
			if (t > max1) max1 = t;
		}

		sOffset = vAxis.Dot( vOffset );
		min0 += sOffset;
		max0 += sOffset;

		if ( ( (min0 - max1) > 0) || ( (min1 - max0) > 0) ) {
			return false;	// Found a seperating axis, they can't possibly be touching
		}
	}

	for (i = 0; i < p1.size(); i++) {
		n = i + 1;
		if ( n >= p1.size() ) n = 0;

		vAxis = Line2<T>( p1[i], p1[n] ).getNormal();

		min0 = vAxis.Dot( Vector[0] );
		max0 = min0;
		for (j = 1; j < this->size(); j++) {
			t = vAxis.Dot( Vector[j] );
			if (t < min0) min0 = t;
			if (t > max0) max0 = t;
		}

		min1 = vAxis.Dot( p1[0] );
		max1 = min1;
		for (j = 1; j < p1.size(); j++) {
			t = vAxis.Dot( p1[j] );
			if (t < min1) min1 = t;
			if (t > max1) max1 = t;
		}

		sOffset = vAxis.Dot( vOffset );
		min0 += sOffset;
		max0 += sOffset;

		if ( ( (min0 - max1) > 0) || ( (min1 - max0) > 0) ) {
			return false;	// Found a seperating axis, they can't possibly be touching
		}
	}

	return true;
}


/** Quad Quad Collision */
template <typename T>
bool Polygon2<T>::intersectQuad2( const Quad2<T>& q0, const Quad2<T>& q1, const Vector2<T>& q0Pos, const Vector2<T>& q1Pos ) {
	Polygon2<T> Tmp1 = Polygon2<T>( q0 );
	Polygon2<T> Tmp2 = Polygon2<T>( q1 );

	Tmp1.position( q0Pos );
	Tmp1.position( q1Pos );

	return Tmp1.intersect( Tmp2 );
}

template <typename T>
tRECT<T> Polygon2<T>::toAABB() {
	tRECT<T> TmpR;

	if ( Vector.size() < 4 ) {
		return TmpR;
	}

	Float MinX = Vector[0].x, MaxX = Vector[0].x, MinY = Vector[0].y, MaxY = Vector[0].y;

	for (Uint32 i = 1; i < Vector.size(); i++ ) {
		if ( MinX > Vector[i].x ) MinX = Vector[i].x;
		if ( MaxX < Vector[i].x ) MaxX = Vector[i].x;
		if ( MinY > Vector[i].y ) MinY = Vector[i].y;
		if ( MaxY < Vector[i].y ) MaxY = Vector[i].y;
	}

	TmpR.Left	= MinX + OffsetX;
	TmpR.Right	= MaxX + OffsetX;
	TmpR.Top	= MinY + OffsetY;
	TmpR.Bottom	= MaxY + OffsetY;

	return TmpR;
}

template <typename T>
void Polygon2<T>::move( Vector2<T> dist ) {
	for ( Uint32 i = 0; i < Vector.size(); i++ ) {
		Vector[i] += dist;
	}
}

template <typename T>
Uint32 Polygon2<T>::closestPoint( const Vector2<T>& to, T * distance ) {
	Uint32 Index	= 0;
	T Dist			= (T)99999999;
	T tDist;

	if ( !Vector.size() ) {
		return eeINDEX_NOT_FOUND;
	}

	for ( Uint32 i = 0; i < Vector.size(); i++ ) {
		tDist = Vector[i].distance( to );

		if ( tDist < Dist  ) {
			Index	= i;
			Dist	= tDist;
		}
	}

	if ( NULL != distance ) {
		*distance = Dist;
	}

	return Index;
}

typedef Polygon2<Float> Polygon2f;

}}

#endif
