#ifndef EE_PHYSICS_BASE
#define EE_PHYSICS_BASE

#include "../base.hpp"

#include "../math/math.hpp"
using namespace EE::Math;

#include "../window/cengine.hpp"
using namespace EE::Window;

#include "../system/tsingleton.hpp"
using namespace EE::System;

#include "../graphics/cprimitives.hpp"
#include "../graphics/cbatchrenderer.hpp"
#include "../graphics/cglobalbatchrenderer.hpp"

#include "../helper/chipmunk/chipmunk_private.h"
#include "../helper/chipmunk/chipmunk_unsafe.h"

#define USE_EE_VECTOR

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

#include "physicshelper.hpp"

#endif
