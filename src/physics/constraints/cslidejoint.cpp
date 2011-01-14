#include "cslidejoint.hpp"

namespace EE { namespace Physics {

cSlideJoint::cSlideJoint( cBody * a, cBody *b, cVect anchr1, cVect anchr2, cpFloat min, cpFloat max ) {
	mConstraint = cpSlideJointNew( a->Body(), b->Body(), tocpv( anchr1 ), tocpv( anchr2 ), min, max );
	SetData();
}

cVect cSlideJoint::Anchr1() {
	return tovect( cpSlideJointGetAnchr1( mConstraint ) );
}

void cSlideJoint::Anchr1( const cVect& anchr1 ) {
	cpSlideJointSetAnchr1( mConstraint, tocpv( anchr1 ) );
}

cVect cSlideJoint::Anchr2() {
	return tovect( cpSlideJointGetAnchr2( mConstraint ) );
}

void cSlideJoint::Anchr2( const cVect& anchr2 ) {
	cpSlideJointSetAnchr2( mConstraint, tocpv( anchr2 ) );
}

cpFloat cSlideJoint::Min() {
	return cpSlideJointGetMin( mConstraint );
}

void cSlideJoint::Min( const cpFloat& min ) {
	cpSlideJointSetMin( mConstraint, min );
}

cpFloat cSlideJoint::Max() {
	return cpSlideJointGetMax( mConstraint );
}

void cSlideJoint::Max( const cpFloat& max ) {
	cpSlideJointSetMax( mConstraint, max );
}

void cSlideJoint::Draw() {
	cpBody * body_a		= mConstraint->a;
	cpBody * body_b		= mConstraint->b;
	cpSlideJoint *joint = (cpSlideJoint *)mConstraint;
	cVect a				= tovect( cpvadd( body_a->p, cpvrotate( joint->anchr1, body_a->rot ) ) );
	cVect b				= tovect( cpvadd( body_b->p, cpvrotate( joint->anchr2, body_b->rot ) ) );

	cBatchRenderer * BR = cGlobalBatchRenderer::instance();
	cpFloat ps			= BR->GetPointSize();

	BR->SetPointSize( 5.0f );
	BR->PointsBegin();
	BR->PointSetColor( eeColorA( 128, 255, 128, 255 ) );
	BR->BatchPoint( a.x, a.y );
	BR->BatchPoint( b.x, b.y );
	BR->DrawOpt();
	BR->LinesBegin();
	BR->BatchLine( a.x, a.y, b.x, b.y );
	BR->Draw();
	BR->SetPointSize( ps );
}

}}
