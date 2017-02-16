#include <eepp/physics/constraints/groovejoint.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

GrooveJoint::GrooveJoint( Body * a, Body * b, cVect groove_a, cVect groove_b, cVect anchr2 )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 5.f )
#endif
{
	mConstraint = cpGrooveJointNew( a->getBody(), b->getBody(), tocpv( groove_a ), tocpv( groove_b ), tocpv( anchr2 ) );
	setData();
}

cVect GrooveJoint::anchr2() {
	return tovect( cpGrooveJointGetAnchr2( mConstraint ) );
}

void GrooveJoint::anchr2( const cVect& anchr2 ) {
	cpGrooveJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cVect GrooveJoint::grooveA() {
	return tovect( cpGrooveJointGetGrooveA( mConstraint ) );
}

void GrooveJoint::grooveA( const cVect& groove_a ) {
	cpGrooveJointSetGrooveA( mConstraint, tocpv( groove_a ) );
}

cVect GrooveJoint::grooveB() {
	return tovect( cpGrooveJointGetGrooveB( mConstraint ) );
}

void GrooveJoint::grooveB( const cVect& groove_b ) {
	cpGrooveJointSetGrooveB( mConstraint, tocpv( groove_b ) );
}

void GrooveJoint::draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpGrooveJoint *joint= (cpGrooveJoint *)mConstraint;
	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cVect a				= tovect( cpvadd(body_a->p, cpvrotate(joint->grv_a, body_a->rot)) );
	cVect b				= tovect( cpvadd(body_a->p, cpvrotate(joint->grv_b, body_a->rot)) );
	cVect c				= tovect( cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot)) );
	BatchRenderer * BR = GlobalBatchRenderer::instance();

	cpFloat ps = BR->getPointSize();
	BR->setTexture( NULL );
	BR->setPointSize( mDrawPointSize );
	BR->pointsBegin();
	BR->pointSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->batchPoint( c.x, c.y );
	BR->draw();
	BR->linesBegin();
	BR->linesSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->batchLine( a.x, a.y, b.x, b.y );
	BR->draw();
	BR->setPointSize( ps );
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat GrooveJoint::drawPointSize() {
	return mDrawPointSize;
}

void GrooveJoint::drawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
