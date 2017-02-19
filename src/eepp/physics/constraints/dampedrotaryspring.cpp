#include <eepp/physics/constraints/dampedrotaryspring.hpp>

CP_NAMESPACE_BEGIN

DampedRotarySpring::DampedRotarySpring( Body * a, Body * b, cpFloat restAngle, cpFloat stiffness, cpFloat damping ) {
	mConstraint = cpDampedRotarySpringNew( a->getBody(), b->getBody(), restAngle, stiffness, damping );
	setData();
}

cpFloat DampedRotarySpring::getRestAngle() {
	return cpDampedRotarySpringGetRestAngle( mConstraint );
}

void DampedRotarySpring::setRestAngle( const cpFloat& restangle ) {
	cpDampedRotarySpringSetRestAngle( mConstraint, restangle );
}

cpFloat DampedRotarySpring::getStiffness() {
	return cpDampedRotarySpringGetStiffness( mConstraint );
}

void DampedRotarySpring::setStiffness( const cpFloat& stiffness ) {
	cpDampedRotarySpringSetStiffness( mConstraint, stiffness );
}

cpFloat DampedRotarySpring::getDamping() {
	return cpDampedRotarySpringGetDamping( mConstraint );
}

void DampedRotarySpring::setDamping( const cpFloat& damping ) {
	cpDampedRotarySpringSetDamping( mConstraint, damping );
}

void DampedRotarySpring::draw() {
	// Not implemented
}

CP_NAMESPACE_END
