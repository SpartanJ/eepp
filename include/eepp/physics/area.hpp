#ifndef EE_PHYSICS_AREA_HPP
#define EE_PHYSICS_AREA_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class CP_API Area {
	public:
		cpFloat forCircle( cpFloat r1, cpFloat r2 );

		cpFloat forSegment( cVect a, cVect b, cpFloat r );

		cpFloat forPoly( const int numVerts, const cVect * verts );
};

CP_NAMESPACE_END

#endif
