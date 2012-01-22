#include "cpackmanager.hpp"

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION(cPackManager)

cPackManager::cPackManager() :
	mFallback( true )
{
}

cPackManager::~cPackManager() {
}

cPack * cPackManager::Exists( std::string& path ) {
	std::string tpath( path );

	FilePathRemoveProcessPath( tpath );

	std::list<cPack*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		if ( -1 != (*it)->Exists( tpath ) ) {
			if ( path.size() != tpath.size() ) {
				path = tpath;
			}

			return (*it);
		}
	}

	return NULL;
}

const bool& cPackManager::FallbackToPacks() const {
	return mFallback;
}

void cPackManager::FallbackToPacks( const bool& fallback ) {
	mFallback = fallback;
}

}}
