#ifndef EE_PHYSICS_MOMENT_HPP
#define EE_PHYSICS_MOMENT_HPP

#include <eepp/physics/base.hpp>

namespace EE { namespace Physics {

class EE_API Moment {
	public:
		static cpFloat forCircle( cpFloat m, cpFloat r1, cpFloat r2, cVect offset );

		static cpFloat forSegment( cpFloat m, cVect a, cVect b);

		static cpFloat forPoly( cpFloat m, int numVerts, const cVect *verts, cVect offset );

		static cpFloat forBox( cpFloat m, cpFloat width, cpFloat height );
};

}}

#endif
