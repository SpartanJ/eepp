#include <eepp/physics/constraints/simplemotor.hpp>

CP_NAMESPACE_BEGIN

SimpleMotor::SimpleMotor( Body * a, Body * b, cpFloat rate ) {
	mConstraint = cpSimpleMotorNew( a->GetBody(), b->GetBody(), rate );
	SetData();
}

cpFloat SimpleMotor::Rate() {
	return cpSimpleMotorGetRate( mConstraint );
}

void SimpleMotor::Rate( const cpFloat& rate ) {
	cpSimpleMotorSetRate( mConstraint, rate );
}

void SimpleMotor::Draw() {
	// Not implemented
}

CP_NAMESPACE_END
