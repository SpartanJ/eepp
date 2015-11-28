#ifndef EE_PHYSICS_HELPER
#define EE_PHYSICS_HELPER

#include <eepp/physics/base.hpp>
#include <eepp/graphics/base.hpp>

CP_NAMESPACE_BEGIN

const cpFloat cpPI		= 3.14159265358979323846;
const cpFloat cpPI_180	= cpPI / 180;
const cpFloat cp180_PI	= 180 / cpPI;

inline static cpFloat cpRadians( const cpFloat& Ang ) {
	return Ang * cpPI_180;
}

inline static cpFloat cpDegrees( const cpFloat& Radians ) {
	return Radians * cp180_PI;
}

#ifdef USE_EE_VECTOR

typedef Vector2<cpFloat>		cVect;

inline static cVect toVect( cpVect vect ) {
	return cVect( vect.x, vect.y );
}

#define tocpv( vect )			cpv( vect.x, vect.y )
#define tovect( vect )			toVect( vect )
#define casttocpv( vect )		reinterpret_cast<cpVect*>( vect )
#define constcasttocpv( vect )	reinterpret_cast<const cpVect*>( vect )
#define cVectZero				cVect( 0, 0 )
#define cVectNew( x, y )		cVect( x, y )

#else

typedef cpVect					cVect;
#define tocpv( vect )			vect
#define tovect( vect )			vect
#define casttocpv( vect )		vect
#define constcasttocpv( vect )	vect
#define cVectZero				cpvzero
#define cVectNew( x, y )		cpv( x, y )

#endif

#ifdef USE_EE_AABB

typedef tRECT<cpFloat>			cBB;

inline static cBB toAABB( cpBB bb ) {
#ifdef BB_INVERT_Y_AXIS
	return cBB( bb.l, bb.b, bb.r, bb.t );
#else
	return cBB( bb.l, bb.t, bb.r, bb.b );
#endif
}

#define tocpbb( bb )			cpBBNew( bb.Left, bb.Top, bb.Right, bb.Bottom )
#define tocbb( bb )				toAABB( bb )
#define cBBNew( l, t, r, b )	cBB( l, t, r, b )

#else

typedef cpBB					cBB;

#define tocpbb( bb )			bb
#define tocbb( bb )				bb

#ifdef BB_INVERT_Y_AXIS
#define cBBNew( l, t, r, b )	cpBBNew( l, t, r, b ) //! Inverted Top/Bottom here too
#else
#define cBBNew( l, t, r, b )	cpBBNew( l, b, r, t )
#endif

#endif

#ifdef PHYSICS_RENDERER_ENABLED

inline ColorA ColorFromPointer(void *ptr) {
	unsigned long val = (long)ptr;

	// hash the pointer up nicely
	val = (val+0x7ed55d16) + (val<<12);
	val = (val^0xc761c23c) ^ (val>>19);
	val = (val+0x165667b1) + (val<<5);
	val = (val+0xd3a2646c) ^ (val<<9);
	val = (val+0xfd7046c5) + (val<<3);
	val = (val^0xb55a4f09) ^ (val>>16);

	unsigned char r = (val>>0) & 0xFF;
	unsigned char g = (val>>8) & 0xFF;
	unsigned char b = (val>>16) & 0xFF;

	unsigned char max = r>g ? (r>b ? r : b) : (g>b ? g : b);

	const int mult = 127;
	const int add = 63;
	r = (r*mult)/max + add;
	g = (g*mult)/max + add;
	b = (b*mult)/max + add;

	return ColorA(r, g, b, 255);
}

inline ColorA ColorForShape( cpShape *shape, cpSpace *space ) {
	cpBody *body = shape->body;
	int nc;

	if(body){
		if(cpBodyIsSleeping(body)){
			float v = 0.25f;
			nc = (int)( v * 255 );
			return ColorA( nc, nc, nc, 255 );
		} else if(body->CP_PRIVATE(node).idleTime > space->sleepTimeThreshold) {
			float v = 0.9f;
			nc = (int)( v * 255 );
			return ColorA( nc, nc, nc, 255 );
		}
	}

	return ColorFromPointer( shape );
}

#endif

CP_NAMESPACE_END

#endif
