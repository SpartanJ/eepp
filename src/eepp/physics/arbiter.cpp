#include <eepp/physics/arbiter.hpp>

namespace EE { namespace Physics {

Arbiter::Arbiter( cpArbiter* arbiter ) {
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

void Arbiter::getShapes( Shape** a, Shape** b ) {
	cpShape* tA;
	cpShape* tB;

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

void Arbiter::getBodies( Body** a, Body** b ) {
	cpBody* tA;
	cpBody* tB;

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

cpContactPointSet Arbiter::getContactPointSet() {
	return cpArbiterGetContactPointSet( mArbiter );
}

void Arbiter::setContactPointSet( cpContactPointSet* contact ) {
	cpArbiterSetContactPointSet( mArbiter, contact );
}

cpArbiter* Arbiter::getArbiter() const {
	return mArbiter;
}

cpFloat Arbiter::getElasticity() {
	return cpArbiterGetElasticity( mArbiter );
}

void Arbiter::setElasticity( cpFloat value ) {
	cpArbiterSetElasticity( mArbiter, value );
}

cpFloat Arbiter::getFriction() {
	return cpArbiterGetFriction( mArbiter );
}

void Arbiter::setFriction( cpFloat value ) {
	cpArbiterSetFriction( mArbiter, value );
}

cVect Arbiter::getSurfaceVelocity() {
	return tovect( cpArbiterGetSurfaceVelocity( mArbiter ) );
}

void Arbiter::setSurfaceVelocity( cVect value ) {
	cpArbiterSetSurfaceVelocity( mArbiter, tocpv( value ) );
}

void Arbiter::setUserData( cpDataPointer value ) {
	cpArbiterSetUserData( mArbiter, value );
}

cpDataPointer Arbiter::getUserData() const {
	return cpArbiterGetUserData( mArbiter );
}

}} // namespace EE::Physics
