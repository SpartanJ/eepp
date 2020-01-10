#include <eepp/physics/moment.hpp>

namespace EE { namespace Physics {

cpFloat Moment::forCircle( cpFloat m, cpFloat r1, cpFloat r2, cVect offset ) {
	return cpMomentForCircle( m, r1, r2, tocpv( offset ) );
}

cpFloat Moment::forSegment( cpFloat m, cVect a, cVect b ) {
	return cpMomentForSegment( m, tocpv( a ), tocpv( b ) );
}

cpFloat Moment::forPoly( cpFloat m, int numVerts, const cVect* verts, cVect offset ) {
	return cpMomentForPoly( m, numVerts, constcasttocpv( verts ), tocpv( offset ) );
}

cpFloat Moment::forBox( cpFloat m, cpFloat width, cpFloat height ) {
	return cpMomentForBox( m, width, height );
}

}} // namespace EE::Physics
