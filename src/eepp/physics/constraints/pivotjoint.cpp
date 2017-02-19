#include <eepp/physics/constraints/pivotjoint.hpp>

#ifdef PHYSICS_RENDERER_ENABLED
#include <eepp/graphics/globalbatchrenderer.hpp>
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

PivotJoint::PivotJoint( Body * a, Body * b, cVect pivot )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 10.f )
#endif
{
	mConstraint = cpPivotJointNew( a->getBody(), b->getBody(), tocpv( pivot ) );
	setData();
}

PivotJoint::PivotJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 10.f )
#endif
{
	mConstraint = cpPivotJointNew2( a->getBody(), b->getBody(), tocpv( anchr1 ), tocpv( anchr2 ) );
	setData();
}

cVect PivotJoint::getAnchr1() {
	return tovect( cpPivotJointGetAnchr1( mConstraint ) );
}

void PivotJoint::setAnchr1( const cVect& anchr1 ) {
	cpPivotJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect PivotJoint::getAnchr2() {
	return tovect( cpPivotJointGetAnchr2( mConstraint ) );
}

void PivotJoint::setAnchr2( const cVect& anchr2 ) {
	cpPivotJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

void PivotJoint::draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cpPivotJoint* joint	= (cpPivotJoint *)mConstraint;
	cVect a				= tovect( cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot)) );
	cVect b				= tovect( cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot)) );
	BatchRenderer * BR = GlobalBatchRenderer::instance();

	cpFloat ps = BR->getPointSize();
	BR->setTexture( NULL );
	BR->setPointSize( mDrawPointSize );
	BR->pointsBegin();
	BR->pointSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->batchPoint( a.x, a.y );
	BR->batchPoint( b.x, b.y );
	BR->draw();
	BR->setPointSize( ps );
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat PivotJoint::getDrawPointSize() {
	return mDrawPointSize;
}

void PivotJoint::setDrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
