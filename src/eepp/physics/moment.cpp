#include <eepp/physics/moment.hpp>

CP_NAMESPACE_BEGIN

cpFloat Moment::ForCircle( cpFloat m, cpFloat r1, cpFloat r2, cVect offset ) {
	return cpMomentForCircle( m, r1, r2, tocpv( offset ) );
}

cpFloat Moment::ForSegment( cpFloat m, cVect a, cVect b) {
	return cpMomentForSegment( m, tocpv( a ), tocpv( b ) );
}

cpFloat Moment::ForPoly( cpFloat m, int numVerts, const cVect *verts, cVect offset ) {
	return cpMomentForPoly( m, numVerts, constcasttocpv( verts ), tocpv( offset ) );
}

cpFloat Moment::ForBox( cpFloat m, cpFloat width, cpFloat height ) {
	return cpMomentForBox( m, width, height );
}
	
CP_NAMESPACE_END
