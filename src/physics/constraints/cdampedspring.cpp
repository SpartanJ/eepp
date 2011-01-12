#include "cdampedspring.hpp"

namespace EE { namespace Physics {

cDampedSpring::cDampedSpring( cBody * a, cBody * b, cpVect anchr1, cpVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping ) {
	mConstraint = cpDampedSpringNew( a->Body(), b->Body(), anchr1, anchr2, restLength, stiffness, damping );
	SetData();
}

cpVect cDampedSpring::Anchr1() {
	return cpDampedSpringGetAnchr1( mConstraint );
}

void cDampedSpring::Anchr1( const cpVect& anchr1 ) {
	cpDampedSpringSetAnchr1( mConstraint, anchr1 );
}

cpVect cDampedSpring::Anchr2() {
	return cpDampedSpringGetAnchr2( mConstraint );
}

void cDampedSpring::Anchr2( const cpVect& anchr2 ) {
	cpDampedSpringSetAnchr2( mConstraint, anchr2 );
}

cpFloat cDampedSpring::RestLength() {
	return cpDampedSpringGetRestLength( mConstraint );
}

void cDampedSpring::RestLength( const cpFloat& restlength ) {
	cpDampedSpringSetRestLength( mConstraint, restlength );
}

cpFloat cDampedSpring::Stiffness() {
	return cpDampedSpringGetStiffness( mConstraint );
}

void cDampedSpring::Stiffness( const cpFloat& stiffness ) {
	cpDampedSpringSetStiffness( mConstraint, stiffness );
}

cpFloat cDampedSpring::Damping() {
	return cpDampedSpringGetDamping( mConstraint );
}

void cDampedSpring::Damping( const cpFloat& damping ) {
	cpDampedSpringSetDamping( mConstraint, damping );
}

void cDampedSpring::Draw() {
	cpDampedSpring * spring = (cpDampedSpring*)mConstraint;
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;

	cpVect a = cpvadd(body_a->p, cpvrotate(spring->anchr1, body_a->rot));
	cpVect b = cpvadd(body_b->p, cpvrotate(spring->anchr2, body_b->rot));

	glPointSize(5.0f);
	glBegin(GL_POINTS); {
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();

	cpVect delta = cpvsub(b, a);

	glVertexPointer(2, GL_FLOAT, 0, springVAR);
	glPushMatrix(); {
		GLfloat x = a.x;
		GLfloat y = a.y;
		GLfloat cos = delta.x;
		GLfloat sin = delta.y;
		GLfloat s = 1.0f/cpvlength(delta);

		const GLfloat matrix[] = {
				 cos,    sin, 0.0f, 0.0f,
			-sin*s,  cos*s, 0.0f, 0.0f,
				0.0f,   0.0f, 1.0f, 0.0f,
					 x,      y, 0.0f, 1.0f,
		};

		glMultMatrixf(matrix);
		glDrawArrays(GL_LINE_STRIP, 0, springVAR_count);
	} glPopMatrix();
}

}}
