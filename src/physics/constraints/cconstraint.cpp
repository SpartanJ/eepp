#include "cconstraint.hpp"
#include "../cphysicsmanager.hpp"

CP_NAMESPACE_BEGIN

void cConstraint::Free( cConstraint * constraint ) {
	cpSAFE_DELETE( constraint );
}

cConstraint::cConstraint( cpConstraint * Constraint ) {
	mConstraint = Constraint;
	SetData();
}

cConstraint::cConstraint() {
}

cConstraint::~cConstraint() {
	cpConstraintFree( mConstraint );

	cPhysicsManager::instance()->RemoveConstraintFree( this );
}

void cConstraint::SetData() {
	mConstraint->data = (void*)this;
	cPhysicsManager::instance()->AddConstraintFree( this );
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

cpFloat cConstraint::MaxBias() {
	return mConstraint->maxBias;
}

void cConstraint::MaxBias( const cpFloat& maxbias ) {
	mConstraint->maxBias = maxbias;
}

void cConstraint::Draw() {
}

CP_NAMESPACE_END
