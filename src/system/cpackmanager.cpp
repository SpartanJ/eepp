#include "cpackmanager.hpp"

namespace EE { namespace System {

cPackManager::cPackManager()
{
}

cPackManager::~cPackManager() {
}

cPack * cPackManager::Exists( std::string& path ) {
	FilePathRemoveProcessPath( path );

	std::list<cPack*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		if ( -1 != (*it)->Exists( path ) ) {
			return (*it);
		}
	}

	return NULL;
}

}}
