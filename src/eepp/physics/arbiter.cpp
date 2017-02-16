#include <eepp/physics/arbiter.hpp>

CP_NAMESPACE_BEGIN

Arbiter::Arbiter( cpArbiter * arbiter ) {
	mArbiter = arbiter;
}

cVect Arbiter::totalImpulse() {
	return tovect( cpArbiterTotalImpulse( mArbiter ) );
}

cVect Arbiter::totalImpulseWithFriction() {
	return tovect( cpArbiterTotalImpulseWithFriction( mArbiter ) );
}

void Arbiter::ignore() {
	return cpArbiterIgnore( mArbiter );
}

void Arbiter::getShapes( Shape ** a, Shape ** b ) {
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

void Arbiter::getBodies( Body ** a, Body ** b) {
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

bool Arbiter::isFirstContact() {
	return 0 != cpArbiterIsFirstContact( mArbiter );
}

int Arbiter::getCount() {
	return cpArbiterGetCount( mArbiter );
}

cVect Arbiter::getNormal( int i ) {
	return tovect( cpArbiterGetNormal( mArbiter, i ) );
}

cVect Arbiter::getPoint( int i ) {
	return tovect( cpArbiterGetPoint( mArbiter, i ) );
}

cpFloat Arbiter::getDepth( int i ) {
	return cpArbiterGetDepth( mArbiter, i );
}

cpContactPointSet Arbiter::contactPointSet() {
	return cpArbiterGetContactPointSet( mArbiter );
}

void Arbiter::contactPointSet( cpContactPointSet * contact ) {
	cpArbiterSetContactPointSet( mArbiter, contact );
}

cpArbiter *	Arbiter::getArbiter() const {
	return mArbiter;
}

cpFloat Arbiter::elasticity() {
	return cpArbiterGetElasticity( mArbiter);
}

void Arbiter::elasticity( cpFloat value ) {
	cpArbiterSetElasticity( mArbiter, value );
}

cpFloat Arbiter::friction() {
	return cpArbiterGetFriction( mArbiter );
}

void Arbiter::friction( cpFloat value ) {
	cpArbiterSetFriction( mArbiter, value );
}

cVect Arbiter::surfaceVelocity() {
	return tovect( cpArbiterGetSurfaceVelocity( mArbiter ) );
}

void Arbiter::surfaceVelocity( cVect value ) {
	cpArbiterSetSurfaceVelocity( mArbiter, tocpv( value ) );
}

void Arbiter::userData( cpDataPointer value ) {
	cpArbiterSetUserData( mArbiter, value );
}

cpDataPointer Arbiter::userData() const {
	return cpArbiterGetUserData( mArbiter );
}

CP_NAMESPACE_END
