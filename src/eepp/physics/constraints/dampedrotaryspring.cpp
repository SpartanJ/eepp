#include <eepp/physics/constraints/dampedrotaryspring.hpp>

CP_NAMESPACE_BEGIN

DampedRotarySpring::DampedRotarySpring( Body * a, Body * b, cpFloat restAngle, cpFloat stiffness, cpFloat damping ) {
	mConstraint = cpDampedRotarySpringNew( a->getBody(), b->getBody(), restAngle, stiffness, damping );
	setData();
}

cpFloat DampedRotarySpring::restAngle() {
	return cpDampedRotarySpringGetRestAngle( mConstraint );
}

void DampedRotarySpring::restAngle( const cpFloat& restangle ) {
	cpDampedRotarySpringSetRestAngle( mConstraint, restangle );
}

cpFloat DampedRotarySpring::stiffness() {
	return cpDampedRotarySpringGetStiffness( mConstraint );
}

void DampedRotarySpring::stiffness( const cpFloat& stiffness ) {
	cpDampedRotarySpringSetStiffness( mConstraint, stiffness );
}

cpFloat DampedRotarySpring::damping() {
	return cpDampedRotarySpringGetDamping( mConstraint );
}

void DampedRotarySpring::damping( const cpFloat& damping ) {
	cpDampedRotarySpringSetDamping( mConstraint, damping );
}

void DampedRotarySpring::draw() {
	// Not implemented
}

CP_NAMESPACE_END
