#include <eepp/physics/constraints/slidejoint.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

SlideJoint::SlideJoint( Body * a, Body *b, cVect anchr1, cVect anchr2, cpFloat min, cpFloat max )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 5.f )
#endif
{
	mConstraint = cpSlideJointNew( a->GetBody(), b->GetBody(), tocpv( anchr1 ), tocpv( anchr2 ), min, max );
	SetData();
}

cVect SlideJoint::Anchr1() {
	return tovect( cpSlideJointGetAnchr1( mConstraint ) );
}

void SlideJoint::Anchr1( const cVect& anchr1 ) {
	cpSlideJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect SlideJoint::Anchr2() {
	return tovect( cpSlideJointGetAnchr2( mConstraint ) );
}

void SlideJoint::Anchr2( const cVect& anchr2 ) {
	cpSlideJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cpFloat SlideJoint::Min() {
	return cpSlideJointGetMin( mConstraint );
}

void SlideJoint::Min( const cpFloat& min ) {
	cpSlideJointSetMin( mConstraint, min );
}

cpFloat SlideJoint::Max() {
	return cpSlideJointGetMax( mConstraint );
}

void SlideJoint::Max( const cpFloat& max ) {
	cpSlideJointSetMax( mConstraint, max );
}

void SlideJoint::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cpSlideJoint *joint = (cpSlideJoint *)mConstraint;
	cVect a				= tovect( cpvadd( body_a->p, cpvrotate( joint->anchr1, body_a->rot ) ) );
	cVect b				= tovect( cpvadd( body_b->p, cpvrotate( joint->anchr2, body_b->rot ) ) );

	BatchRenderer * BR = GlobalBatchRenderer::instance();
	cpFloat ps			= BR->GetPointSize();

	BR->SetTexture( NULL );
	BR->SetPointSize( mDrawPointSize );
	BR->PointsBegin();
	BR->PointSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->BatchPoint( a.x, a.y );
	BR->BatchPoint( b.x, b.y );
	BR->Draw();
	BR->LinesBegin();
	BR->BatchLine( a.x, a.y, b.x, b.y );
	BR->Draw();
	BR->SetPointSize( ps );
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat SlideJoint::DrawPointSize() {
	return mDrawPointSize;
}

void SlideJoint::DrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
