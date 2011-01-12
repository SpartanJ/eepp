#include "cgroovejoint.hpp"

namespace EE { namespace Physics {

cGrooveJoint::cGrooveJoint( cBody * a, cBody * b, cpVect groove_a, cpVect groove_b, cpVect anchr2 ) {
	mConstraint = cpGrooveJointNew( a->Body(), b->Body(), groove_a, groove_b, anchr2 );
	SetData();
}

cpVect cGrooveJoint::Anchr2() {
	return cpGrooveJointGetAnchr2( mConstraint );
}

void cGrooveJoint::Anchr2( const cpVect& anchr2 ) {
	cpGrooveJointSetAnchr2( mConstraint, anchr2 );
}

cpVect cGrooveJoint::GrooveA() {
	return cpGrooveJointGetGrooveA( mConstraint );
}

void cGrooveJoint::GrooveA( const cpVect& groove_a ) {
	cpGrooveJointSetGrooveA( mConstraint, groove_a );
}

cpVect cGrooveJoint::GrooveB() {
	return cpGrooveJointGetGrooveB( mConstraint );
}

void cGrooveJoint::GrooveB( const cpVect& groove_b ) {
	cpGrooveJointSetGrooveB( mConstraint, groove_b );
}

void cGrooveJoint::Draw() {
	cpGrooveJoint *joint = (cpGrooveJoint *)mConstraint;
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;

	cpVect a = cpvadd(body_a->p, cpvrotate(joint->grv_a, body_a->rot));
	cpVect b = cpvadd(body_a->p, cpvrotate(joint->grv_b, body_a->rot));
	cpVect c = cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot));

	glPointSize(5.0f);
	glBegin(GL_POINTS); {
		glVertex2f(c.x, c.y);
	} glEnd();

	glBegin(GL_LINES); {
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();
}

}}
