#include <eepp/physics/constraints/ratchetjoint.hpp>

CP_NAMESPACE_BEGIN

RatchetJoint::RatchetJoint( Body * a, Body * b, cpFloat phase, cpFloat ratchet ) {
	mConstraint = cpRatchetJointNew( a->GetBody(), b->GetBody(), phase, ratchet );
	SetData();
}

cpFloat RatchetJoint::Angle() {
	return cpRatchetJointGetAngle( mConstraint );
}

void RatchetJoint::Angle( const cpFloat& angle ) {
	cpRatchetJointSetAngle( mConstraint, angle );
}

cpFloat RatchetJoint::Phase() {
	return cpRatchetJointGetPhase( mConstraint );
}

void RatchetJoint::Phase( const cpFloat& phase ) {
	cpRatchetJointSetPhase( mConstraint, phase );
}

cpFloat RatchetJoint::Ratchet() {
	return cpRatchetJointGetRatchet( mConstraint );
}

void RatchetJoint::Ratchet( const cpFloat& ratchet ) {
	cpRatchetJointSetRatchet( mConstraint, ratchet );
}

void RatchetJoint::Draw() {
	// Not implemented
}

CP_NAMESPACE_END
