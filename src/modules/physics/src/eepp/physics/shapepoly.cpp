#include <eepp/physics/shapepoly.hpp>
#include <eepp/physics/space.hpp>
#include <eepp/thirdparty/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

namespace EE { namespace Physics {

ShapePoly* ShapePoly::New( Physics::Body* body, int numVerts, cVect* verts, cVect offset ) {
	return eeNew( ShapePoly, ( body, numVerts, verts, offset ) );
}

ShapePoly* ShapePoly::New( Physics::Body* body, cpFloat width, cpFloat height ) {
	return eeNew( ShapePoly, ( body, width, height ) );
}

ShapePoly::ShapePoly( Physics::Body* body, int numVerts, cVect* verts, cVect offset ) {
	mShape = cpPolyShapeNew( body->getBody(), numVerts, casttocpv( verts ), tocpv( offset ) );
	setData();
}

ShapePoly::ShapePoly( Physics::Body* body, cpFloat width, cpFloat height ) : Shape() {
	mShape = cpBoxShapeNew( body->getBody(), width, height );
	setData();
}

bool ShapePoly::validate( const cVect* verts, const int numVerts ) {
	return 0 != cpPolyValidate( constcasttocpv( verts ), numVerts );
}

int ShapePoly::getNumVerts() {
	return cpPolyShapeGetNumVerts( mShape );
}

cVect ShapePoly::getVert( int idx ) {
	return tovect( cpPolyShapeGetVert( mShape, idx ) );
}

void ShapePoly::setVerts( int numVerts, cVect* verts, cVect offset ) {
	cpPolyShapeSetVerts( mShape, numVerts, casttocpv( verts ), tocpv( offset ) );
}

void ShapePoly::recenter( int numVerts, cVect* verts ) {
	cpRecenterPoly( numVerts, casttocpv( verts ) );
}

cVect ShapePoly::centroid( int numVerts, const cVect* verts ) {
	return tovect( cpCentroidForPoly( numVerts, constcasttocpv( verts ) ) );
}

void ShapePoly::draw( Space* space ) {
#ifdef PHYSICS_RENDERER_ENABLED
	cpPolyShape* poly = (cpPolyShape*)mShape;

	BatchRenderer* BR = GlobalBatchRenderer::instance();

	BR->setTexture( NULL );

	Color Col = colorForShape( (cpShape*)poly, space->getSpace() );

	if ( !poly->CP_PRIVATE( shape ).sensor ) {
		if ( 4 != poly->CP_PRIVATE( numVerts ) ) {
			BR->pointsBegin();
			BR->polygonSetColor( Col );

			for ( int i = 0; i < poly->CP_PRIVATE( numVerts ); i++ ) {
				BR->batchPolygonByPoint( poly->CP_PRIVATE( tVerts )[i].x,
										 poly->CP_PRIVATE( tVerts )[i].y );
			}
		} else {
			BR->quadsBegin();
			BR->quadsSetColor( Col );

			BR->batchQuadFreeEx( poly->CP_PRIVATE( tVerts )[0].x, poly->CP_PRIVATE( tVerts )[0].y,
								 poly->CP_PRIVATE( tVerts )[1].x, poly->CP_PRIVATE( tVerts )[1].y,
								 poly->CP_PRIVATE( tVerts )[2].x, poly->CP_PRIVATE( tVerts )[2].y,
								 poly->CP_PRIVATE( tVerts )[3].x, poly->CP_PRIVATE( tVerts )[3].y );
		}

		BR->drawOpt();
	}

#endif
}

void ShapePoly::drawBorder( Space* space ) {
#ifdef PHYSICS_RENDERER_ENABLED
	cpPolyShape* poly = (cpPolyShape*)mShape;

	BatchRenderer* BR = GlobalBatchRenderer::instance();

	Color Col = colorForShape( (cpShape*)poly, space->getSpace() );

	BR->lineLoopBegin();
	BR->lineLoopSetColor( Col );

	for ( int i = 0; i < poly->CP_PRIVATE( numVerts ); i++ ) {
		BR->batchLineLoop( poly->CP_PRIVATE( tVerts )[i].x, poly->CP_PRIVATE( tVerts )[i].y );
	}

	BR->draw();
#endif
}

}} // namespace EE::Physics
