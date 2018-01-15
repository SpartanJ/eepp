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
	mConstraint = cpSlideJointNew( a->getBody(), b->getBody(), tocpv( anchr1 ), tocpv( anchr2 ), min, max );
	setData();
}

cVect SlideJoint::getAnchr1() {
	return tovect( cpSlideJointGetAnchr1( mConstraint ) );
}

void SlideJoint::setAnchr1( const cVect& anchr1 ) {
	cpSlideJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect SlideJoint::getAnchr2() {
	return tovect( cpSlideJointGetAnchr2( mConstraint ) );
}

void SlideJoint::setAnchr2( const cVect& anchr2 ) {
	cpSlideJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cpFloat SlideJoint::getMin() {
	return cpSlideJointGetMin( mConstraint );
}

void SlideJoint::setMin( const cpFloat& min ) {
	cpSlideJointSetMin( mConstraint, min );
}

cpFloat SlideJoint::getMax() {
	return cpSlideJointGetMax( mConstraint );
}

void SlideJoint::setMax( const cpFloat& max ) {
	cpSlideJointSetMax( mConstraint, max );
}

void SlideJoint::draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cpSlideJoint *joint = (cpSlideJoint *)mConstraint;
	cVect a				= tovect( cpvadd( body_a->p, cpvrotate( joint->anchr1, body_a->rot ) ) );
	cVect b				= tovect( cpvadd( body_b->p, cpvrotate( joint->anchr2, body_b->rot ) ) );

	BatchRenderer * BR = GlobalBatchRenderer::instance();
	cpFloat ps			= BR->getPointSize();

	BR->setTexture( NULL );
	BR->setPointSize( mDrawPointSize );
	BR->pointsBegin();
	BR->pointSetColor( Color( 128, 255, 128, 255 ) );
	BR->batchPoint( a.x, a.y );
	BR->batchPoint( b.x, b.y );
	BR->draw();
	BR->linesBegin();
	BR->batchLine( a.x, a.y, b.x, b.y );
	BR->draw();
	BR->setPointSize( ps );
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat SlideJoint::getDrawPointSize() {
	return mDrawPointSize;
}

void SlideJoint::setDrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
