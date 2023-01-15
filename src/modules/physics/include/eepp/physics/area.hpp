#ifndef EE_PHYSICS_AREA_HPP
#define EE_PHYSICS_AREA_HPP

#include <eepp/physics/base.hpp>

namespace EE { namespace Physics {

class EE_PHYSICS_API Area {
  public:
	cpFloat forCircle( cpFloat r1, cpFloat r2 );

	cpFloat forSegment( cVect a, cVect b, cpFloat r );

	cpFloat forPoly( const int numVerts, const cVect* verts );
};

}} // namespace EE::Physics

#endif
