#include <eepp/physics/carbiter.hpp>

CP_NAMESPACE_BEGIN

cArbiter::cArbiter( cpArbiter * arbiter ) {
	mArbiter = arbiter;
}

cVect cArbiter::TotalImpulse() {
	return tovect( cpArbiterTotalImpulse( mArbiter ) );
}

cVect cArbiter::TotalImpulseWithFriction() {
	return tovect( cpArbiterTotalImpulseWithFriction( mArbiter ) );
}

void cArbiter::Ignore() {
	return cpArbiterIgnore( mArbiter );
}

void cArbiter::GetShapes( cShape ** a, cShape ** b ) {
	cpShape * tA;
	cpShape * tB;

	cpArbiterGetShapes( mArbiter, &tA, &tB );

	if ( NULL != tA )
		*a = reinterpret_cast<cShape*>( tA->data );
	else
		*a = NULL;

	if ( NULL != tB )
		*b = reinterpret_cast<cShape*>( tB->data );
	else
		*b = NULL;
}

void cArbiter::GetBodies( cBody ** a, cBody ** b) {
	cpBody * tA;
	cpBody * tB;

	cpArbiterGetBodies( mArbiter, &tA, &tB );

	if ( NULL != tA )
		*a = reinterpret_cast<cBody*>( tA->data );
	else
		*a = NULL;

	if ( NULL != tB )
		*b = reinterpret_cast<cBody*>( tB->data );
	else
		*b = NULL;
}

bool cArbiter::IsFirstContact() {
	return 0 != cpArbiterIsFirstContact( mArbiter );
}

int cArbiter::GetCount() {
	return cpArbiterGetCount( mArbiter );
}

cVect cArbiter::GetNormal( int i ) {
	return tovect( cpArbiterGetNormal( mArbiter, i ) );
}

cVect cArbiter::GetPoint( int i ) {
	return tovect( cpArbiterGetPoint( mArbiter, i ) );
}

cpFloat cArbiter::GetDepth( int i ) {
	return cpArbiterGetDepth( mArbiter, i );
}

cpContactPointSet cArbiter::ContactPointSet() {
	return cpArbiterGetContactPointSet( mArbiter );
}

void cArbiter::ContactPointSet( cpContactPointSet * contact ) {
	cpArbiterSetContactPointSet( mArbiter, contact );
}

cpArbiter *	cArbiter::Arbiter() const {
	return mArbiter;
}

cpFloat cArbiter::Elasticity() {
	return cpArbiterGetElasticity( mArbiter);
}

void cArbiter::Elasticity( cpFloat value ) {
	cpArbiterSetElasticity( mArbiter, value );
}

cpFloat cArbiter::Friction() {
	return cpArbiterGetFriction( mArbiter );
}

void cArbiter::Friction( cpFloat value ) {
	cpArbiterSetFriction( mArbiter, value );
}

cVect cArbiter::SurfaceVelocity() {
	return tovect( cpArbiterGetSurfaceVelocity( mArbiter ) );
}

void cArbiter::SurfaceVelocity( cVect value ) {
	cpArbiterSetSurfaceVelocity( mArbiter, tocpv( value ) );
}

void cArbiter::UserData( cpDataPointer value ) {
	cpArbiterSetUserData( mArbiter, value );
}

cpDataPointer cArbiter::UserData() const {
	return cpArbiterGetUserData( mArbiter );
}

CP_NAMESPACE_END
