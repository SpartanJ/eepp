#include <eepp/physics/constraints/rotarylimitjoint.hpp>

namespace EE { namespace Physics {

RotaryLimitJoint::RotaryLimitJoint( Body * a, Body * b, cpFloat min, cpFloat max ) {
	mConstraint = cpRotaryLimitJointNew( a->getBody(), b->getBody(), min, max );
	setData();
}

cpFloat RotaryLimitJoint::getMin() {
	return cpRotaryLimitJointGetMin( mConstraint );
}

void RotaryLimitJoint::setMin( const cpFloat& min ) {
	cpRotaryLimitJointSetMin( mConstraint, min );
}

cpFloat RotaryLimitJoint::getMax() {
	return cpRotaryLimitJointGetMax( mConstraint );
}

void RotaryLimitJoint::setMax( const cpFloat& max ) {
	cpRotaryLimitJointSetMax( mConstraint, max );
}

void RotaryLimitJoint::draw() {
	// Not implemented
}

}}
