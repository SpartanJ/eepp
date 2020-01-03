#include <eepp/physics/constraints/simplemotor.hpp>

namespace EE { namespace Physics {

SimpleMotor::SimpleMotor( Body * a, Body * b, cpFloat rate ) {
	mConstraint = cpSimpleMotorNew( a->getBody(), b->getBody(), rate );
	setData();
}

cpFloat SimpleMotor::getRate() {
	return cpSimpleMotorGetRate( mConstraint );
}

void SimpleMotor::setRate( const cpFloat& rate ) {
	cpSimpleMotorSetRate( mConstraint, rate );
}

void SimpleMotor::draw() {
	// Not implemented
}

}}
