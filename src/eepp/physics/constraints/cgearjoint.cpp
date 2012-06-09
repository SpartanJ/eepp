#include <eepp/physics/constraints/cgearjoint.hpp>

CP_NAMESPACE_BEGIN

cGearJoint::cGearJoint( cBody * a, cBody * b, cpFloat phase, cpFloat ratio ) {
	mConstraint = cpGearJointNew( a->Body(), b->Body(), phase, ratio );
	SetData();
}

cpFloat cGearJoint::Phase() {
	return cpGearJointGetPhase( mConstraint );
}

void cGearJoint::Phase( const cpFloat& phase ) {
	cpGearJointSetPhase( mConstraint, phase );
}

cpFloat cGearJoint::Ratio() {
	return cpGearJointGetRatio( mConstraint );
}

void cGearJoint::Ratio( const cpFloat& ratio ) {
	cpGearJointSetRatio( mConstraint, ratio );
}

void cGearJoint::Draw() {
	// Not implemented
}

CP_NAMESPACE_END
