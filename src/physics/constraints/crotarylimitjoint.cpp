#include "crotarylimitjoint.hpp"

namespace EE { namespace Physics {

cRotaryLimitJoint::cRotaryLimitJoint( cBody * a, cBody * b, cpFloat min, cpFloat max ) {
	mConstraint = cpRotaryLimitJointNew( a->Body(), b->Body(), min, max );
	SetData();
}

cpFloat cRotaryLimitJoint::Min() {
	return cpRotaryLimitJointGetMin( mConstraint );
}

void cRotaryLimitJoint::Min( const cpFloat& min ) {
	cpRotaryLimitJointSetMin( mConstraint, min );
}

cpFloat cRotaryLimitJoint::Max() {
	return cpRotaryLimitJointGetMax( mConstraint );
}

void cRotaryLimitJoint::Max( const cpFloat& max ) {
	cpRotaryLimitJointSetMax( mConstraint, max );
}

void cRotaryLimitJoint::Draw() {
	// Not implemented
}

}}
