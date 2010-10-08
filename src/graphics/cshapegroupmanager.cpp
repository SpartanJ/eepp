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

void cShapeGroupManager::PrintResources() {
	std::list<cShapeGroup*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ )
		(*it)->PrintNames();
}

std::vector<cShape*> cShapeGroupManager::GetShapesByPattern( const std::string& name, const std::string& extension, cShapeGroup * SearchInShapeGroup ) {
	std::vector<cShape*> 	Shapes;
	std::string 			search;
	bool 					found 	= true;
	cShape *				tShape 	= NULL;
	std::string				realext = "";
	eeInt 					c 		= 0;
	if ( extension.size() )
		realext = "." + extension;

	do {
		if ( c < 100 )
			search = StrFormated( "%s%02d%s", name.c_str(), c, realext.c_str() );
		else if ( c < 1000 )
			search = StrFormated( "%s%03d%s", name.c_str(), c, realext.c_str() );
		else if ( c < 10000 )
			search = StrFormated( "%s%04d%s", name.c_str(), c, realext.c_str() );
		else
			found = false;

		if ( found ) {
			if ( NULL == SearchInShapeGroup )
				tShape = GetShapeByName( search );
			else
				tShape = SearchInShapeGroup->GetByName( search );

			if ( NULL != tShape ) {
				Shapes.push_back( tShape );

				found = true;
			} else {
				if ( 0 == c ) // if didn't found "00", will search at least for "01"
					found = true;
				else
					found = false;
			}
		}

		c++;
	} while ( found );

	return Shapes;
}

}}
