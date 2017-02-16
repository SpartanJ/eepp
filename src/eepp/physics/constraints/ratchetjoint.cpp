#include <eepp/physics/constraints/ratchetjoint.hpp>

CP_NAMESPACE_BEGIN

RatchetJoint::RatchetJoint( Body * a, Body * b, cpFloat phase, cpFloat ratchet ) {
	mConstraint = cpRatchetJointNew( a->getBody(), b->getBody(), phase, ratchet );
	setData();
}

cpFloat RatchetJoint::angle() {
	return cpRatchetJointGetAngle( mConstraint );
}

void RatchetJoint::angle( const cpFloat& angle ) {
	cpRatchetJointSetAngle( mConstraint, angle );
}

cpFloat RatchetJoint::phase() {
	return cpRatchetJointGetPhase( mConstraint );
}

void RatchetJoint::phase( const cpFloat& phase ) {
	cpRatchetJointSetPhase( mConstraint, phase );
}

cpFloat RatchetJoint::ratchet() {
	return cpRatchetJointGetRatchet( mConstraint );
}

void RatchetJoint::ratchet( const cpFloat& ratchet ) {
	cpRatchetJointSetRatchet( mConstraint, ratchet );
}

void RatchetJoint::draw() {
	// Not implemented
}

CP_NAMESPACE_END
