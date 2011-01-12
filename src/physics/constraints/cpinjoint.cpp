#include "cpinjoint.hpp"

namespace EE { namespace Physics {

cPinJoint::cPinJoint( cBody * a, cBody * b, cpVect anchr1, cpVect anchr2 ) {
	mConstraint = cpPinJointNew( a->Body(), b->Body(), anchr1, anchr2 );
	SetData();
}

cpVect cPinJoint::Anchr1() {
	return cpPinJointGetAnchr1( mConstraint );
}

void cPinJoint::Anchr1( const cpVect& anchr1 ) {
	cpPinJointSetAnchr1( mConstraint, anchr1 );
}

cpVect cPinJoint::Anchr2() {
	return cpPinJointGetAnchr2( mConstraint );
}

void cPinJoint::Anchr2( const cpVect& anchr2 ) {
	cpPinJointSetAnchr2( mConstraint, anchr2 );
}

cpFloat cPinJoint::Dist() {
	return cpPinJointGetDist( mConstraint );
}

void cPinJoint::Dist( const cpFloat& dist ) {
	cpPinJointSetDist( mConstraint, dist );
}

void cPinJoint::Draw() {
	cpPinJoint *joint = (cpPinJoint *)mConstraint;
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;

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
