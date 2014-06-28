#include <eepp/physics/constraints/rotarylimitjoint.hpp>

CP_NAMESPACE_BEGIN

RotaryLimitJoint::RotaryLimitJoint( Body * a, Body * b, cpFloat min, cpFloat max ) {
	mConstraint = cpRotaryLimitJointNew( a->GetBody(), b->GetBody(), min, max );
	SetData();
}

cpFloat RotaryLimitJoint::Min() {
	return cpRotaryLimitJointGetMin( mConstraint );
}

void RotaryLimitJoint::Min( const cpFloat& min ) {
	cpRotaryLimitJointSetMin( mConstraint, min );
}

cpFloat RotaryLimitJoint::Max() {
	return cpRotaryLimitJointGetMax( mConstraint );
}

void RotaryLimitJoint::Max( const cpFloat& max ) {
	cpRotaryLimitJointSetMax( mConstraint, max );
}

void RotaryLimitJoint::Draw() {
	// Not implemented
}

CP_NAMESPACE_END
