#include <eepp/physics/constraints/cdampedspring.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif


CP_NAMESPACE_BEGIN

cDampedSpring::cDampedSpring( cBody * a, cBody * b, cVect anchr1, cVect anchr2, cpFloat restLength, cpFloat stiffness, cpFloat damping )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 5.f )
#endif
{
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
	static const float springVAR[] = {
		0.00f, 0.0f,
		0.20f, 0.0f,
		0.25f, 3.0f,
		0.30f,-6.0f,
		0.35f, 6.0f,
		0.40f,-6.0f,
		0.45f, 6.0f,
		0.50f,-6.0f,
		0.55f, 6.0f,
		0.60f,-6.0f,
		0.65f, 6.0f,
		0.70f,-3.0f,
		0.75f, 6.0f,
		0.80f, 0.0f,
		1.00f, 0.0f,
	};
	static const int springVAR_count = sizeof(springVAR)/sizeof(float)/2;

	if ( mDrawPointSize <= 0 )
		return;

	cpDampedSpring * spring = (cpDampedSpring*)mConstraint;
	cpBody * body_a = mConstraint->a;
	cpBody * body_b = mConstraint->b;

	cVect a = tovect( cpvadd(body_a->p, cpvrotate(spring->anchr1, body_a->rot)) );
	cVect b = tovect( cpvadd(body_b->p, cpvrotate(spring->anchr2, body_b->rot)) );

	GLi->PointSize( mDrawPointSize );

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
	GLi->ColorPointer( 4, GL_UNSIGNED_BYTE, 0, reinterpret_cast<const GLvoid*>( &tcolors[0] ), springVAR_count * 4 );
	GLi->VertexPointer( 2, GL_FLOAT, 0, springVAR, springVAR_count * sizeof(GLfloat) * 2 );

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


#ifdef PHYSICS_RENDERER_ENABLED
cpFloat cDampedSpring::DrawPointSize() {
	return mDrawPointSize;
}

void cDampedSpring::DrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
