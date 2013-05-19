#ifndef EE_MATHPOLIGON2_H
#define EE_MATHPOLIGON2_H

#include <eepp/math/triangle2.hpp>
#include <eepp/math/quad2.hpp>
#include <eepp/math/line2.hpp>
#include <eepp/math/rect.hpp>
#include <vector>

namespace EE { namespace Math {

template <typename T>
class Polygon2 {
	public:
		Polygon2();

		~Polygon2();

		Polygon2( const Polygon2<T>& fromPoly );

		Polygon2( const Triangle2<T>& fromTrig );

		Polygon2( const Quad2<T>& fromQuad );

		Polygon2( const tRECT<T>& fromRect );

		Polygon2( const std::vector< Vector2<T> >& theVecs );

		Uint32 PushBack( const Vector2<T>& V );

		void PopBack();

		void Clear();

		const Vector2<T>& operator[] ( const Uint32& Pos ) const;

		Vector2<T>& GetAt( const Uint32& Pos )
		{
			return Vector[Pos];
		}

		Vector2<T>& SetAt( const Uint32& Pos, Vector2<T> newPos )
		{
			Vector[Pos] = newPos;
			return Vector[Pos];
		}

		std::size_t Size() const;

		Vector2<T> Position() { return Vector2<T>(cOffsetX, cOffsetY); }

		void Move( Vector2<T> dist );

		T X() const { return cOffsetX; }

		T Y() const { return cOffsetY; }

		void Position( const Vector2<T>& V ) { cOffsetX = V.x; cOffsetY = V.y; }

		T X( const T& x ) { cOffsetX = x; }

		T Y( const T& y ) { cOffsetY = y; }

		bool Intersect( const Polygon2<T>& p1 );

		void Rotate( const T& Angle, const Vector2<T>& Center );

		void Scale( const T& scale, const Vector2<T>& Center );

		bool PointInside( const Vector2<T>& point );

		tRECT<T> ToAABB();

		static Polygon2<T> CreateRoundedRectangle( const T& x, const T& y, const T& width, const T& height, const eeUint& Radius = 8 );

		static bool IntersectQuad2( const Quad2<T>& q0, const Quad2<T>& q1, const Vector2<T>& q0Pos = Vector2<T>(0,0), const Vector2<T>& q1Pos = Vector2<T>(0,0) );

