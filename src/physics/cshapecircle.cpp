#include "cshapecircle.hpp"
#include "cspace.hpp"

namespace EE { namespace Physics {

cShapeCircle::cShapeCircle( cBody * body, cpFloat radius, cpVect offset ) {
	mShape	= cpCircleShapeNew( body->Body(), radius, offset );
	SetData();
}

cpVect cShapeCircle::Offset() {
	return cpCircleShapeGetOffset( mShape );
}

void cShapeCircle::Offset( const cpVect &offset ) {
	cpCircleShapeSetOffset( mShape, offset );
}

cpFloat cShapeCircle::Radius() {
	return cpCircleShapeGetRadius( mShape );
}

void cShapeCircle::Radius( const cpFloat& radius ) {
	cpCircleShapeSetRadius( mShape, radius );
}

void cShapeCircle::Draw( cSpace * space ) {
	cPrimitives p;

	cpCircleShape * cs = (cpCircleShape*)mShape;

	p.DrawCircle( cs->tc.x, cs->tc.y, cs->r );

	/*glVertexPointer( 2, GL_FLOAT, 0, circleVAR );

	cpCircleShape * cs = (cpCircleShape*)mShape;

	glPushMatrix();

	cpVect center = cs->tc;

	glTranslatef( center.x, center.y, 0.0f );
	glRotatef( space->StaticBody()->Body()->a * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);

	glScalef( cs->r, cs->r, 1.0f);

	if(!cs->shape.sensor){
		glColor_for_shape( mShape, space->Space() );
		glDrawArrays(GL_TRIANGLE_FAN, 0, circleVAR_count - 1);
	}

	glColor3f(LINE_COLOR);
	glDrawArrays(GL_LINE_STRIP, 0, circleVAR_count);
	glPopMatrix();*/
}

}}
