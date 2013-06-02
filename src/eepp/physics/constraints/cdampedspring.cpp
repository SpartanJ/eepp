#include <eepp/physics/constraints/cdampedspring.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif


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

	GLi->PointSize( 5.0f );

	cBatchRenderer * BR = cGlobalBatchRenderer::instance();
	BR->SetTexture( NULL );
	BR->PointsBegin();
	BR->BatchPoint( a.x, a.y );
	BR->BatchPoint( b.x, b.y );
	BR->Draw();

	cVect delta = b - a;

	GLi->Disable( GL_TEXTURE_2D );
	GLi->DisableClientState( GL_TEXTURE_COORD_ARRAY );

	std::vector<eeColorA> tcolors( springVAR_count * 4, eeColorA( 0, 255, 0, 255 ) );
	GLi->ColorPointer( 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<const GLvoid*>( &tcolors[0] ) );
	GLi->VertexPointer( 2, GL_FLOAT, 0, springVAR );

	GLi->PushMatrix();

	GLfloat x = a.x;
	GLfloat y = a.y;
	GLfloat cos = delta.x;
	GLfloat sin = delta.y;
	GLfloat s = 1.0f / cpvlength( tocpv( delta ) );

	const GLfloat matrix[] = {
		cos		, sin		, 0.0f, 0.0f,
		-sin * s, cos * s	, 0.0f, 0.0f,
		0.0f	, 0.0f		, 1.0f, 0.0f,
		 x		, y			, 0.0f, 1.0f,
	};
	GLi->MultMatrixf( matrix );

	GLi->DrawArrays( GL_LINE_STRIP, 0, springVAR_count );

	GLi->PopMatrix();

	GLi->Enable( GL_TEXTURE_2D );
	GLi->EnableClientState( GL_TEXTURE_COORD_ARRAY );
	#endif
}

CP_NAMESPACE_END
