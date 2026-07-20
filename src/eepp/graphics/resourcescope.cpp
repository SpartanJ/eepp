#include <algorithm>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;
using namespace EE::Window;

namespace EE { namespace Graphics {

ResourceScopePtr ResourceScope::New() {
	return ResourceScopePtr( eeNew( ResourceScope, () ), ResourceDeleter<ResourceScope>() );
}

ResourceScope::ResourceScope() : mLocalCatalog( ResourceCatalog::New() ) {}

TexturePtr ResourceScope::findTexture( const ResourceKey& key ) const {
	return findTexture( key.value() );
}

TexturePtr ResourceScope::findTexture( const std::string& key ) const {
	if ( TexturePtr texture = mLocalCatalog->findTexture( key ) )
		return texture;

	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		if ( TexturePtr texture = catalog->findTexture( key ) )
			return texture;
	}
	return {};
}

void ResourceScope::publishLocal( ResourceKey key, TexturePtr texture ) {
	publishLocal( key.value(), std::move( texture ) );
}

void ResourceScope::publishLocal( std::string key, TexturePtr texture ) {
	mLocalCatalog->publish( std::move( key ), std::move( texture ) );
}

bool ResourceScope::eraseLocal( const ResourceKey& key ) {
	return mLocalCatalog->erase( key );
}

bool ResourceScope::eraseLocal( const std::string& key ) {
	return mLocalCatalog->erase( key );
}

void ResourceScope::clearLocal() {
	mLocalCatalog->clear();
}

void ResourceScope::importCatalog( ResourceCatalogPtr catalog ) {
	if ( !catalog || catalog == mLocalCatalog )
		return;

	Lock lock( mMutex );
	if ( std::find( mImports.begin(), mImports.end(), catalog ) == mImports.end() )
		mImports.emplace_back( std::move( catalog ) );
}

bool ResourceScope::removeCatalog( const ResourceCatalogPtr& catalog ) {
	ResourceCatalogPtr removed;
	{
		Lock lock( mMutex );
		auto it = std::find( mImports.begin(), mImports.end(), catalog );
		if ( it == mImports.end() )
			return false;

		removed = std::move( *it );
		mImports.erase( it );
	}

	removed.reset();
	return true;
}

void ResourceScope::clearImports() {
	std::vector<ResourceCatalogPtr> imports;
	{
		Lock lock( mMutex );
		imports = std::move( mImports );
	}

	imports.clear();
}

ResourceCatalogPtr ResourceScope::getLocalCatalog() const {
	return mLocalCatalog;
}

ResourceCatalog& globalResourceCatalog() {
	return *Engine::instance()->getGlobalResourceCatalog();
}

ResourceScope& defaultResourceScope() {
	return *Engine::instance()->getDefaultResourceScope();
}

}} // namespace EE::Graphics
