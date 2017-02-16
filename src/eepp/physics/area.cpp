#include <eepp/physics/area.hpp>

CP_NAMESPACE_BEGIN

cpFloat Area::forCircle( cpFloat r1, cpFloat r2 ) {
	return cpAreaForCircle( r1, r2 );
}

cpFloat Area::forSegment( cVect a, cVect b, cpFloat r ) {
	return cpAreaForSegment( tocpv( a ), tocpv( b ), r );
}

cpFloat Area::forPoly( const int numVerts, const cVect * verts ) {
	return cpAreaForPoly( numVerts, constcasttocpv( verts ) );
}

CP_NAMESPACE_END
