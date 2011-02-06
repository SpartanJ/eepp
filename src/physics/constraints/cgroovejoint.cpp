#include "cgroovejoint.hpp"

#ifdef PHYSICS_RENDERER_ENABLED
#include "../../graphics/cglobalbatchrenderer.hpp"
using namespace EE::Graphics;
#endif

CP_NAMESPACE_BEGIN

cGrooveJoint::cGrooveJoint( cBody * a, cBody * b, cVect groove_a, cVect groove_b, cVect anchr2 ) {
	mConstraint = cpGrooveJointNew( a->Body(), b->Body(), tocpv( groove_a ), tocpv( groove_b ), tocpv( anchr2 ) );
	SetData();
}

cVect cGrooveJoint::Anchr2() {
	return tovect( cpGrooveJointGetAnchr2( mConstraint ) );
}

void cGrooveJoint::Anchr2( const cVect& anchr2 ) {
	cpGrooveJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cVect cGrooveJoint::GrooveA() {
	return tovect( cpGrooveJointGetGrooveA( mConstraint ) );
}

void cGrooveJoint::GrooveA( const cVect& groove_a ) {
	cpGrooveJointSetGrooveA( mConstraint, tocpv( groove_a ) );
}

cVect cGrooveJoint::GrooveB() {
	return tovect( cpGrooveJointGetGrooveB( mConstraint ) );
}

void cGrooveJoint::GrooveB( const cVect& groove_b ) {
	cpGrooveJointSetGrooveB( mConstraint, tocpv( groove_b ) );
}

void cGrooveJoint::Draw() {
	#ifdef PHYSICS_RENDERER_ENABLED
	cpGrooveJoint *joint= (cpGrooveJoint *)mConstraint;
	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cVect a				= tovect( cpvadd(body_a->p, cpvrotate(joint->grv_a, body_a->rot)) );
	cVect b				= tovect( cpvadd(body_a->p, cpvrotate(joint->grv_b, body_a->rot)) );
	cVect c				= tovect( cpvadd(body_b->p, cpvrotate(joint->anchr2, body_b->rot)) );
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	cpFloat ps = BR->GetPointSize();
	BR->SetTexture( NULL );
	BR->SetPointSize( 5.0f );
	BR->PointsBegin();
	BR->PointSetColor( eeColorA( 128, 255, 128, 255 ) );
	BR->BatchPoint( c.x, c.y );
	BR->Draw();
	BR->LinesBegin();
	BR->LinesSetColor( eeColorA( 128, 255, 128, 255 ) );
	BR->BatchLine( a.x, a.y, b.x, b.y );
	BR->Draw();

	BR->SetPointSize( ps );
	#endif
}

CP_NAMESPACE_END
