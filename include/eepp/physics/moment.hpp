#ifndef EE_PHYSICS_MOMENT_HPP
#define EE_PHYSICS_MOMENT_HPP

#include <eepp/physics/base.hpp>

CP_NAMESPACE_BEGIN

class CP_API Moment {
	public:
		public:

		inline static cpFloat ForCircle( cpFloat m, cpFloat r1, cpFloat r2, cVect offset ) {
			return cpMomentForCircle( m, r1, r2, tocpv( offset ) );
		}

		inline static cpFloat ForSegment( cpFloat m, cVect a, cVect b) {
			return cpMomentForSegment( m, tocpv( a ), tocpv( b ) );
		}

		inline static cpFloat ForPoly( cpFloat m, int numVerts, const cVect *verts, cVect offset ) {
			return cpMomentForPoly( m, numVerts, constcasttocpv( verts ), tocpv( offset ) );
		}

		inline static cpFloat ForBox( cpFloat m, cpFloat width, cpFloat height ) {
			return cpMomentForBox( m, width, height );
		}
};

CP_NAMESPACE_END

#endif
