#include <eepp/physics/arbiter.hpp>

CP_NAMESPACE_BEGIN

Arbiter::Arbiter( cpArbiter * arbiter ) {
	mArbiter = arbiter;
}

cVect Arbiter::TotalImpulse() {
	return tovect( cpArbiterTotalImpulse( mArbiter ) );
}

cVect Arbiter::TotalImpulseWithFriction() {
	return tovect( cpArbiterTotalImpulseWithFriction( mArbiter ) );
}

void Arbiter::Ignore() {
	return cpArbiterIgnore( mArbiter );
}

void Arbiter::GetShapes( Shape ** a, Shape ** b ) {
	cpShape * tA;
	cpShape * tB;

	cpArbiterGetShapes( mArbiter, &tA, &tB );

	if ( NULL != tA )
		*a = reinterpret_cast<Shape*>( tA->data );
	else
		*a = NULL;

	if ( NULL != tB )
		*b = reinterpret_cast<Shape*>( tB->data );
	else
		*b = NULL;
}

void Arbiter::GetBodies( Body ** a, Body ** b) {
	cpBody * tA;
	cpBody * tB;

	cpArbiterGetBodies( mArbiter, &tA, &tB );

	if ( NULL != tA )
		*a = reinterpret_cast<Body*>( tA->data );
	else
		*a = NULL;

	if ( NULL != tB )
		*b = reinterpret_cast<Body*>( tB->data );
	else
		*b = NULL;
}

bool Arbiter::IsFirstContact() {
	return 0 != cpArbiterIsFirstContact( mArbiter );
}

int Arbiter::GetCount() {
	return cpArbiterGetCount( mArbiter );
}

cVect Arbiter::GetNormal( int i ) {
	return tovect( cpArbiterGetNormal( mArbiter, i ) );
}

cVect Arbiter::GetPoint( int i ) {
	return tovect( cpArbiterGetPoint( mArbiter, i ) );
}

cpFloat Arbiter::GetDepth( int i ) {
	return cpArbiterGetDepth( mArbiter, i );
}

cpContactPointSet Arbiter::ContactPointSet() {
	return cpArbiterGetContactPointSet( mArbiter );
}

void Arbiter::ContactPointSet( cpContactPointSet * contact ) {
	cpArbiterSetContactPointSet( mArbiter, contact );
}

cpArbiter *	Arbiter::GetArbiter() const {
	return mArbiter;
}

cpFloat Arbiter::Elasticity() {
	return cpArbiterGetElasticity( mArbiter);
}

void Arbiter::Elasticity( cpFloat value ) {
	cpArbiterSetElasticity( mArbiter, value );
}

cpFloat Arbiter::Friction() {
	return cpArbiterGetFriction( mArbiter );
}

void Arbiter::Friction( cpFloat value ) {
	cpArbiterSetFriction( mArbiter, value );
}

cVect Arbiter::SurfaceVelocity() {
	return tovect( cpArbiterGetSurfaceVelocity( mArbiter ) );
}

void Arbiter::SurfaceVelocity( cVect value ) {
	cpArbiterSetSurfaceVelocity( mArbiter, tocpv( value ) );
}

void Arbiter::UserData( cpDataPointer value ) {
	cpArbiterSetUserData( mArbiter, value );
}

cpDataPointer Arbiter::UserData() const {
	return cpArbiterGetUserData( mArbiter );
}

CP_NAMESPACE_END
