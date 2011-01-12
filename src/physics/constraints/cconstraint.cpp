#include "cconstraint.hpp"

namespace EE { namespace Physics {

cConstraint::cConstraint( cpConstraint * Constraint ) {
	mConstraint = Constraint;
	SetData();
}

cConstraint::cConstraint() {
}

cConstraint::~cConstraint() {
}

void cConstraint::SetData() {
	mConstraint->data = (void*)this;
}

cpConstraint * cConstraint::Constraint() const {
	return mConstraint;
}

cBody * cConstraint::A() {
	return reinterpret_cast<cBody*>( mConstraint->a->data );
}

cBody * cConstraint::B() {
	return reinterpret_cast<cBody*>( mConstraint->b->data );
}

cpFloat cConstraint::MaxForce() {
	return mConstraint->maxForce;
}

void cConstraint::MaxForce( const cpFloat& maxforce ) {
	mConstraint->maxForce = maxforce;
}

cpFloat cConstraint::BiasCoef() {
	return mConstraint->biasCoef;
}

void cConstraint::BiasCoef( const cpFloat& biascoef ) {
	mConstraint->biasCoef = biascoef;
}

cpFloat cConstraint::MaxBias() {
	return mConstraint->maxBias;
}

void cConstraint::MaxBias( const cpFloat& maxbias ) {
	mConstraint->maxBias = maxbias;
}

void cConstraint::Draw() {
}

}}
