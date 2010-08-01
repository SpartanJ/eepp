#include "cshapegroupmanager.hpp"
#include "cglobalshapegroup.hpp"

namespace EE { namespace Graphics {

cShapeGroupManager::cShapeGroupManager() {
	Add( cGlobalShapeGroup::instance() );
}

cShapeGroupManager::~cShapeGroupManager() {
	Destroy();
}

void cShapeGroupManager::Destroy() {
	std::list<cShapeGroup*>::iterator it;

	for ( it = mGroups.begin(); it != mGroups.end(); it++ )
		eeSAFE_DELETE( (*it) );
}

void cShapeGroupManager::Add( cShapeGroup * Group ) {
	if ( NULL != Group )
		mGroups.push_back( Group );
}

void cShapeGroupManager::Remove( cShapeGroup * Group, bool Delete ) {
	if ( NULL != Group ) {
		mGroups.remove( Group );

		if ( Delete )
			eeSAFE_DELETE( Group );
	}
}

Uint32 cShapeGroupManager::Count() {
	return mGroups.size();
}

cShapeGroup * cShapeGroupManager::GetByName( const std::string& Name ) {
	return GetById( MakeHash( Name ) );
}

cShapeGroup * cShapeGroupManager::GetById( const Uint32& Id ) {
	std::list<cShapeGroup*>::iterator it;

	cShapeGroup * tGroup = NULL;

	for ( it = mGroups.begin(); it != mGroups.end(); it++ ) {
		tGroup = (*it);

		if ( tGroup->Id() == Id )
			return tGroup;
	}

	return NULL;
}

cShape * cShapeGroupManager::GetShapeByName( const std::string& Name ) {
	return GetShapeById( MakeHash( Name ) );
}

cShape * cShapeGroupManager::GetShapeById( const Uint32& Id ) {
	std::list<cShapeGroup*>::iterator it;

	cShapeGroup * tGroup = NULL;
	cShape * tShape = NULL;

	for ( it = mGroups.begin(); it != mGroups.end(); it++ ) {
		tGroup = (*it);
		tShape = tGroup->GetById( Id );

		if ( NULL != tShape )
			return tShape;
	}

	return NULL;
}

}}
