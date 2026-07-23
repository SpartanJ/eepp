#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
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

DrawablePtr ResourceScope::findDrawableSource( const ResourceKey& key ) const {
	return findDrawableSource( key.value() );
}

DrawablePtr ResourceScope::findDrawableSource( const std::string& key ) const {
	if ( DrawablePtr drawable = mLocalCatalog->findDrawable( key ) )
		return drawable;

	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		if ( DrawablePtr drawable = catalog->findDrawable( key ) )
			return drawable;
	}
	return {};
}

DrawablePtr ResourceScope::findDrawable( const std::string& name, bool firstSearchSprite ) const {
	if ( name.empty() )
		return {};

	auto findSprite = []( const std::string& pattern ) -> DrawablePtr {
		std::vector<TextureRegion*> textureRegions =
			TextureAtlasManager::instance()->getTextureRegionsByPattern( pattern );
		if ( textureRegions.empty() )
			return {};
		SpritePtr sprite = Sprite::New();
		sprite->createAnimation();
		sprite->addFrames( textureRegions );
		return sprite;
	};

	bool searchedSprite = false;
	if ( firstSearchSprite ) {
		DrawablePtr sprite =
			findSprite( String::startsWith( name, "@sprite/" ) ? name.substr( 8 ) : name );
		if ( sprite )
			return sprite;
		searchedSprite = true;
	}

	if ( name[0] == '@' ) {
		if ( String::startsWith( name, "@textureregion/" ) ) {
			Drawable* source =
				TextureAtlasManager::instance()->getTextureRegionByName( name.substr( 12 ) );
			return source ? source->clone() : DrawablePtr{};
		}
		if ( String::startsWith( name, "@image/" ) ) {
			TexturePtr texture = findTexture( name.substr( 7 ) );
			return texture ? texture->clone() : DrawablePtr{};
		}
		if ( String::startsWith( name, "@texture/" ) ) {
			TexturePtr texture = findTexture( name.substr( 9 ) );
			return texture ? texture->clone() : DrawablePtr{};
		}
		if ( String::startsWith( name, "@sprite/" ) && !searchedSprite )
			return findSprite( name.substr( 8 ) );
		if ( String::startsWith( name, "@drawable/" ) )
			return findDrawable( name.substr( 10 ) );
		if ( String::startsWith( name, "@9p/" ) ) {
			DrawablePtr source = findDrawableSource( name.substr( 4 ) );
			return source && source->getDrawableType() == Drawable::NINEPATCH ? source->clone()
																			  : DrawablePtr{};
		}
	}

	if ( DrawablePtr source = findDrawableSource( name ) )
		return source->clone();

	String::HashType id = String::hash( name );
	Drawable* source = TextureAtlasManager::instance()->getTextureRegionById( id );
	if ( source )
		return source->clone();

	TexturePtr texture = findTexture( name );
	return texture ? texture->clone() : DrawablePtr{};
}

DrawablePtr ResourceScope::findDrawable( const Uint32& id ) const {
	Drawable* source = TextureAtlasManager::instance()->getTextureRegionById( id );
	return source ? source->clone() : DrawablePtr{};
}

void ResourceScope::publishLocal( ResourceKey key, TexturePtr texture ) {
	publishLocal( key.value(), std::move( texture ) );
}

void ResourceScope::publishLocal( std::string key, TexturePtr texture ) {
	mLocalCatalog->publish( std::move( key ), std::move( texture ) );
}

void ResourceScope::publishLocalDrawable( ResourceKey key, DrawablePtr drawable ) {
	publishLocalDrawable( key.value(), std::move( drawable ) );
}

void ResourceScope::publishLocalDrawable( std::string key, DrawablePtr drawable ) {
	mLocalCatalog->publishDrawable( std::move( key ), std::move( drawable ) );
}

bool ResourceScope::eraseLocal( const ResourceKey& key ) {
	return mLocalCatalog->erase( key );
}

bool ResourceScope::eraseLocal( const std::string& key ) {
	return mLocalCatalog->erase( key );
}

bool ResourceScope::eraseLocalDrawable( const ResourceKey& key ) {
	return mLocalCatalog->eraseDrawable( key );
}

bool ResourceScope::eraseLocalDrawable( const std::string& key ) {
	return mLocalCatalog->eraseDrawable( key );
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