		Uint32 ClosestPoint( const Vector2<T> &to, T * distance = NULL );
	private:
		std::vector< Vector2<T> > Vector;
		T cOffsetX, cOffsetY;
};

template <typename T>
Polygon2<T>::Polygon2() :
	cOffsetX(0),
	cOffsetY(0)
{
	Clear();
}

template <typename T>
Polygon2<T>::Polygon2( const Polygon2<T>& fromPoly ) :
	Vector( fromPoly.Vector ),
	cOffsetX( fromPoly.cOffsetX ),
	cOffsetY( fromPoly.cOffsetY )
{
}

template <typename T>
Polygon2<T>::Polygon2( const std::vector< Vector2<T> >& theVecs ) : cOffsetX(0), cOffsetY(0) {
	for (Uint32 i = 0; i < theVecs.size(); i++)
		PushBack ( theVecs[i] );
}

template <typename T>
Polygon2<T>::Polygon2( const Triangle2<T>& fromTrig ) : cOffsetX(0), cOffsetY(0) {
	for (Uint8 i = 0; i < 3; i++)
		PushBack ( fromTrig.V[i] );
}

template <typename T>
Polygon2<T>::Polygon2( const Quad2<T>& fromQuad ) : cOffsetX(0), cOffsetY(0) {
	for (Uint8 i = 0; i < 4; i++)
		PushBack ( fromQuad.V[i] );
}

template<typename T>
Polygon2<T>::Polygon2( const tRECT<T>& fromRect ) : cOffsetX(0), cOffsetY(0) {
	Vector.push_back( Vector2<T>( fromRect.Left, fromRect.Top ) );
	Vector.push_back( Vector2<T>( fromRect.Left, fromRect.Bottom ) );
	Vector.push_back( Vector2<T>( fromRect.Right, fromRect.Bottom ) );
	Vector.push_back( Vector2<T>( fromRect.Right, fromRect.Top ) );
}

template <typename T>
Polygon2<T>::~Polygon2() {
	Clear();
}

template <typename T>
void Polygon2<T>::Clear() {
	Vector.clear();
}

template <typename T>
Uint32 Polygon2<T>::PushBack( const Vector2<T>& V ) {
	Vector.push_back( V );
	return (Uint32)Vector.size() - 1;
}

template <typename T>
void Polygon2<T>::PopBack() {
	Vector.pop_back();
}

template <typename T>
const Vector2<T>& Polygon2<T>::operator[] ( const Uint32& Pos ) const {
	if ( Vector.size() > 0 && Pos < Vector.size() )
		return Vector[Pos];
	return Vector[0];
}

template <typename T>
std::size_t Polygon2<T>::Size() const {
	return Vector.size();
}

template <typename T>
void Polygon2<T>::Rotate( const T& Angle, const Vector2<T>& Center ) {
	if ( Angle == 0.f )
		return;

	for ( eeUint i = 0; i < Vector.size(); i++ )
		Vector[ i ].RotateVectorCentered( Angle, Center );
}

template <typename T>
void Polygon2<T>::Scale( const T& scale, const Vector2<T>& Center ) {
	if ( scale == 1.0f )
		return;

	for ( Uint32 i = 0; i < Vector.size(); i++ ) {
		if ( Vector[i].x < Center.x )
			Vector[i].x = Center.x - eeabs( Center.x - Vector[i].x ) * scale;
		else
			Vector[i].x = Center.x + eeabs( Center.x - Vector[i].x ) * scale;

		if ( Vector[i].y < Center.y )
			Vector[i].y = Center.y - eeabs( Center.y - Vector[i].y ) * scale;
		else
			Vector[i].y = Center.y + eeabs( Center.y - Vector[i].y ) * scale;
	}
}

template<typename T>
Polygon2<T> Polygon2<T>::CreateRoundedRectangle( const T& x, const T& y, const T& width, const T& height, const eeUint& Radius ) {
	T PI05 = (T)EE_PI * 0.5f;
	T PI15 = (T)EE_PI * 1.5f;
	T PI20 = (T)EE_PI2;
	T sx, sy;
	T t;

	Polygon2<T> Poly;

	Poly.PushBack( Vector2<T>( x, y + height - Radius) );
	Poly.PushBack( Vector2<T>( x, y + Radius ) );

	for( t = (T)EE_PI; t < PI15; t += 0.1f ) {
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

	for( t = PI05; t < (T)EE_PI; t += 0.1f ) {
		sx = x + Radius + (eeFloat)cosf(t) * Radius;
		sy = y + height - Radius + (eeFloat)sinf(t) * Radius;

		Poly.PushBack( Vector2<T> (sx, sy) );
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
bool Polygon2<T>::PointInside( const Vector2<T>& point ) {
	int i, j, c = 0;
	int nvert = (int)Size();

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
bool Polygon2<T>::Intersect( const Polygon2<T>& p1 ) {
	T min0, max0, min1, max1, sOffset, t;
	Vector2<T> vAxis, vOffset;
	eeUint i = 0, j = 0, n, size = Size();

	vOffset = Vector2<T>( X() - p1.X(), Y() - p1.Y() );

	for (i = 0; i < size; i++) {
		n = i + 1;
		if ( n >= Size() ) n = 0;

		vAxis = Line2<T>( Vector[i], Vector[n] ).GetNormal();

		min0 = vAxis.Dot( Vector[0] );
		max0 = min0;
		for (j = 1; j < Size(); j++) {
			t = vAxis.Dot( Vector[j] );
			if (t < min0) min0 = t;
			if (t > max0) max0 = t;
		}

		min1 = vAxis.Dot( p1[0] );
		max1 = min1;
		for (j = 1; j < p1.Size(); j++) {
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

	for (i = 0; i < p1.Size(); i++) {
		n = i + 1;
		if ( n >= p1.Size() ) n = 0;

		vAxis = Line2<T>( p1[i], p1[n] ).GetNormal();

		min0 = vAxis.Dot( Vector[0] );
		max0 = min0;
		for (j = 1; j < Size(); j++) {
			t = vAxis.Dot( Vector[j] );
			if (t < min0) min0 = t;
			if (t > max0) max0 = t;
		}

		min1 = vAxis.Dot( p1[0] );
		max1 = min1;
		for (j = 1; j < p1.Size(); j++) {
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
bool Polygon2<T>::IntersectQuad2( const Quad2<T>& q0, const Quad2<T>& q1, const Vector2<T>& q0Pos, const Vector2<T>& q1Pos ) {
	Polygon2<T> Tmp1 = Polygon2<T>( q0 );
	Polygon2<T> Tmp2 = Polygon2<T>( q1 );

	Tmp1.Position( q0Pos );
	Tmp1.Position( q1Pos );

	return Tmp1.Intersect( Tmp2 );
}

template <typename T>
tRECT<T> Polygon2<T>::ToAABB() {
	tRECT<T> TmpR;

	if ( Vector.size() < 4 ) {
		return TmpR;
	}

	eeFloat MinX = Vector[0].x, MaxX = Vector[0].x, MinY = Vector[0].y, MaxY = Vector[0].y;

	for (Uint32 i = 1; i < Vector.size(); i++ ) {
		if ( MinX > Vector[i].x ) MinX = Vector[i].x;
		if ( MaxX < Vector[i].x ) MaxX = Vector[i].x;
		if ( MinY > Vector[i].y ) MinY = Vector[i].y;
		if ( MaxY < Vector[i].y ) MaxY = Vector[i].y;
	}

	TmpR.Left	= MinX + cOffsetX;
	TmpR.Right	= MaxX + cOffsetX;
	TmpR.Top	= MinY + cOffsetY;
	TmpR.Bottom	= MaxY + cOffsetY;

	return TmpR;
}

template <typename T>
void Polygon2<T>::Move( Vector2<T> dist ) {
	for ( Uint32 i = 0; i < Vector.size(); i++ ) {
		Vector[i] += dist;
	}
}

template <typename T>
Uint32 Polygon2<T>::ClosestPoint( const Vector2<T>& to, T * distance ) {
	Uint32 Index	= 0;
	T Dist			= (T)99999999;
	T tDist;

	if ( !Vector.size() ) {
		return eeINDEX_NOT_FOUND;
	}

	for ( Uint32 i = 0; i < Vector.size(); i++ ) {
		tDist = Vector[i].Distance( to );

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

typedef Polygon2<eeFloat> eePolygon2f;

}}

#endif
