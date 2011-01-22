#include "cdampedspring.hpp"

CP_NAMESPACE_BEGIN

cDampedSpring::cDampedSpring( cBody * a, cBody * b, cVect anchr1, cVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping ) {
	mConstraint = cpDampedSpringNew( a->Body(), b->Body(), tocpv( anchr1 ), tocpv( anchr2 ), restLength, stiffness, damping );
	SetData();
}

cVect cDampedSpring::Anchr1() {
	return tovect( cpDampedSpringGetAnchr1( mConstraint ) );
}

void cDampedSpring::Anchr1( const cVect& anchr1 ) {
	cpDampedSpringSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect cDampedSpring::Anchr2() {
	return tovect( cpDampedSpringGetAnchr2( mConstraint ) );
}

void cDampedSpring::Anchr2( const cVect& anchr2 ) {
	cpDampedSpringSetAnchr2( mConstraint, tocpv( anchr2 ) );
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
	#ifdef PHYSICS_RENDERER_ENABLED
	cpDampedSpring * spring = (cpDampedSpring*)mConstraint;
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;

	cVect a = tovect( cpvadd(body_a->p, cpvrotate(spring->anchr1, body_a->rot)) );
	cVect b = tovect( cpvadd(body_b->p, cpvrotate(spring->anchr2, body_b->rot)) );

	GLi->PointSize(5.0f);
	glBegin(GL_POINTS); {
		glVertex2f(a.x, a.y);
		glVertex2f(b.x, b.y);
	} glEnd();

	cVect delta = b - a;

	GLi->VertexPointer( 2, GL_FLOAT, 0, springVAR, springVAR_count * sizeof(GLfloat) * 2 );
	GLi->PushMatrix();

	GLfloat x = a.x;
	GLfloat y = a.y;
	GLfloat cos = delta.x;
	GLfloat sin = delta.y;
	GLfloat s = 1.0f / cpvlength( tocpv( delta ) );

	const GLfloat matrix[] = {
			 cos,    sin, 0.0f, 0.0f,
			-sin*s,  cos*s, 0.0f, 0.0f,
			0.0f,   0.0f, 1.0f, 0.0f,
				 x,      y, 0.0f, 1.0f,
	};

	glMultMatrixf(matrix);

	GLi->DrawArrays(GL_LINE_STRIP, 0, springVAR_count);

	GLi->PopMatrix();
	#endif
}

CP_NAMESPACE_END
