#ifndef EECMATH_H
#define EECMATH_H

#include "base.hpp"

namespace EE { namespace Math {

/** Set a Random Seed to the Randomizer */
Uint32 EE_API SetRandomSeed();

/** Generate a eeFloating point random number
* @param fMin The minimun value
* @param fMax the maximun value
* @return The random number generated
*/
eeFloat EE_API eeRandf( const eeFloat& fMin = 0.0f, const eeFloat& fMax = 1.0f );

/** Generate a integer random number
* @param fMin The minimun value
* @param fMax the maximun value
* @return The random number generated
*/
eeInt EE_API eeRandi( const eeInt& fMin = 0, const eeInt& fMax = 1 );

/** Cosine from an Angle in Degress */
eeFloat EE_API cosAng( const eeFloat& Ang );

/** Sinus from an Angle in Degress */
eeFloat EE_API sinAng( const eeFloat& Ang );

/** Tangen from an Angle in Degress */
eeFloat EE_API tanAng( const eeFloat& Ang );

/** Convert an Angle from Degrees to Radians */
eeFloat EE_API Radians( const eeFloat& Ang );

/** Convert an Angle from Radians to Degrees */
eeFloat EE_API Degrees( const eeFloat& Radians );

template <typename T>
T NextPowOfTwo( T Size ) {
	T p = 1;

	while ( p < Size )
		p <<= 1;

	return p;
}

template <typename T>
T IsPow2( T v ) {
	return ( ( v & ( v - 1 ) ) == 0 );
}

template <typename T>
T LineAngle( const T& X1, const T& Y1, const T& X2, const T& Y2 ) {
	return eeatan2( (eeFloat)(Y2 - Y1), (eeFloat)(X2 - X1) ) * d180PI;
}
eeFloat EE_API LineAngle( const eeVector2f& p1, const eeVector2f& p2 );

eeFloat EE_API RotatePointFromX ( const eeFloat& x, const eeFloat& y, const eeFloat& Angle );

eeFloat EE_API RotatePointFromY ( const eeFloat& x, const eeFloat& y, const eeFloat& Angle );

eeFloat EE_API RotatePointFromX ( const eeVector2f& p, const eeFloat& Angle );

eeFloat EE_API RotatePointFromY ( const eeVector2f& p, const eeFloat& Angle );

eeVector2f EE_API RotateVector( const eeVector2f& p, const eeFloat& Angle );

eeVector2f EE_API RotateVectorCentered( const eeVector2f& p, const eeFloat& Angle, const eeVector2f& RotationCenter );

eeVector2f EE_API GetQuadCenter( const eeQuad2f& Q );

eeQuad2f EE_API RotateQuadCentered( const eeQuad2f& p, const eeFloat& Angle, const eeVector2f& RotationCenter );

eeQuad2f EE_API ScaleQuadCentered( const eeQuad2f& Quad, const eeFloat& Scale, const eeVector2f& RotationCenter );

void EE_API RotateVector( eeVector2f* p, const eeFloat& Angle );

void EE_API RotateVectorCentered( eeVector2f* p, const eeFloat& Angle, const eeVector2f& RotationCenter );

template <typename T>
T Distance( T x1, T y1, T x2, T y2 ) {
	return  eesqrt( (eeFloat)( (x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2) ) );
}
eeFloat EE_API Distance( const eeVector2f& p1, const eeVector2f& p2);

template <typename T>
bool Intersect( const tRECT<T>& a, const tRECT<T>& b ) {
	return !(a.Left > b.Right || a.Right < b.Left || a.Top > b.Bottom || a.Bottom < b.Top);
}

/** @return If a contains b */
template <typename T>
bool Contains( const tRECT<T>& a, const tRECT<T>& b ) {
	return ( a.Left <= b.Left && a.Right >= b.Right && a.Top <= b.Top && a.Bottom >= b.Bottom );
}

template <typename T>
bool Contains( const tRECT<T>& a, const Vector2<T>& b) {
	return ( a.Left <= b.x && a.Right >= b.x && a.Top <= b.y && a.Bottom >= b.y );
}

template <typename T>
bool IntersectCircles( const tRECT<T>& a, const tRECT<T>& b ) {
	eeFloat ra = (eeFloat)(a.Right - a.Left) * 0.5f;
	eeFloat rb = (eeFloat)(b.Right - b.Left) * 0.5f;
	eeFloat dist = ra + rb;
	eeFloat dx = (b.Left + rb) - (a.Left + ra);
	eeFloat dy = (b.Top + rb) - (a.Top + ra);
	eeFloat res = (dx * dx) + (dy * dy);

	if ( res <= (dist * dist))
		return true;
	return false;
}

/** Determine if a RECT and a Circle are intersecting
* @param obj Object RECT
* @param cx Circle x axis position
* @param cy Circle y axis position
* @param radius Circle Radius
* @return True if are intersecting
*/
template <typename T>
bool IntersectRectCircle(const tRECT<T>& obj, const eeFloat& cx, const eeFloat& cy, const eeFloat& radius) {
	eeFloat tx = cx;
	eeFloat ty = cy;

	if (tx < obj.Left) tx = (eeFloat)obj.Left;
	if (tx > obj.Right) tx = (eeFloat)obj.Right;
	if (ty < obj.Top) ty = (eeFloat)obj.Top;
	if (ty > obj.Bottom) ty = (eeFloat)obj.Bottom;

	if (Distance( cx, cy, tx, ty ) < radius)
		return true;

	return false;
}

/** Determine if two lines are intersecting
* @param Ax First Line Fist Point X axis
* @param Ay First Line Fist Point Y axis
* @param Bx Fist Line Second Point X axis
* @param By Fist Line Second Point Y axis
* @param Cx Second Line Fist Point X axis
* @param Cy Second Line Fist Point Y axis
* @param Dx Second Line Second Point X axis
* @param Dy Second Line Second Point Y axis
* @param X Optional Pointer returning the X point position of intersection
* @param Y Optional Pointer returning the Y point position of intersection
* @return True if the lines are intersecting
*/
template <typename T>
bool IntersectLines( T Ax, T Ay, T Bx, T By, T Cx, T Cy, T Dx, T Dy, T* X, T* Y ) {
	T distAB, theCos, theSin, newX, ABpos;

	if ( ( Ax==Bx && Ay==By ) || ( Cx==Dx && Cy==Dy ) ) return false;

	if ( ( Ax==Cx && Ay==Cy ) || ( Bx==Cx && By==Cy )
	||  ( Ax==Dx && Ay==Dy ) || ( Bx==Dx && By==Dy ) ) {
		return false; }

	Bx-=Ax; By-=Ay;
	Cx-=Ax; Cy-=Ay;
	Dx-=Ax; Dy-=Ay;

	distAB=eesqrt(Bx*Bx+By*By);

	theCos=Bx/distAB;
	theSin=By/distAB;
	newX=Cx*theCos+Cy*theSin;
	Cy  =Cy*theCos-Cx*theSin; Cx=newX;
	newX=Dx*theCos+Dy*theSin;
	Dy  =Dy*theCos-Dx*theSin; Dx=newX;

	if ( ( Cy<0. && Dy<0. ) || ( Cy>=0. && Dy>=0. ) ) return false;

	ABpos=Dx+(Cx-Dx)*Dy/(Dy-Cy);

	if (ABpos<0. || ABpos>distAB) return false;

	if (X != NULL && Y != NULL) {
		*X=Ax + ABpos * theCos;
		*Y=Ay + ABpos * theSin;
	}
	return true;
}

bool EE_API IntersectLines( const eeFloat& Ax, const eeFloat& Ay, const eeFloat& Bx, const eeFloat& By, const eeFloat& Cx, const eeFloat& Cy, const eeFloat& Dx, const eeFloat& Dy, eeFloat *X = NULL, eeFloat *Y = NULL);

bool EE_API IntersectLines( const eeVector2f& l1p1, const eeVector2f& l1p2, const eeVector2f& l2p1, const eeVector2f& l2p2, eeFloat *X = NULL, eeFloat *Y = NULL);

/** @return The Dot Product of two Vectors */
template <typename T>
T VectorDotProduct( const Vector2<T>& A, const Vector2<T>& B) {
	return A.x * B.x + A.y * B.y;
}

template <typename T>
Quad2<T> AABBtoQuad2( const tRECT<T>& R ) {
	return Quad2<T>( Vector2<T>( R.Left, R.Top ), Vector2<T>( R.Left, R.Bottom ), Vector2<T>( R.Right, R.Bottom ), Vector2<T>( R.Right, R.Top ) );
}

template <typename T>
tRECT<T> Quad2toAABB( const Quad2<T>& Q, const T& OffsetX = 0, const T& OffsetY = 0 ) {
	tRECT<T> TmpR;

	eeFloat MinX = Q.V[0].x, MaxX = Q.V[0].x, MinY = Q.V[0].y, MaxY = Q.V[0].y;

	for (Uint8 i = 1; i < 4; i++ ) {
		if ( MinX > Q.V[i].x ) MinX = Q.V[i].x;
		if ( MaxX < Q.V[i].x ) MaxX = Q.V[i].x;
		if ( MinY > Q.V[i].y ) MinY = Q.V[i].y;
		if ( MaxY < Q.V[i].y ) MaxY = Q.V[i].y;
	}

	TmpR.Left = MinX + OffsetX;
	TmpR.Right = MaxX + OffsetX;
	TmpR.Top = MinY + OffsetY;
	TmpR.Bottom = MaxY + OffsetY;

	return TmpR;
}

/** Quad Quad Collision */
template <typename T>
bool IntersectQuad2( const Quad2<T>& q0, const Quad2<T>& q1, const Vector2<T>& q0Pos = Vector2<T>(0,0), const Vector2<T>& q1Pos = Vector2<T>(0,0) ) {
	Polygon2<T> Tmp1 = Polygon2<T>( q0 );
	Polygon2<T> Tmp2 = Polygon2<T>( q1 );
	Tmp1.Position( q0Pos );
	Tmp1.Position( q1Pos );

	return IntersectPolygon2( Tmp1, Tmp2 );
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
bool PointInsidePolygon2( const Polygon2<T>& poly, const Vector2<T>& point ) {
	int i, j, c = 0;

	Int32 nvert = (Int32)poly.Size();

	for ( i = 0, j = nvert - 1; i < nvert; j = i++ ) {
		if ( ( ( poly[i].y > point.y ) != ( poly[j].y > point.y ) ) && ( point.x < ( poly[j].x - poly[i].x ) * ( point.y - poly[i].y ) / ( poly[j].y - poly[i].y ) + poly[i].x ) )
			c = !c;
	}

	return 0 != c;
}

/** Polygon Polygon Collision */
template <typename T>
bool IntersectPolygon2( const Polygon2<T>& p0, const Polygon2<T>& p1 ) {
	T min0, max0, min1, max1, sOffset, t;
	Vector2<T> vAxis, vOffset;
	eeUint i = 0, j = 0, n;

	vOffset = Vector2<T>( p0.X() - p1.X(), p0.Y() - p1.Y() );

	for (i = 0; i < p0.Size(); i++) {
		n = i + 1;
		if ( n >= p0.Size() ) n = 0;

		vAxis = Line2<T>( p0[i], p0[n] ).GetNormal();

		min0 = VectorDotProduct( vAxis, p0[0] );
		max0 = min0;
		for (j = 1; j < p0.Size(); j++) {
			t = VectorDotProduct( vAxis, p0[j] );
			if (t < min0) min0 = t;
			if (t > max0) max0 = t;
		}

		min1 = VectorDotProduct( vAxis, p1[0] );
		max1 = min1;
		for (j = 1; j < p1.Size(); j++) {
			t = VectorDotProduct( vAxis, p1[j] );
			if (t < min1) min1 = t;
			if (t > max1) max1 = t;
		}

		sOffset = VectorDotProduct( vAxis, vOffset );
		min0 += sOffset;
		max0 += sOffset;

		if ( ( (min0 - max1) > 0) || ( (min1 - max0) > 0) ) {
			return false;	// Found a seperating axis, they can't possibly be touching
		}
	}

	for (i = 0; i < p1.Size(); i++) {
		n = i + 1;
		if ( n >= p1.Size() ) n = 0;

		vAxis = Line2<T>( p1[i], p1[n] ).GetNormal();

		min0 = VectorDotProduct( vAxis, p0[0] );
		max0 = min0;
		for (j = 1; j < p0.Size(); j++) {
			t = VectorDotProduct( vAxis, p0[j] );
			if (t < min0) min0 = t;
			if (t > max0) max0 = t;
		}

		min1 = VectorDotProduct( vAxis, p1[0] );
		max1 = min1;
		for (j = 1; j < p1.Size(); j++) {
			t = VectorDotProduct( vAxis, p1[j] );
			if (t < min1) min1 = t;
			if (t > max1) max1 = t;
		}

		sOffset = VectorDotProduct( vAxis, vOffset );
		min0 += sOffset;
		max0 += sOffset;

		if ( ( (min0 - max1) > 0) || ( (min1 - max0) > 0) ) {
			return false;	// Found a seperating axis, they can't possibly be touching
		}
	}

	return true;
}

template<typename T>
Polygon2<T> CreateRoundedPolygon( const T& x, const T& y, const T& width, const T& height, const eeUint& Radius = 8 ) {
	eeFloat PI05 = PI * 0.5f;
	eeFloat PI15 = PI * 1.5f;
	eeFloat PI20 = PI * 2.0f;
	eeFloat sx, sy;
	eeFloat t;

	Polygon2<T> Poly;

	Poly.PushBack( Vector2<T>( x, y + height - Radius) );
	Poly.PushBack( Vector2<T>( x, y + Radius ) );

	for( t = PI; t < PI15; t += 0.1f ) {
		sx = x + Radius + (eeFloat)cosf(t) * Radius;
		sy = y + Radius + (eeFloat)sinf(t) * Radius;

		Poly.PushBack( Vector2<T> (sx, sy) );
	}

	Poly.PushBack( Vector2<T>( x + Radius, y ) );
	Poly.PushBack( Vector2<T>( x + width - Radius, y ) );

	for( t = PI15; t < PI20; t += 0.1f ) {
		sx = x + width - Radius + (eeFloat)cosf(t) * Radius;
		sy = y + Radius + (eeFloat)sinf(t) * Radius;

		Poly.PushBack( Vector2<T> (sx, sy) );
	}

	Poly.PushBack( Vector2<T> ( x + width, y + Radius ) );
	Poly.PushBack( Vector2<T> ( x + width, y + height - Radius ) );

	for( t = 0; t < PI05; t += 0.1f ){
		sx = x + width - Radius + (eeFloat)cosf(t) * Radius;
		sy = y + height -Radius + (eeFloat)sinf(t) * Radius;

		Poly.PushBack( Vector2<T> (sx, sy) );
	}

	Poly.PushBack( Vector2<T> ( x + width - Radius, y + height ) );
	Poly.PushBack( Vector2<T> ( x + Radius, y + height ) );

	for( t = PI05; t < PI; t += 0.1f ) {
		sx = x + Radius + (eeFloat)cosf(t) * Radius;
		sy = y + height - Radius + (eeFloat)sinf(t) * Radius;

		Poly.PushBack( Vector2<T> (sx, sy) );
	}

	return Poly;
}

/* A very fast function to calculate the approximate inverse square root of a
* floating point value and a helper function that uses it for getting the
* normal squareroot. For an explanation of the inverse squareroot function
* read:
* http://www.math.purdue.edu/~clomont/Math/Papers/2003/InvSqrt.pdf
*
* Unfortunately the original creator of this function seems to be unknown.
*
* $Id: fastsqrt.h 4046 2008-04-07 15:41:22Z b_lindeijer $
*/

eeFloat EE_API eeFastInvSqrt(eeFloat x);
eeFloat EE_API eeFastSqrt(eeFloat x);

}}

#endif
