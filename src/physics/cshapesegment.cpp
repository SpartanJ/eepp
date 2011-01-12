#include "cshapesegment.hpp"
#include "cspace.hpp"

namespace EE { namespace Physics {

cShapeSegment::cShapeSegment( cBody * body, cpVect a, cpVect b, cpFloat radius ) {
	mShape = cpSegmentShapeNew( body->Body(), a, b, radius );
	SetData();
}

cpVect cShapeSegment::A() {
	return cpSegmentShapeGetA( mShape );
}

cpVect cShapeSegment::B() {
	return cpSegmentShapeGetB( mShape );
}

cpVect cShapeSegment::Normal() {
	return cpSegmentShapeGetNormal( mShape );
}

cpFloat cShapeSegment::Radius() {
	return cpSegmentShapeGetRadius( mShape );
}

void cShapeSegment::Radius( const cpFloat& radius ) {
	cpSegmentShapeSetRadius( mShape, radius );
}

void cShapeSegment::Endpoints( const cpVect& a, const cpVect& b ) {
	cpSegmentShapeSetEndpoints( mShape, a, b );
}

bool cShapeSegment::Query( cpVect a, cpVect b, cpSegmentQueryInfo * info ) {
	return 0 != cpShapeSegmentQuery( mShape, a, b, info );
}

cpVect cShapeSegment::HitPoint( const cpVect start, const cpVect end, const cpSegmentQueryInfo info ) {
	return cpSegmentQueryHitPoint( start, end, info );
}

cpFloat cShapeSegment::HitDist( const cpVect start, const cpVect end, const cpSegmentQueryInfo info ) {
	return cpSegmentQueryHitDist( start, end, info );
}

void cShapeSegment::Draw( cSpace * space ) {
	cPrimitives p;

	cpSegmentShape * seg = (cpSegmentShape *)mShape;
	cpVect a = seg->ta;
	cpVect b = seg->tb;

	p.DrawLine( eeVector2f( a.x, a.y ), eeVector2f( b.x, b.y ) );
	/*cpSegmentShape * seg = (cpSegmentShape *)mShape;

	cpVect a = seg->ta;
	cpVect b = seg->tb;

	if(seg->r){
		glVertexPointer(3, GL_FLOAT, 0, pillVAR);
		glPushMatrix(); {
			cpVect d = cpvsub(b, a);
			cpVect r = cpvmult(d, seg->r/cpvlength(d));

			const GLfloat matrix[] = {
				 r.x, r.y, 0.0f, 0.0f,
				-r.y, r.x, 0.0f, 0.0f,
				 d.x, d.y, 0.0f, 0.0f,
				 a.x, a.y, 0.0f, 1.0f,
			};
			glMultMatrixf(matrix);

			if(!seg->shape.sensor){
				glColor_for_shape((cpShape *)seg, space->Space());
				glDrawArrays(GL_TRIANGLE_FAN, 0, pillVAR_count);
			}

			glColor3f(LINE_COLOR);
			glDrawArrays(GL_LINE_LOOP, 0, pillVAR_count);
		} glPopMatrix();
	} else {
		glColor3f(LINE_COLOR);
		glBegin(GL_LINES); {
			glVertex2f(a.x, a.y);
			glVertex2f(b.x, b.y);
		} glEnd();
	}*/
}

}}
