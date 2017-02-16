#include <eepp/physics/constraints/gearjoint.hpp>

CP_NAMESPACE_BEGIN

GearJoint::GearJoint( Body * a, Body * b, cpFloat phase, cpFloat ratio ) {
	mConstraint = cpGearJointNew( a->getBody(), b->getBody(), phase, ratio );
	setData();
}

cpFloat GearJoint::phase() {
	return cpGearJointGetPhase( mConstraint );
}

void GearJoint::phase( const cpFloat& phase ) {
	cpGearJointSetPhase( mConstraint, phase );
}

cpFloat GearJoint::ratio() {
	return cpGearJointGetRatio( mConstraint );
}

void GearJoint::ratio( const cpFloat& ratio ) {
	cpGearJointSetRatio( mConstraint, ratio );
}

void GearJoint::draw() {
	// Not implemented
}

CP_NAMESPACE_END
