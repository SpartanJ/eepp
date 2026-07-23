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

void ResourceCatalog::publishDrawable( ResourceKey key, DrawablePtr drawable ) {
	publishDrawable( key.value(), std::move( drawable ) );
}

void ResourceCatalog::publishDrawable( std::string key, DrawablePtr drawable ) {
	if ( key.empty() )
		return;

	if ( !drawable ) {
		eraseDrawable( key );
		return;
	}

	DrawablePtr previous;
	{
		Lock lock( mMutex );
		auto it = mDrawables.find( key );
		if ( it == mDrawables.end() ) {
			mDrawables.emplace( std::move( key ), std::move( drawable ) );
			return;
		}

		previous = std::move( it->second );
		it->second = std::move( drawable );
	}

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

DrawablePtr ResourceCatalog::findDrawable( const ResourceKey& key ) const {
	return findDrawable( key.value() );
}

DrawablePtr ResourceCatalog::findDrawable( const std::string& key ) const {
	Lock lock( mMutex );
	auto it = mDrawables.find( key );
	return it != mDrawables.end() ? it->second : DrawablePtr{};
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

bool ResourceCatalog::eraseDrawable( const ResourceKey& key ) {
	return eraseDrawable( key.value() );
}

bool ResourceCatalog::eraseDrawable( const std::string& key ) {
	DrawablePtr drawable;
	{
		Lock lock( mMutex );
		auto it = mDrawables.find( key );
		if ( it == mDrawables.end() )
			return false;

		drawable = std::move( it->second );
		mDrawables.erase( it );
	}

	drawable.reset();
	return true;
}

void ResourceCatalog::clear() {
	UnorderedMap<std::string, TexturePtr> textures;
	UnorderedMap<std::string, DrawablePtr> drawables;
	{
		Lock lock( mMutex );
		textures = std::move( mTextures );
		drawables = std::move( mDrawables );
	}

	textures.clear();
	drawables.clear();
}

std::size_t ResourceCatalog::size() const {
	Lock lock( mMutex );
	return mTextures.size() + mDrawables.size();
}

}} // namespace EE::Graphics
