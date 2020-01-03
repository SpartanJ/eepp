#include <eepp/physics/constraints/ratchetjoint.hpp>

namespace EE { namespace Physics {

RatchetJoint::RatchetJoint( Body * a, Body * b, cpFloat phase, cpFloat ratchet ) {
	mConstraint = cpRatchetJointNew( a->getBody(), b->getBody(), phase, ratchet );
	setData();
}

cpFloat RatchetJoint::getAngle() {
	return cpRatchetJointGetAngle( mConstraint );
}

void RatchetJoint::setAngle( const cpFloat& angle ) {
	cpRatchetJointSetAngle( mConstraint, angle );
}

cpFloat RatchetJoint::getPhase() {
	return cpRatchetJointGetPhase( mConstraint );
}

void RatchetJoint::setPhase( const cpFloat& phase ) {
	cpRatchetJointSetPhase( mConstraint, phase );
}

cpFloat RatchetJoint::getRatchet() {
	return cpRatchetJointGetRatchet( mConstraint );
}

void RatchetJoint::setRatchet( const cpFloat& ratchet ) {
	cpRatchetJointSetRatchet( mConstraint, ratchet );
}

void RatchetJoint::draw() {
	// Not implemented
}

}}
