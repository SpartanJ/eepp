#include "cshapegroupmanager.hpp"
#include "cglobalshapegroup.hpp"

namespace EE { namespace Graphics {

cShapeGroupManager::cShapeGroupManager() :
	tResourceManager<cShapeGroup>( false )
{
	Add( cGlobalShapeGroup::instance() );
}

cShapeGroupManager::~cShapeGroupManager() {
}

cShape * cShapeGroupManager::GetShapeByName( const std::string& Name ) {
	return GetShapeById( MakeHash( Name ) );
}

cShape * cShapeGroupManager::GetShapeById( const Uint32& Id ) {
	std::list<cShapeGroup*>::iterator it;

	cShape * tShape = NULL;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		tShape = (*it)->GetById( Id );

		if ( NULL != tShape )
			return tShape;
	}

	return NULL;
}

}}
