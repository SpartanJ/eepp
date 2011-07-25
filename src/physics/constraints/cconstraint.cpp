#include "cconstraint.hpp"
#include "../cphysicsmanager.hpp"

CP_NAMESPACE_BEGIN

void cConstraint::Free( cConstraint * constraint ) {
	cpSAFE_DELETE( constraint );
}

cConstraint::cConstraint( cpConstraint * Constraint ) :
	mData( NULL )
{
	mConstraint = Constraint;
	SetData();
}

cConstraint::cConstraint() :
	mConstraint( NULL ),
	mData( NULL )
{
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

cpFloat cConstraint::ErrorBias() {
	return cpConstraintGetErrorBias( mConstraint );
}

void cConstraint::ErrorBias( cpFloat value ) {
	cpConstraintSetErrorBias( mConstraint, value );
}

void cConstraint::Data( void * data ) {
	mData = data;
}

void * cConstraint::Data() const {
	return mData;
}

cpFloat cConstraint::Impulse() {
	return cpConstraintGetImpulse( mConstraint );
}

void cConstraint::Draw() {
}

CP_NAMESPACE_END
