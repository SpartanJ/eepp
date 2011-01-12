#include "cratchetjoint.hpp"

namespace EE { namespace Physics {

cRatchetJoint::cRatchetJoint( cBody * a, cBody * b, cpFloat phase, cpFloat ratchet ) {
	mConstraint = cpRatchetJointNew( a->Body(), b->Body(), phase, ratchet );
	SetData();
}

cpFloat cRatchetJoint::Angle() {
	return cpRatchetJointGetAngle( mConstraint );
}

void cRatchetJoint::Angle( const cpFloat& angle ) {
	cpRatchetJointSetAngle( mConstraint, angle );
}

cpFloat cRatchetJoint::Phase() {
	return cpRatchetJointGetPhase( mConstraint );
}

void cRatchetJoint::Phase( const cpFloat& phase ) {
	cpRatchetJointSetPhase( mConstraint, phase );
}

cpFloat cRatchetJoint::Ratchet() {
	return cpRatchetJointGetRatchet( mConstraint );
}

void cRatchetJoint::Ratchet( const cpFloat& ratchet ) {
	cpRatchetJointSetRatchet( mConstraint, ratchet );
}

void cRatchetJoint::Draw() {
	// Not implemented
}

}}
