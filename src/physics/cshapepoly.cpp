#include "cshapepoly.hpp"
#include "cspace.hpp"

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

	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	BR->SetTexture( NULL );

	eeColorA Col = ColorForShape( (cpShape *)poly, space->Space() );

	// Could be a triangle fan
	BR->PointsBegin();
	BR->PolygonSetColor( Col );

	if( !poly->CP_PRIVATE(shape).sensor ){
		for ( int i = 0; i < poly->CP_PRIVATE(numVerts); i++ ) {
			BR->BatchPolygonByPoint( poly->CP_PRIVATE(tVerts)[i].x, poly->CP_PRIVATE(tVerts)[i].y );
		}

		BR->Draw();
	}

	BR->LineLoopSetColor( Col );

	for ( int i = 0; i < poly->CP_PRIVATE(numVerts); i++ ) {
		BR->BatchLineLoop( poly->CP_PRIVATE(tVerts)[i].x, poly->CP_PRIVATE(tVerts)[i].y );
	}

	BR->Draw();
	#endif
}

CP_NAMESPACE_END
