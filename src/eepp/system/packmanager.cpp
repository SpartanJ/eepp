#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/packregistry.hpp>

namespace EE { namespace System {

SINGLETON_DECLARE_IMPLEMENTATION( PackRegistry )

PackRegistry::PackRegistry() : mFallback( true ) {}

PackRegistry::~PackRegistry() {}

Pack* PackRegistry::exists( std::string& path ) {
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

Pack* PackRegistry::getPackByPath( std::string path ) {
	for ( auto& pack : mResources ) {
		if ( path == pack->getPackPath() ) {
			return pack;
		}
	}

	return NULL;
}

const bool& PackRegistry::isFallbackToPacksActive() const {
	return mFallback;
}

void PackRegistry::setFallbackToPacks( const bool& fallback ) {
	mFallback = fallback;
}

}} // namespace EE::System
