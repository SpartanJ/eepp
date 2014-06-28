#include <eepp/physics/physicsmanager.hpp>
#include <eepp/physics/body.hpp>
#include <eepp/physics/shape.hpp>
#include <eepp/physics/space.hpp>
#include <eepp/physics/constraints/constraint.hpp>
#include <algorithm>

CP_NAMESPACE_BEGIN

SINGLETON_DECLARE_IMPLEMENTATION(PhysicsManager)

PhysicsManager::PhysicsManager() :
	mMemoryManager( false )
{
}

PhysicsManager::~PhysicsManager() {
	if ( mMemoryManager ) {
		mMemoryManager = false;

		std::list<Space*>::iterator its = mSpaces.begin();
		for ( ; its != mSpaces.end(); its++ )
			cpSAFE_DELETE( *its );

		std::list<Body*>::iterator itb = mBodysFree.begin();
		for ( ; itb != mBodysFree.end(); itb++ )
			cpSAFE_DELETE( *itb );

		std::list<Shape*>::iterator itp = mShapesFree.begin();
		for ( ; itp != mShapesFree.end(); itp++ )
			cpSAFE_DELETE( *itp );

		std::list<Constraint*>::iterator itc = mConstraintFree.begin();
		for ( ; itc != mConstraintFree.end(); itc++ )
			cpSAFE_DELETE( *itc );
	}
}

PhysicsManager::cDrawSpaceOptions * PhysicsManager::GetDrawOptions() {
	return &mOptions;
}

void PhysicsManager::MemoryManager( bool MemoryManager ) {
	mMemoryManager = MemoryManager;
}

const bool& PhysicsManager::MemoryManager() const {
	return mMemoryManager;
}

void PhysicsManager::AddBodyFree( Body * body ) {
	if ( mMemoryManager ) {
		if ( std::find( mBodysFree.begin(), mBodysFree.end(), body ) == mBodysFree.end() )
			mBodysFree.push_back( body );
	}
}

void PhysicsManager::RemoveBodyFree( Body * body ) {
	if ( mMemoryManager ) {
		mBodysFree.remove( body );
	}
}

void PhysicsManager::AddShapeFree( Shape * shape ) {
	if ( mMemoryManager ) {
		if ( std::find( mShapesFree.begin(), mShapesFree.end(), shape ) == mShapesFree.end() )
			mShapesFree.push_back( shape );
	}
}

void PhysicsManager::RemoveShapeFree( Shape * shape ) {
	if ( mMemoryManager ) {
		mShapesFree.remove( shape );
	}
}

void PhysicsManager::AddConstraintFree( Constraint * constraint ) {
	if ( mMemoryManager ) {
		if ( std::find( mConstraintFree.begin(), mConstraintFree.end(), constraint ) == mConstraintFree.end() )
			mConstraintFree.push_back( constraint );
	}
}

void PhysicsManager::RemoveConstraintFree( Constraint * constraint ) {
	if ( mMemoryManager ) {
		mConstraintFree.remove( constraint );
	}
}

void PhysicsManager::AddSpace( Space * space ) {
	if ( mMemoryManager ) {
		if ( std::find( mSpaces.begin(), mSpaces.end(), space ) == mSpaces.end() )
			mSpaces.push_back( space );
	}
}

void PhysicsManager::RemoveSpace( Space * space ) {
	if ( mMemoryManager ) {
		mSpaces.remove( space );
	}
}

CP_NAMESPACE_END
