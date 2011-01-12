#include "cslidejoint.hpp"

namespace EE { namespace Physics {

cSlideJoint::cSlideJoint( cBody * a, cBody *b, cpVect anchr1, cpVect anchr2, cpFloat min, cpFloat max ) {
	mConstraint = cpSlideJointNew( a->Body(), b->Body(), anchr1, anchr2, min, max );
	SetData();
}

cpVect cSlideJoint::Anchr1() {
	return cpSlideJointGetAnchr1( mConstraint );
}

void cSlideJoint::Anchr1( const cpVect& anchr1 ) {
	cpSlideJointSetAnchr1( mConstraint, anchr1 );
}

cpVect cSlideJoint::Anchr2() {
	return cpSlideJointGetAnchr2( mConstraint );
}

void cSlideJoint::Anchr2( const cpVect& anchr2 ) {
	cpSlideJointSetAnchr2( mConstraint, anchr2 );
}

cpFloat cSlideJoint::Min() {
	return cpSlideJointGetMin( mConstraint );
}

void cSlideJoint::Min( const cpFloat& min ) {
	cpSlideJointSetMin( mConstraint, min );
}

cpFloat cSlideJoint::Max() {
	return cpSlideJointGetMax( mConstraint );
}

void cSlideJoint::Max( const cpFloat& max ) {
	cpSlideJointSetMax( mConstraint, max );
}

void cSlideJoint::Draw() {
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;
	cpSlideJoint *joint = (cpSlideJoint *)mConstraint;

	cpVect a = cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot));
	cpVect b = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));

	glPointSize(5.0f);
	glBegin(GL_POINTS); {
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();

	glBegin(GL_LINES); {
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();
}

}}
