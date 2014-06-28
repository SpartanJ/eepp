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
	mConstraint = cpPivotJointNew( a->GetBody(), b->GetBody(), tocpv( pivot ) );
	SetData();
}

PivotJoint::PivotJoint( Body * a, Body * b, cVect anchr1, cVect anchr2 )
#ifdef PHYSICS_RENDERER_ENABLED
	: mDrawPointSize( 10.f )
#endif
{
	mConstraint = cpPivotJointNew2( a->GetBody(), b->GetBody(), tocpv( anchr1 ), tocpv( anchr2 ) );
	SetData();
}

cVect PivotJoint::Anchr1() {
	return tovect( cpPivotJointGetAnchr1( mConstraint ) );
}

void PivotJoint::Anchr1( const cVect& anchr1 ) {
	cpPivotJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect PivotJoint::Anchr2() {
	return tovect( cpPivotJointGetAnchr2( mConstraint ) );
}

void PivotJoint::Anchr2( const cVect& anchr2 ) {
	cpPivotJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

void PivotJoint::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	if ( mDrawPointSize <= 0 )
		return;

	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cpPivotJoint* joint	= (cpPivotJoint *)mConstraint;
	cVect a				= tovect( cpvadd(body_a->p, cpvrotate(joint->anchr1, body_a->rot)) );
	cVect b				= tovect( cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot)) );
	BatchRenderer * BR = GlobalBatchRenderer::instance();

	cpFloat ps = BR->GetPointSize();
	BR->SetTexture( NULL );
	BR->SetPointSize( mDrawPointSize );
	BR->PointsBegin();
	BR->PointSetColor( ColorA( 128, 255, 128, 255 ) );
	BR->BatchPoint( a.x, a.y );
	BR->BatchPoint( b.x, b.y );
	BR->Draw();
	BR->SetPointSize( ps );
	#endif
}

#ifdef PHYSICS_RENDERER_ENABLED
cpFloat PivotJoint::DrawPointSize() {
	return mDrawPointSize;
}

void PivotJoint::DrawPointSize( const cpFloat& size ) {
	mDrawPointSize = size;
}
#endif

CP_NAMESPACE_END
