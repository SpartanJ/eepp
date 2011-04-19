#include "cphysicsmanager.hpp"
#include "cbody.hpp"
#include "cshape.hpp"
#include "cspace.hpp"
#include "constraints/cconstraint.hpp"

CP_NAMESPACE_BEGIN

cPhysicsManager::cPhysicsManager() :
	mMemoryManager( false )
{
	cpInitChipmunk();
}

cPhysicsManager::~cPhysicsManager() {
	if ( mMemoryManager ) {
		mMemoryManager = false;

		std::list<cSpace*>::iterator its = mSpaces.begin();
		for ( ; its != mSpaces.end(); its++ )
			cpSAFE_DELETE( *its );

		std::list<cBody*>::iterator itb = mBodysFree.begin();
		for ( ; itb != mBodysFree.end(); itb++ )
			cpSAFE_DELETE( *itb );

		std::list<cShape*>::iterator itp = mShapesFree.begin();
		for ( ; itp != mShapesFree.end(); itp++ )
			cpSAFE_DELETE( *itp );

		std::list<cConstraint*>::iterator itc = mConstraintFree.begin();
		for ( ; itc != mConstraintFree.end(); itc++ )
			cpSAFE_DELETE( *itc );
	}
}

cPhysicsManager::cDrawSpaceOptions * cPhysicsManager::GetDrawOptions() {
	return &mOptions;
}

void cPhysicsManager::MemoryManager( bool MemoryManager ) {
	mMemoryManager = MemoryManager;
}

const bool& cPhysicsManager::MemoryManager() const {
	return mMemoryManager;
}

void cPhysicsManager::AddBodyFree( cBody * body ) {
	if ( mMemoryManager ) {
		if ( std::find( mBodysFree.begin(), mBodysFree.end(), body ) == mBodysFree.end() )
			mBodysFree.push_back( body );
	}
}

void cPhysicsManager::RemoveBodyFree( cBody * body ) {
	if ( mMemoryManager ) {
		mBodysFree.remove( body );
	}
}

void cPhysicsManager::AddShapeFree( cShape * shape ) {
	if ( mMemoryManager ) {
		if ( std::find( mShapesFree.begin(), mShapesFree.end(), shape ) == mShapesFree.end() )
			mShapesFree.push_back( shape );
	}
}

void cPhysicsManager::RemoveShapeFree( cShape * shape ) {
	if ( mMemoryManager ) {
		mShapesFree.remove( shape );
	}
}

void cPhysicsManager::AddConstraintFree( cConstraint * constraint ) {
	if ( mMemoryManager ) {
		if ( std::find( mConstraintFree.begin(), mConstraintFree.end(), constraint ) == mConstraintFree.end() )
			mConstraintFree.push_back( constraint );
	}
}

void cPhysicsManager::RemoveConstraintFree( cConstraint * constraint ) {
	if ( mMemoryManager ) {
		mConstraintFree.remove( constraint );
	}
}

void cPhysicsManager::AddSpace( cSpace * space ) {
	if ( mMemoryManager ) {
		if ( std::find( mSpaces.begin(), mSpaces.end(), space ) == mSpaces.end() )
			mSpaces.push_back( space );
	}
}

void cPhysicsManager::RemoveSpace( cSpace * space ) {
	if ( mMemoryManager ) {
		mSpaces.remove( space );
	}
}

CP_NAMESPACE_END
