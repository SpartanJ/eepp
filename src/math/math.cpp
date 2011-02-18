#include "math.hpp"

namespace EE { namespace Math {

Uint32 SetRandomSeed() {
	Uint32 Seed = static_cast<Uint32>( GetSystemTime() * 1000 );
	srand(Seed);
	return Seed;
}

eeFloat eeRandf( const eeFloat& fMin, const eeFloat& fMax ) {
	return (fMin + (fMax - fMin) * ( rand() / ( (eeFloat) RAND_MAX + 1) ) );
}

eeInt eeRandi( const eeInt& fMin, const eeInt& fMax ) {
	return (eeInt)(fMin + (fMax - fMin + 1) * ( rand() / ( (eeFloat) RAND_MAX + 1) ) );
}

eeFloat cosAng( const eeFloat& Ang ) {
	return eecos(Ang * EE_PI_180);
}

eeFloat sinAng( const eeFloat& Ang ) {
	return eesin(Ang * EE_PI_180);
}

eeFloat tanAng( const eeFloat& Ang ) {
	return tan(Ang * EE_PI_180);
}

eeFloat Radians( const eeFloat& Ang ) {
	return Ang * EE_PI_180;
}

eeFloat Degrees( const eeFloat& Radians ) {
	return Radians * EE_180_PI;
}

eeFloat RotatePointFromX ( const eeFloat& x, const eeFloat& y, const eeFloat& Angle ) {
	return x * cosAng(Angle) - y * sinAng(Angle);
}

eeFloat RotatePointFromY ( const eeFloat& x, const eeFloat& y, const eeFloat& Angle ) {
	return y * cosAng(Angle) + x * sinAng(Angle);
}

eeFloat RotatePointFromX ( const eeVector2f& p, const eeFloat& Angle ) {
	return RotatePointFromX( p.x, p.y, Angle );
}

eeFloat RotatePointFromY ( const eeVector2f& p, const eeFloat& Angle ) {
	return RotatePointFromY( p.x, p.y, Angle );
}

eeVector2f RotateVector( const eeVector2f& p, const eeFloat& Angle ) {
	return eeVector2f( RotatePointFromX( p, Angle ), RotatePointFromY( p, Angle ) );
}

void RotateVector( eeVector2f* p, const eeFloat& Angle ) {
	eeFloat x = p->x;
	x = p->x * cosAng(Angle) - p->y * sinAng(Angle);
	p->y = p->y * cosAng(Angle) + p->x * sinAng(Angle);
	p->x = x;
}

void RotateVectorCentered( eeVector2f* p, const eeFloat& Angle, const eeVector2f& RotationCenter ) {
	*p -= RotationCenter;
	RotateVector( p, Angle );
	*p += RotationCenter;
}

eeVector2f RotateVectorCentered( const eeVector2f& p, const eeFloat& Angle, const eeVector2f& RotationCenter ) {
	return RotationCenter + RotateVector( (p - RotationCenter), Angle );
}

eeQuad2f RotateQuadCentered( const eeQuad2f& p, const eeFloat& Angle, const eeVector2f& RotationCenter ) {
	return eeQuad2f( RotateVectorCentered( p.V[0], Angle, RotationCenter ), RotateVectorCentered( p.V[1], Angle, RotationCenter ), RotateVectorCentered( p.V[2], Angle, RotationCenter ), RotateVectorCentered( p.V[3], Angle, RotationCenter ) );
}

eeVector2f GetQuadCenter( const eeQuad2f& Q ) {
	eeVector2f QCenter;
	eeFloat MinX = Q.V[0].x, MaxX = Q.V[0].x, MinY = Q.V[0].y, MaxY = Q.V[0].y;

	for (Uint8 i = 1; i < 4; i++ ) {
		if ( MinX > Q.V[i].x ) MinX = Q.V[i].x;
		if ( MaxX < Q.V[i].x ) MaxX = Q.V[i].x;
		if ( MinY > Q.V[i].y ) MinY = Q.V[i].y;
		if ( MaxY < Q.V[i].y ) MaxY = Q.V[i].y;
	}

	QCenter.x = MinX + ( MaxX - MinX ) * 0.5f;
	QCenter.y = MinY + ( MaxY - MinY ) * 0.5f;

	return QCenter;
}

eeQuad2f ScaleQuadCentered( const eeQuad2f& Quad, const eeFloat& Scale, const eeVector2f& RotationCenter ) {
	eeQuad2f mQ = Quad;
	eeVector2f QCenter = RotationCenter;

	for (Uint8 i = 0; i < 4; i++ ) {
		if ( mQ.V[i].x < QCenter.x )
			mQ.V[i].x = QCenter.x - eeabs(QCenter.x - mQ.V[i].x) * Scale;
		else
			mQ.V[i].x = QCenter.x + eeabs(QCenter.x - mQ.V[i].x) * Scale;

		if ( mQ.V[i].y < QCenter.y )
			mQ.V[i].y = QCenter.y - eeabs(QCenter.y - mQ.V[i].y) * Scale;
		else
			mQ.V[i].y = QCenter.y + eeabs(QCenter.y - mQ.V[i].y) * Scale;
	}

	return mQ;
}

eeFloat LineAngle( const eeVector2f& p1, const eeVector2f& p2 ) {
	return LineAngle( p1.x, p1.y, p2.x, p2.y );
}

eeFloat Distance( const eeVector2f& p1, const eeVector2f& p2) {
	return Distance( p1.x, p1.y, p2.x, p2.y );
}

bool IntersectLines( const eeFloat& Ax, const eeFloat& Ay, const eeFloat& Bx, const eeFloat& By, const eeFloat& Cx, const eeFloat& Cy, const eeFloat& Dx, const eeFloat& Dy, eeFloat *X, eeFloat *Y) {
	return IntersectLines<eeFloat> (Ax, Ay, Bx, By, Cx, Cy, Dx, Dy, X, Y);
}

bool IntersectLines( const eeVector2f& l1p1, const eeVector2f& l1p2, const eeVector2f& l2p1, const eeVector2f& l2p2, eeFloat *X, eeFloat *Y) {
	return IntersectLines<eeFloat> (l1p1.x, l1p1.y, l1p2.x, l1p2.y, l2p1.x, l2p1.y, l2p2.x, l2p2.y, X, Y);
}

eeFloat eeFastInvSqrt( eeFloat x ) {
    union { int i; eeFloat x; } tmp;
    eeFloat xhalf = 0.5f * x;
    tmp.x = x;
    tmp.i = 0x5f375a86 - (tmp.i >> 1);
    x = tmp.x;
    x = x * (1.5f - xhalf * x * x);
    return x;
}

eeFloat eeFastSqrt( eeFloat x ) {
    return 1.0f / eeFastInvSqrt(x);
}

}}
