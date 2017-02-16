#include <eepp/physics/constraints/rotarylimitjoint.hpp>

CP_NAMESPACE_BEGIN

RotaryLimitJoint::RotaryLimitJoint( Body * a, Body * b, cpFloat min, cpFloat max ) {
	mConstraint = cpRotaryLimitJointNew( a->getBody(), b->getBody(), min, max );
	setData();
}

cpFloat RotaryLimitJoint::min() {
	return cpRotaryLimitJointGetMin( mConstraint );
}

void RotaryLimitJoint::min( const cpFloat& min ) {
	cpRotaryLimitJointSetMin( mConstraint, min );
}

cpFloat RotaryLimitJoint::max() {
	return cpRotaryLimitJointGetMax( mConstraint );
}

void RotaryLimitJoint::max( const cpFloat& max ) {
	cpRotaryLimitJointSetMax( mConstraint, max );
}

void RotaryLimitJoint::draw() {
	// Not implemented
}

CP_NAMESPACE_END
