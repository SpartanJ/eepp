#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION(PackManager)

PackManager::PackManager() :
	mFallback( true )
{
}

PackManager::~PackManager() {
}

Pack * PackManager::exists( std::string& path ) {
	std::string tpath( path );

	FileSystem::filePathRemoveProcessPath( tpath );

	std::list<Pack*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		if ( -1 != (*it)->exists( tpath ) ) {
			if ( path.size() != tpath.size() ) {
				path = tpath;
			}

			return (*it);
		}
	}

	return NULL;
}

Pack * PackManager::getPackByPath( std::string path ) {
	std::list<Pack*>::iterator it;

	for ( it = mResources.begin(); it != mResources.end(); it++ ) {
		if ( path == (*it)->getPackPath() ) {
			return (*it);
		}
	}

	return NULL;
}

const bool& PackManager::fallbackToPacks() const {
	return mFallback;
}

void PackManager::fallbackToPacks( const bool& fallback ) {
	mFallback = fallback;
}

}}
