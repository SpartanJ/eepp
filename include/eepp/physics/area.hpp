#ifndef EE_PHYSICS_AREA_HPP
#define EE_PHYSICS_AREA_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class CP_API Area {
	public:
		cpFloat ForCircle( cpFloat r1, cpFloat r2 );

		cpFloat ForSegment( cVect a, cVect b, cpFloat r );

		cpFloat ForPoly( const int numVerts, const cVect * verts );
};

CP_NAMESPACE_END

#endif
