#include "carbiter.hpp"

namespace EE { namespace Physics {

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

cpContactPointSet cArbiter::GetContactPointSet() {
	return cpArbiterGetContactPointSet( mArbiter );
}

cpArbiter *	cArbiter::Arbiter() const {
	return mArbiter;
}

}}
