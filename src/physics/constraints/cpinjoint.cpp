#include "cpinjoint.hpp"

#ifdef PHYSICS_RENDERER_ENABLED
#include "../../graphics/cglobalbatchrenderer.hpp"
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

cPinJoint::cPinJoint( cBody * a, cBody * b, cVect anchr1, cVect anchr2 ) {
	mConstraint = cpPinJointNew( a->Body(), b->Body(), tocpv( anchr1 ), tocpv( anchr2 ) );
	SetData();
}

cVect cPinJoint::Anchr1() {
	return tovect( cpPinJointGetAnchr1( mConstraint ) );
}

void cPinJoint::Anchr1( const cVect& anchr1 ) {
	cpPinJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect cPinJoint::Anchr2() {
	return tovect( cpPinJointGetAnchr2( mConstraint ) );
}

void cPinJoint::Anchr2( const cVect& anchr2 ) {
	cpPinJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cpFloat cPinJoint::Dist() {
	return cpPinJointGetDist( mConstraint );
}

void cPinJoint::Dist( const cpFloat& dist ) {
	cpPinJointSetDist( mConstraint, dist );
}

void cPinJoint::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	cpPinJoint *joint	= (cpPinJoint *)mConstraint;
	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cVect a				= tovect( cpvadd( body_a->p, cpvrotate( joint->anchr1, body_a->rot ) ) );
	cVect b				= tovect( cpvadd( body_b->p, cpvrotate(joint->anchr2, body_b->rot ) ) );
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	cpFloat ps = BR->GetPointSize();
	BR->SetTexture( NULL );
	BR->SetPointSize( 5.0f );
	BR->PointsBegin();
	BR->PointSetColor( eeColorA( 128, 255, 128, 255 ) );
	BR->BatchPoint( a.x, a.y );
	BR->BatchPoint( b.x, b.y );
	BR->Draw();

	BR->LinesBegin();
	BR->LinesSetColor( eeColorA( 128, 255, 128, 255 ) );
	BR->BatchLine( a.x, a.y, b.x, b.y );
	BR->Draw();

	BR->SetPointSize( ps );
	#endif
}

CP_NAMESPACE_END
