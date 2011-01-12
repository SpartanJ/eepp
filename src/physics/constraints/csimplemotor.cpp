#include "csimplemotor.hpp"

namespace EE { namespace Physics {

cSimpleMotor::cSimpleMotor( cBody * a, cBody * b, cpFloat rate ) {
	mConstraint = cpSimpleMotorNew( a->Body(), b->Body(), rate );
	SetData();
}

cpFloat cSimpleMotor::Rate() {
	return cpSimpleMotorGetRate( mConstraint );
}

void cSimpleMotor::Rate( const cpFloat& rate ) {
	cpSimpleMotorSetRate( mConstraint, rate );
}

void cSimpleMotor::Draw() {
	// Not implemented
}

}}
