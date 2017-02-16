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

Body * Constraint::a() {
	return reinterpret_cast<Body*>( mConstraint->a->data );
}

Body * Constraint::b() {
	return reinterpret_cast<Body*>( mConstraint->b->data );
}

cpFloat Constraint::maxForce() {
	return mConstraint->maxForce;
}

void Constraint::maxForce( const cpFloat& maxforce ) {
	mConstraint->maxForce = maxforce;
}

cpFloat Constraint::maxBias() {
	return mConstraint->maxBias;
}

void Constraint::maxBias( const cpFloat& maxbias ) {
	mConstraint->maxBias = maxbias;
}

cpFloat Constraint::errorBias() {
	return cpConstraintGetErrorBias( mConstraint );
}

void Constraint::errorBias( cpFloat value ) {
	cpConstraintSetErrorBias( mConstraint, value );
}

void Constraint::data( void * data ) {
	mData = data;
}

void * Constraint::data() const {
	return mData;
}

cpFloat Constraint::impulse() {
	return cpConstraintGetImpulse( mConstraint );
}

void Constraint::draw() {
}

CP_NAMESPACE_END
