#include "cdampedrotaryspring.hpp"

namespace EE { namespace Physics {

cDampedRotarySpring::cDampedRotarySpring( cBody * a, cBody * b, cpFloat restAngle, cpFloat stiffness, cpFloat damping ) {
	mConstraint = cpDampedRotarySpringNew( a->Body(), b->Body(), restAngle, stiffness, damping );
	SetData();
}

cpFloat cDampedRotarySpring::RestAngle() {
	return cpDampedRotarySpringGetRestAngle( mConstraint );
}

void cDampedRotarySpring::RestAngle( const cpFloat& restangle ) {
	cpDampedRotarySpringSetRestAngle( mConstraint, restangle );
}

cpFloat cDampedRotarySpring::Stiffness() {
	return cpDampedRotarySpringGetStiffness( mConstraint );
}

void cDampedRotarySpring::Stiffness( const cpFloat& stiffness ) {
	cpDampedRotarySpringSetStiffness( mConstraint, stiffness );
}

cpFloat cDampedRotarySpring::Damping() {
	return cpDampedRotarySpringGetDamping( mConstraint );
}

void cDampedRotarySpring::Damping( const cpFloat& damping ) {
	cpDampedRotarySpringSetDamping( mConstraint, damping );
}

void cDampedRotarySpring::Draw() {
	// Not implemented
}

}}
