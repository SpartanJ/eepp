#ifndef EE_PHYSICS_MOMENT_HPP
#define EE_PHYSICS_MOMENT_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class CP_API Moment {
	public:
		static cpFloat ForCircle( cpFloat m, cpFloat r1, cpFloat r2, cVect offset );

		static cpFloat ForSegment( cpFloat m, cVect a, cVect b);

		static cpFloat ForPoly( cpFloat m, int numVerts, const cVect *verts, cVect offset );

		static cpFloat ForBox( cpFloat m, cpFloat width, cpFloat height );
};

CP_NAMESPACE_END

#endif
