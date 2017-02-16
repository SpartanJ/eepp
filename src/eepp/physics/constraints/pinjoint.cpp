#include <eepp/physics/constraints/pinjoint.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

PinJoint::PinJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 5.f )
#endif
{
	mConstraint = cpPinJointNew( a->GetBody(), b->GetBody(), tocpv( anchr1 ), tocpv( anchr2 ) );
	SetData();
}

cVect PinJoint::Anchr1() {
	return tovect( cpPinJointGetAnchr1( mConstraint ) );
}

void PinJoint::Anchr1( const cVect& anchr1 ) {
	cpPinJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect PinJoint::Anchr2() {
	return tovect( cpPinJointGetAnchr2( mConstraint ) );
}

void PinJoint::Anchr2( const cVect& anchr2 ) {
	cpPinJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cpFloat PinJoint::Dist() {
	return cpPinJointGetDist( mConstraint );
}

void PinJoint::Dist( const cpFloat& dist ) {
	cpPinJointSetDist( mConstraint, dist );
}

void PinJoint::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpPinJoint *joint	= (cpPinJoint *)mConstraint;
	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cVect a				= tovect( cpvadd( body_a->p, cpvrotate( joint->anchr1, body_a->rot ) ) );
	cVect b				= tovect( cpvadd( body_b->p, cpvrotate(joint->anchr2, body_b->rot ) ) );
	BatchRenderer * BR = GlobalBatchRenderer::instance();

	cpFloat ps = BR->getPointSize();
	BR->setTexture( NULL );
	BR->setPointSize( mDrawPointSize );
	BR->pointsBegin();
	BR->pointSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->batchPoint( a.x, a.y );
	BR->batchPoint( b.x, b.y );
	BR->draw();

	BR->linesBegin();
	BR->linesSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->batchLine( a.x, a.y, b.x, b.y );
	BR->draw();

	BR->setPointSize( ps );
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat PinJoint::DrawPointSize() {
	return mDrawPointSize;
}

void PinJoint::DrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
