#include <eepp/physics/constraints/gearjoint.hpp>

CP_NAMESPACE_BEGIN

GearJoint::GearJoint( Body * a, Body * b, cpFloat phase, cpFloat ratio ) {
	mConstraint = cpGearJointNew( a->GetBody(), b->GetBody(), phase, ratio );
	SetData();
}

cpFloat GearJoint::Phase() {
	return cpGearJointGetPhase( mConstraint );
}

void GearJoint::Phase( const cpFloat& phase ) {
	cpGearJointSetPhase( mConstraint, phase );
}

cpFloat GearJoint::Ratio() {
	return cpGearJointGetRatio( mConstraint );
}

void GearJoint::Ratio( const cpFloat& ratio ) {
	cpGearJointSetRatio( mConstraint, ratio );
}

void GearJoint::Draw() {
	// Not implemented
}

CP_NAMESPACE_END
