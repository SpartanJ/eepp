#include "cpivotjoint.hpp"

namespace EE { namespace Physics {

cPivotJoint::cPivotJoint( cBody * a, cBody * b, cpVect pivot ) {
	mConstraint = cpPivotJointNew( a->Body(), b->Body(), pivot );
	SetData();
}

cPivotJoint::cPivotJoint( cBody * a, cBody * b, cpVect anchr1, cpVect anchr2 ) {
	mConstraint = cpPivotJointNew2( a->Body(), b->Body(), anchr1, anchr2 );
	SetData();
}

cpVect cPivotJoint::Anchr1() {
	return cpPivotJointGetAnchr1( mConstraint );
}

void cPivotJoint::Anchr1( const cpVect& anchr1 ) {
	cpPivotJointSetAnchr1( mConstraint, anchr1 );
}

cpVect cPivotJoint::Anchr2() {
	return cpPivotJointGetAnchr2( mConstraint );
}

void cPivotJoint::Anchr2( const cpVect& anchr2 ) {
	cpPivotJointSetAnchr2( mConstraint, anchr2 );
}

void cPivotJoint::Draw() {
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;
	cpPivotJoint *joint = (cpPivotJoint *)mConstraint;

	cpVect a = cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot));
	cpVect b = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));

	glPointSize(10.0f);
	glBegin(GL_POINTS); {
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();
}

}}
