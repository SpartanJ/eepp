#include <eepp/physics/constraints/dampedrotaryspring.hpp>

CP_NAMESPACE_BEGIN

DampedRotarySpring::DampedRotarySpring( Body * a, Body * b, cpFloat restAngle, cpFloat stiffness, cpFloat damping ) {
	mConstraint = cpDampedRotarySpringNew( a->GetBody(), b->GetBody(), restAngle, stiffness, damping );
	SetData();
}

cpFloat DampedRotarySpring::RestAngle() {
	return cpDampedRotarySpringGetRestAngle( mConstraint );
}

void DampedRotarySpring::RestAngle( const cpFloat& restangle ) {
	cpDampedRotarySpringSetRestAngle( mConstraint, restangle );
}

cpFloat DampedRotarySpring::Stiffness() {
	return cpDampedRotarySpringGetStiffness( mConstraint );
}

void DampedRotarySpring::Stiffness( const cpFloat& stiffness ) {
	cpDampedRotarySpringSetStiffness( mConstraint, stiffness );
}

cpFloat DampedRotarySpring::Damping() {
	return cpDampedRotarySpringGetDamping( mConstraint );
}

void DampedRotarySpring::Damping( const cpFloat& damping ) {
	cpDampedRotarySpringSetDamping( mConstraint, damping );
}

void DampedRotarySpring::Draw() {
	// Not implemented
}

CP_NAMESPACE_END
