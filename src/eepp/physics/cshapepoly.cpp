#include <eepp/physics/cshapepoly.hpp>
#include <eepp/physics/cspace.hpp>
#include <eepp/helper/chipmunk/chipmunk_unsafe.h>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

cShapePoly * cShapePoly::New( cBody * body, int numVerts, cVect *verts, cVect offset ) {
	return cpNew( cShapePoly, ( body, numVerts, verts, offset ) );
}

cShapePoly * cShapePoly::New( cBody * body, cpFloat width, cpFloat height ) {
	return cpNew( cShapePoly, ( body, width, height ) );
}

cShapePoly::cShapePoly( cBody * body, int numVerts, cVect *verts, cVect offset ) {
	mShape = cpPolyShapeNew( body->Body(), numVerts, casttocpv( verts ), tocpv( offset ) );
	SetData();
}

cShapePoly::cShapePoly( cBody * body, cpFloat width, cpFloat height ) :
	cShape()
{
	mShape = cpBoxShapeNew( body->Body(), width, height );
	SetData();
}

bool cShapePoly::Validate( const cVect * verts, const int numVerts ) {
	return 0 != cpPolyValidate( constcasttocpv( verts ),  numVerts );
}

int cShapePoly::GetNumVerts() {
	return cpPolyShapeGetNumVerts( mShape );
}

cVect cShapePoly::GetVert( int idx ) {
	return tovect( cpPolyShapeGetVert( mShape, idx ) );
}

void cShapePoly::SetVerts( int numVerts, cVect *verts, cVect offset ) {
	cpPolyShapeSetVerts( mShape, numVerts, casttocpv( verts ), tocpv( offset ) );
}

void cShapePoly::Recenter( int numVerts, cVect *verts ) {
	cpRecenterPoly( numVerts, casttocpv( verts ) );
}

cVect cShapePoly::Centroid( int numVerts, const cVect * verts ) {
	return tovect( cpCentroidForPoly( numVerts, constcasttocpv( verts ) ) );
}

void cShapePoly::Draw( cSpace * space ) {
	#ifdef PHYSICS_RENDERER_ENABLED
	cpPolyShape * poly = (cpPolyShape*)mShape;

	BatchRenderer * BR = GlobalBatchRenderer::instance();

	BR->SetTexture( NULL );

	ColorA Col = ColorForShape( (cpShape *)poly, space->Space() );

	if( !poly->CP_PRIVATE(shape).sensor ){
		if ( 4 != poly->CP_PRIVATE(numVerts) ) {
			BR->PointsBegin();
			BR->PolygonSetColor( Col );

			for ( int i = 0; i < poly->CP_PRIVATE(numVerts); i++ ) {
				BR->BatchPolygonByPoint( poly->CP_PRIVATE(tVerts)[i].x, poly->CP_PRIVATE(tVerts)[i].y );
			}
		} else {
			BR->QuadsBegin();
			BR->QuadsSetColor( Col );

			BR->BatchQuadFreeEx(poly->CP_PRIVATE(tVerts)[0].x, poly->CP_PRIVATE(tVerts)[0].y, poly->CP_PRIVATE(tVerts)[1].x, poly->CP_PRIVATE(tVerts)[1].y, poly->CP_PRIVATE(tVerts)[2].x, poly->CP_PRIVATE(tVerts)[2].y, poly->CP_PRIVATE(tVerts)[3].x, poly->CP_PRIVATE(tVerts)[3].y );
		}

		BR->DrawOpt();
	}


	#endif
}

void cShapePoly::DrawBorder( cSpace *space ) {
#ifdef PHYSICS_RENDERER_ENABLED
	cpPolyShape * poly = (cpPolyShape*)mShape;

	BatchRenderer * BR = GlobalBatchRenderer::instance();

	ColorA Col = ColorForShape( (cpShape *)poly, space->Space() );

	BR->LineLoopBegin();
	BR->LineLoopSetColor( Col );

	for ( int i = 0; i < poly->CP_PRIVATE(numVerts); i++ ) {
		BR->BatchLineLoop( poly->CP_PRIVATE(tVerts)[i].x, poly->CP_PRIVATE(tVerts)[i].y );
	}

	BR->Draw();
#endif
}

CP_NAMESPACE_END
