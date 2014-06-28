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
	SetData();
}

Constraint::Constraint() :
	mConstraint( NULL ),
	mData( NULL )
{
}

Constraint::~Constraint() {
	cpConstraintFree( mConstraint );

	PhysicsManager::instance()->RemoveConstraintFree( this );
}

void Constraint::SetData() {
	mConstraint->data = (void*)this;
	PhysicsManager::instance()->AddConstraintFree( this );
}

cpConstraint * Constraint::GetConstraint() const {
	return mConstraint;
}

Body * Constraint::A() {
	return reinterpret_cast<Body*>( mConstraint->a->data );
}

Body * Constraint::B() {
	return reinterpret_cast<Body*>( mConstraint->b->data );
}

cpFloat Constraint::MaxForce() {
	return mConstraint->maxForce;
}

void Constraint::MaxForce( const cpFloat& maxforce ) {
	mConstraint->maxForce = maxforce;
}

cpFloat Constraint::MaxBias() {
	return mConstraint->maxBias;
}

void Constraint::MaxBias( const cpFloat& maxbias ) {
	mConstraint->maxBias = maxbias;
}

cpFloat Constraint::ErrorBias() {
	return cpConstraintGetErrorBias( mConstraint );
}

void Constraint::ErrorBias( cpFloat value ) {
	cpConstraintSetErrorBias( mConstraint, value );
}

void Constraint::Data( void * data ) {
	mData = data;
}

void * Constraint::Data() const {
	return mData;
}

cpFloat Constraint::Impulse() {
	return cpConstraintGetImpulse( mConstraint );
}

void Constraint::Draw() {
}

CP_NAMESPACE_END
