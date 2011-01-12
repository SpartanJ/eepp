#include "cshapepoly.hpp"
#include "cspace.hpp"

namespace EE { namespace Physics {

cShapePoly::cShapePoly( cBody * body, int numVerts, cpVect *verts, cpVect offset ) {
	mShape = cpPolyShapeNew( body->Body(), numVerts, verts, offset );
	SetData();
}

cShapePoly::cShapePoly( cBody * body, cpFloat width, cpFloat height ) :
	cShape()
{
	mShape = cpBoxShapeNew( body->Body(), width, height );
}

bool cShapePoly::Validate( const cpVect * verts, const int numVerts ) {
	return 0 != cpPolyValidate( verts,  numVerts );
}

int cShapePoly::GetNumVerts() {
	return cpPolyShapeGetNumVerts( mShape );
}

cpVect cShapePoly::GetVert( int idx ) {
	return cpPolyShapeGetVert( mShape, idx );
}

void cShapePoly::SetVerts( int numVerts, cpVect *verts, cpVect offset ) {
	cpPolyShapeSetVerts( mShape, numVerts, verts, offset );
}

void cShapePoly::Draw( cSpace * space ) {
	cpPolyShape *poly = (cpPolyShape*)mShape;

	int count = poly->numVerts;
#if CP_USE_DOUBLES
	glVertexPointer(2, GL_DOUBLE, 0, poly->tVerts);
#else
	glVertexPointer(2, GL_FLOAT, 0, poly->tVerts);
#endif

	if(!poly->shape.sensor){
		glColor_for_shape((cpShape *)poly, space->Space());
		glDrawArrays(GL_TRIANGLE_FAN, 0, count);
	}

	glColor3f(LINE_COLOR);
	glDrawArrays(GL_LINE_LOOP, 0, count);
}

}}
