#include <eepp/physics/constraints/constraint.hpp>
#include <eepp/physics/physicsmanager.hpp>

CP_NAMESPACE_BEGIN

void Constraint::Free( Constraint * constraint ) {
	cpSAFE_DELETE( constraint );
}

Constraint::Constraint( cpConstraint * Constraint ) :
	mData( NULL )
{
	mConstraint = Constraint;
	setData();
}

Constraint::Constraint() :
	mConstraint( NULL ),
	mData( NULL )
{
}

Constraint::~Constraint() {
	cpConstraintFree( mConstraint );

	PhysicsManager::instance()->removeConstraintFree( this );
}

void Constraint::setData() {
	mConstraint->data = (void*)this;
	PhysicsManager::instance()->addConstraintFree( this );
}

cpConstraint * Constraint::getConstraint() const {
	return mConstraint;
}

Body * Constraint::getA() {
	return reinterpret_cast<Body*>( mConstraint->a->data );
}

Body * Constraint::getB() {
	return reinterpret_cast<Body*>( mConstraint->b->data );
}

cpFloat Constraint::getMaxForce() {
	return mConstraint->maxForce;
}

void Constraint::setMaxForce( const cpFloat& maxforce ) {
	mConstraint->maxForce = maxforce;
}

cpFloat Constraint::getMaxBias() {
	return mConstraint->maxBias;
}

void Constraint::setMaxBias( const cpFloat& maxbias ) {
	mConstraint->maxBias = maxbias;
}

cpFloat Constraint::getErrorBias() {
	return cpConstraintGetErrorBias( mConstraint );
}

void Constraint::setErrorBias( cpFloat value ) {
	cpConstraintSetErrorBias( mConstraint, value );
}

void Constraint::setData( void * data ) {
	mData = data;
}

void * Constraint::getData() const {
	return mData;
}

cpFloat Constraint::getImpulse() {
	return cpConstraintGetImpulse( mConstraint );
}

void Constraint::draw() {
}

CP_NAMESPACE_END
