#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( PackManager )

PackManager::PackManager() : mFallback( true ) {}

PackManager::~PackManager() {}

Pack* PackManager::exists( std::string& path ) {
	std::string tpath( path );

	FileSystem::filePathRemoveProcessPath( tpath );

	for ( auto& pack : mResources ) {
		if ( -1 != pack->exists( tpath ) ) {
			if ( path.size() != tpath.size() ) {
				path = tpath;
			}

			return pack;
		}
	}

	return NULL;
}

Pack* PackManager::getPackByPath( std::string path ) {
	for ( auto& pack : mResources ) {
		if ( path == pack->getPackPath() ) {
			return pack;
		}
	}

	return NULL;
}

const bool& PackManager::isFallbackToPacksActive() const {
	return mFallback;
}

void PackManager::setFallbackToPacks( const bool& fallback ) {
	mFallback = fallback;
}

}} // namespace EE::System
