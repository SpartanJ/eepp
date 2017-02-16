#include <eepp/physics/shapepoly.hpp>
#include <eepp/physics/space.hpp>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

ShapePoly * ShapePoly::New( Physics::Body * body, int numVerts, cVect *verts, cVect offset ) {
	return cpNew( ShapePoly, ( body, numVerts, verts, offset ) );
}

ShapePoly * ShapePoly::New( Physics::Body * body, cpFloat width, cpFloat height ) {
	return cpNew( ShapePoly, ( body, width, height ) );
}

ShapePoly::ShapePoly( Physics::Body * body, int numVerts, cVect *verts, cVect offset ) {
	mShape = cpPolyShapeNew( body->GetBody(), numVerts, casttocpv( verts ), tocpv( offset ) );
	SetData();
}

ShapePoly::ShapePoly( Physics::Body * body, cpFloat width, cpFloat height ) :
	Shape()
{
	mShape = cpBoxShapeNew( body->GetBody(), width, height );
	SetData();
}

bool ShapePoly::Validate( const cVect * verts, const int numVerts ) {
	return 0 != cpPolyValidate( constcasttocpv( verts ),  numVerts );
}

int ShapePoly::GetNumVerts() {
	return cpPolyShapeGetNumVerts( mShape );
}

cVect ShapePoly::GetVert( int idx ) {
	return tovect( cpPolyShapeGetVert( mShape, idx ) );
}

void ShapePoly::SetVerts( int numVerts, cVect *verts, cVect offset ) {
	cpPolyShapeSetVerts( mShape, numVerts, casttocpv( verts ), tocpv( offset ) );
}

void ShapePoly::Recenter( int numVerts, cVect *verts ) {
	cpRecenterPoly( numVerts, casttocpv( verts ) );
}

cVect ShapePoly::Centroid( int numVerts, const cVect * verts ) {
	return tovect( cpCentroidForPoly( numVerts, constcasttocpv( verts ) ) );
}

void ShapePoly::Draw( Space * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	cpPolyShape * poly = (cpPolyShape*)mShape;

	BatchRenderer * BR = GlobalBatchRenderer::instance();

	BR->setTexture( NULL );

	ColorA Col = ColorForShape( (cpShape *)poly, space->GetSpace() );

	if( !poly->CP_PRIVATE(shape).sensor ){
		if ( 4 != poly->CP_PRIVATE(numVerts) ) {
			BR->pointsBegin();
			BR->polygonSetColor( Col );

			for ( int i = 0; i < poly->CP_PRIVATE(numVerts); i++ ) {
				BR->batchPolygonByPoint( poly->CP_PRIVATE(tVerts)[i].x, poly->CP_PRIVATE(tVerts)[i].y );
			}
		} else {
			BR->quadsBegin();
			BR->quadsSetColor( Col );

			BR->batchQuadFreeEx(poly->CP_PRIVATE(tVerts)[0].x, poly->CP_PRIVATE(tVerts)[0].y, poly->CP_PRIVATE(tVerts)[1].x, poly->CP_PRIVATE(tVerts)[1].y, poly->CP_PRIVATE(tVerts)[2].x, poly->CP_PRIVATE(tVerts)[2].y, poly->CP_PRIVATE(tVerts)[3].x, poly->CP_PRIVATE(tVerts)[3].y );
		}

		BR->drawOpt();
	}


	#endif
}

void ShapePoly::DrawBorder( Space *space ) {
#ifdef PHYSICS_RENDERER_ENABLED
	cpPolyShape * poly = (cpPolyShape*)mShape;

	BatchRenderer * BR = GlobalBatchRenderer::instance();

	ColorA Col = ColorForShape( (cpShape *)poly, space->GetSpace() );

	BR->lineLoopBegin();
	BR->lineLoopSetColor( Col );

	for ( int i = 0; i < poly->CP_PRIVATE(numVerts); i++ ) {
		BR->batchLineLoop( poly->CP_PRIVATE(tVerts)[i].x, poly->CP_PRIVATE(tVerts)[i].y );
	}

	BR->draw();
#endif
}

CP_NAMESPACE_END
