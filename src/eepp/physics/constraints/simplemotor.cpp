#include <eepp/physics/constraints/simplemotor.hpp>

CP_NAMESPACE_BEGIN

SimpleMotor::SimpleMotor( Body * a, Body * b, cpFloat rate ) {
	mConstraint = cpSimpleMotorNew( a->getBody(), b->getBody(), rate );
	setData();
}

cpFloat SimpleMotor::rate() {
	return cpSimpleMotorGetRate( mConstraint );
}

void SimpleMotor::rate( const cpFloat& rate ) {
	cpSimpleMotorSetRate( mConstraint, rate );
}

void SimpleMotor::draw() {
	// Not implemented
}

CP_NAMESPACE_END
