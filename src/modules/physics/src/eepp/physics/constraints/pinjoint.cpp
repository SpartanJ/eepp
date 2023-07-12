#include <eepp/physics/constraints/pinjoint.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

namespace EE { namespace Physics {

PinJoint::PinJoint( Body* a, Body* b, cVect anchr1, cVect anchr2 )
#ifdef PHYSICS_RENDERER_ENABLED
	:
	mDrawPointSize( 5.f )
#endif
{
	mConstraint = cpPinJointNew( a->getBody(), b->getBody(), tocpv( anchr1 ), tocpv( anchr2 ) );
	setData();
}

cVect PinJoint::getAnchr1() {
	return tovect( cpPinJointGetAnchr1( mConstraint ) );
}

void PinJoint::setAnchr1( const cVect& anchr1 ) {
	cpPinJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect PinJoint::getAnchr2() {
	return tovect( cpPinJointGetAnchr2( mConstraint ) );
}

void PinJoint::setAnchr2( const cVect& anchr2 ) {
	cpPinJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cpFloat PinJoint::getDist() {
	return cpPinJointGetDist( mConstraint );
}

void PinJoint::setDist( const cpFloat& dist ) {
	cpPinJointSetDist( mConstraint, dist );
}

void PinJoint::draw() {
#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpPinJoint* joint = (cpPinJoint*)mConstraint;
	cpBody* body_a = mConstraint->a;
	cpBody* body_b = mConstraint->b;
	cVect a = tovect( cpvadd( body_a->p, cpvrotate( joint->anchr1, body_a->rot ) ) );
	cVect b = tovect( cpvadd( body_b->p, cpvrotate( joint->anchr2, body_b->rot ) ) );
	BatchRenderer* BR = GlobalBatchRenderer::instance();

	cpFloat ps = BR->getPointSize();
	BR->setTexture( NULL );
	BR->setPointSize( mDrawPointSize );
	BR->pointsBegin();
	BR->pointSetColor( Color( 128, 255, 128, 255 ) );
	BR->batchPoint( a.x, a.y );
	BR->batchPoint( b.x, b.y );
	BR->draw();

	BR->linesBegin();
	BR->linesSetColor( Color( 128, 255, 128, 255 ) );
	BR->batchLine( a.x, a.y, b.x, b.y );
	BR->draw();

	BR->setPointSize( ps );
#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat PinJoint::getDrawPointSize() {
	return mDrawPointSize;
}

void PinJoint::setDrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

}} // namespace EE::Physics
