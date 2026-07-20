#include <eepp/graphics/resourcecatalog.hpp>
#include <eepp/system/lock.hpp>

using namespace EE::System;

namespace EE { namespace Graphics {

ResourceCatalogPtr ResourceCatalog::New() {
	return ResourceCatalogPtr( eeNew( ResourceCatalog, () ), ResourceDeleter<ResourceCatalog>() );
}

void ResourceCatalog::publish( ResourceKey key, TexturePtr texture ) {
	publish( key.value(), std::move( texture ) );
}

void ResourceCatalog::publish( std::string key, TexturePtr texture ) {
	if ( key.empty() )
		return;

	if ( !texture ) {
		erase( key );
		return;
	}

	TexturePtr previous;
	{
		Lock lock( mMutex );
		auto it = mTextures.find( key );
		if ( it == mTextures.end() ) {
			mTextures.emplace( std::move( key ), std::move( texture ) );
			return;
		}

		previous = std::move( it->second );
		it->second = std::move( texture );
	}

	// A replaced handle may be the final owner. Release it without holding the catalog mutex.
	previous.reset();
}

TexturePtr ResourceCatalog::findTexture( const ResourceKey& key ) const {
	return findTexture( key.value() );
}

TexturePtr ResourceCatalog::findTexture( const std::string& key ) const {
	Lock lock( mMutex );
	auto it = mTextures.find( key );
	return it != mTextures.end() ? it->second : TexturePtr{};
}

bool ResourceCatalog::erase( const ResourceKey& key ) {
	return erase( key.value() );
}

bool ResourceCatalog::erase( const std::string& key ) {
	TexturePtr texture;
	{
		Lock lock( mMutex );
		auto it = mTextures.find( key );
		if ( it == mTextures.end() )
			return false;

		texture = std::move( it->second );
		mTextures.erase( it );
	}

	texture.reset();
	return true;
}

void ResourceCatalog::clear() {
	UnorderedMap<std::string, TexturePtr> textures;
	{
		Lock lock( mMutex );
		textures = std::move( mTextures );
	}

	textures.clear();
}

std::size_t ResourceCatalog::size() const {
	Lock lock( mMutex );
	return mTextures.size();
}

}} // namespace EE::Graphics
