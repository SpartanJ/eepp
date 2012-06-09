#ifndef EE_PHYSICS_AREA_HPP
#define EE_PHYSICS_AREA_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class CP_API Area {
	public:

	inline static cpFloat ForCircle( cpFloat r1, cpFloat r2 ) {
		return cpAreaForCircle( r1, r2 );
	}

	inline static cpFloat ForSegment( cVect a, cVect b, cpFloat r ) {
		return cpAreaForSegment( tocpv( a ), tocpv( b ), r );
	}

	inline static cpFloat ForPoly( const int numVerts, const cVect * verts ) {
		return cpAreaForPoly( numVerts, constcasttocpv( verts ) );
	}
};

CP_NAMESPACE_END

#endif

