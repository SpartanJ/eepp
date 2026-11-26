#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/sprite.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::System;
using namespace EE::Window;

namespace EE { namespace Graphics {

ResourceScopePtr ResourceScope::New() {
	return ResourceScopePtr( eeNew( ResourceScope, () ), ResourceDeleter<ResourceScope>() );
}

ResourceScope::ResourceScope() : mLocalCatalog( ResourceCatalog::New() ), mFontService( *this ) {}

ResourceScope::~ResourceScope() {
	for ( const FontPtr& font : mLocalCatalog->getFonts() )
		detachFontService( font );
}

void ResourceScope::attachFontService( const FontPtr& font ) {
	// FontTrueType currently carries one borrowed service pointer. Catalog imports deliberately do
	// not call this function; sharing must preserve the service of the font's owning local scope.
	// Moving fallback resolution out of FontTrueType would allow true multi-scope local
	// publication.
	if ( font && font->getType() == FontType::TTF )
		static_cast<FontTrueType*>( font.get() )->setFontService( &mFontService );
}

void ResourceScope::detachFontService( const FontPtr& font ) {
	if ( font ) {
		mFontService.onFontRemoved( font.get() );
		if ( font->getType() != FontType::TTF )
			return;
		auto* ttf = static_cast<FontTrueType*>( font.get() );
		if ( ttf->getFontService() == &mFontService ) {
			ttf->setFontService( nullptr );
		}
	}
}

FontService& ResourceScope::getFontService() {
	return mFontService;
}

const FontService& ResourceScope::getFontService() const {
	return mFontService;
}

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

TexturePtr ResourceScope::findOrLoadTexture( const std::string& key,
											 const std::function<TexturePtr()>& load ) {
	if ( auto texture = findTexture( key ) )
		return texture;

	std::shared_future<TexturePtr> result;
	std::packaged_task<TexturePtr()> task;
	std::shared_ptr<DeferredTextureLoad> deferred;
	PendingTextureLoad expired;
	{
		Lock lock( mMutex );
		auto pending = mPendingTextureLoads.find( key );
		if ( pending != mPendingTextureLoads.end() && pending->second.deferred ) {
			deferred = pending->second.deferred->lock();
			if ( !deferred ) {
				expired = std::move( pending->second );
				mPendingTextureLoads.erase( pending );
				pending = mPendingTextureLoads.end();
			}
		}
		if ( pending != mPendingTextureLoads.end() ) {
			result = pending->second.result;
			if ( deferred && deferred->upload.valid() ) {
				// A thread that can upload finishes a deferred load directly. Waiting on the scene
				// would block the scene thread forever, and could block a worker behind its own
				// queued preparation or while the scene thread waits for that worker. A worker
				// without a shared GL context could not upload this texture itself either.
				if ( !canUploadTextures() )
					return {};
				task = std::move( deferred->upload );
			}
		} else {
			task = textureLoadTask( key, load );
			result = task.get_future().share();
			mPendingTextureLoads.emplace( key, PendingTextureLoad{ result, {}, {} } );
		}
	}

	if ( task.valid() )
		runTextureLoad( key, std::move( task ), result );
	// Another thread that can upload is finishing a taken deferred load. Drop our ownership
	// before waiting so a discarded load still releases its promise.
	const bool wasDeferred = static_cast<bool>( deferred );
	deferred.reset();
	try {
		return result.get();
	} catch ( const std::future_error& error ) {
		if ( wasDeferred && error.code() == std::future_errc::broken_promise )
			return {};
		throw;
	}
}

TexturePtr ResourceScope::findSharedRaster( const std::string& key ) const {
	Lock lock( mMutex );
	auto it = mSharedRasters.find( key );
	return it != mSharedRasters.end() ? it->second.lock() : TexturePtr{};
}

TexturePtr ResourceScope::publishSharedRaster( const std::string& key, TexturePtr texture ) {
	if ( !texture )
		return texture;
	Lock lock( mMutex );
	auto [it, inserted] = mSharedRasters.try_emplace( key );
	if ( !inserted ) {
		if ( auto shared = it->second.lock() )
			return shared;
	}
	it->second = texture;
	// Expired entries only hold their key: drop them as new rasters arrive.
	for ( auto entry = mSharedRasters.begin(); entry != mSharedRasters.end(); ) {
		if ( entry->second.expired() )
			entry = mSharedRasters.erase( entry );
		else
			++entry;
	}
	return texture;
}

bool ResourceScope::canUploadTextures() {
	return Engine::isMainThread() ||
		   ( Engine::isEngineRunning() && Engine::instance()->isSharedGLContextEnabled() );
}

ResourceScope::DeferredTextureLoad::DeferredTextureLoad( const ResourceScopePtr& scope,
														 const std::string& key ) :
	scope( scope ), key( key ) {}

ResourceScope::DeferredTextureLoad::~DeferredTextureLoad() {
	if ( auto owner = scope.lock() )
		owner->cancelUnownedTextureLoad( key );
}

std::optional<TexturePtr>
ResourceScope::requestTextureLoad( const ResourceScopePtr& scope, const std::string& key,
								   ThreadPool* pool, const std::function<TextureLoad()>& createLoad,
								   PostTextureUpload dispatch, TextureLoadCallback ready,
								   bool sharedRaster ) {
	if ( auto texture = scope->findTexture( key ) )
		return texture;
	if ( sharedRaster ) {
		if ( auto texture = scope->findSharedRaster( key ) )
			return texture;
	}

	std::shared_ptr<DeferredTextureLoad> deferred;
	std::function<void()> prepare;
	PendingTextureLoad expired;
	{
		Lock lock( scope->mMutex );
		auto pending = scope->mPendingTextureLoads.find( key );
		if ( pending != scope->mPendingTextureLoads.end() && pending->second.deferred ) {
			deferred = pending->second.deferred->lock();
			if ( !deferred ) {
				expired = std::move( pending->second );
				scope->mPendingTextureLoads.erase( pending );
				pending = scope->mPendingTextureLoads.end();
			}
		}
		if ( pending == scope->mPendingTextureLoads.end() ) {
			if ( auto texture = scope->mLocalCatalog->findTexture( key ) )
				return texture;
			if ( sharedRaster ) {
				// A shared raster may have been published since the unlocked lookup.
				auto shared = scope->mSharedRasters.find( key );
				if ( shared != scope->mSharedRasters.end() ) {
					if ( auto texture = shared->second.lock() )
						return texture;
				}
			}
			if ( !pool )
				return std::nullopt;
			auto load = createLoad();
			prepare = std::move( load.prepare );
			deferred = std::make_shared<DeferredTextureLoad>( scope, key );
			deferred->upload =
				scope->textureLoadTask( key, std::move( load.upload ), sharedRaster );
			auto result = deferred->upload.get_future().share();
			pending =
				scope->mPendingTextureLoads
					.emplace( key,
							  PendingTextureLoad{ std::move( result ),
												  {},
												  std::weak_ptr<DeferredTextureLoad>{ deferred } } )
					.first;
		}
		pending->second.callbacks.push_back( std::move( ready ) );
		if ( !deferred || !deferred->upload.valid() )
			return TexturePtr{};
		if ( !deferred->prepared ) {
			deferred->dispatchers.push_back( std::move( dispatch ) );
			dispatch = {};
		}
	}

	// The key lives in the load, so the queued closures don't copy it.
	const auto finish = [scope, deferred] { scope->finishTextureLoad( deferred ); };
	if ( dispatch )
		dispatch( finish );
	if ( prepare ) {
		pool->run( [scope, deferred, prepare = std::move( prepare ), finish] {
			prepare();
			decltype( deferred->dispatchers ) dispatchers;
			{
				Lock lock( scope->mMutex );
				deferred->prepared = true;
				dispatchers.swap( deferred->dispatchers );
			}
			for ( auto& post : dispatchers )
				post( finish );
		} );
	}
	return TexturePtr{};
}

void ResourceScope::finishTextureLoad( const std::shared_ptr<DeferredTextureLoad>& deferred ) {
	const std::string& key = deferred->key;
	std::packaged_task<TexturePtr()> task;
	std::shared_future<TexturePtr> result;
	std::shared_ptr<DeferredTextureLoad> pendingLoad;
	{
		Lock lock( mMutex );
		auto pending = mPendingTextureLoads.find( key );
		if ( pending == mPendingTextureLoads.end() || !pending->second.deferred )
			return;
		pendingLoad = pending->second.deferred->lock();
		if ( pendingLoad != deferred )
			return;
		task = std::move( deferred->upload );
		result = pending->second.result;
	}
	if ( task.valid() )
		runTextureLoad( key, std::move( task ), result );
}

void ResourceScope::cancelUnownedTextureLoad( const std::string& key ) {
	PendingTextureLoad expired;
	{
		Lock lock( mMutex );
		auto pending = mPendingTextureLoads.find( key );
		if ( pending == mPendingTextureLoads.end() || !pending->second.deferred ||
			 !pending->second.deferred->expired() )
			return;
		expired = std::move( pending->second );
		mPendingTextureLoads.erase( pending );
	}
}

std::packaged_task<TexturePtr()> ResourceScope::textureLoadTask( const std::string& key,
																 std::function<TexturePtr()> load,
																 bool sharedRaster ) {
	return std::packaged_task<TexturePtr()>( [this, key, load = std::move( load ), sharedRaster] {
		// A load may have finished between the initial lookup and acquiring mMutex.
		if ( auto texture = findTexture( key ) )
			return texture;
		if ( sharedRaster ) {
			if ( auto texture = findSharedRaster( key ) )
				return texture;
		}
		auto texture = load();
		if ( !texture )
			return texture;
		if ( sharedRaster )
			return publishSharedRaster( key, std::move( texture ) );
		publishLocal( key, texture );
		return texture;
	} );
}

void ResourceScope::runTextureLoad( const std::string& key, std::packaged_task<TexturePtr()> task,
									const std::shared_future<TexturePtr>& result ) {
	task();
	decltype( PendingTextureLoad::callbacks ) callbacks;
	{
		Lock lock( mMutex );
		auto pending = mPendingTextureLoads.find( key );
		callbacks.swap( pending->second.callbacks );
		mPendingTextureLoads.erase( pending );
	}
	if ( callbacks.empty() )
		return;

	TexturePtr texture;
	try {
		texture = result.get();
	} catch ( ... ) {
		// Report failure to subscribers. Synchronous callers receive the exception via get().
	}
	for ( auto& callback : callbacks )
		callback( texture );
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

TextureAtlasPtr ResourceScope::findAtlas( const ResourceKey& key ) const {
	return findAtlas( key.value() );
}

TextureAtlasPtr ResourceScope::findAtlas( const std::string& key ) const {
	if ( TextureAtlasPtr atlas = mLocalCatalog->findAtlas( key ) )
		return atlas;

	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		if ( TextureAtlasPtr atlas = catalog->findAtlas( key ) )
			return atlas;
	}
	return {};
}

std::vector<TextureAtlasPtr> ResourceScope::getAtlases() const {
	std::vector<TextureAtlasPtr> atlases = mLocalCatalog->getAtlases();
	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		std::vector<TextureAtlasPtr> imported = catalog->getAtlases();
		atlases.insert( atlases.end(), imported.begin(), imported.end() );
	}
	return atlases;
}

FontPtr ResourceScope::findFont( const ResourceKey& key ) const {
	return findFont( key.value() );
}

FontPtr ResourceScope::findFont( const std::string& key ) const {
	if ( FontPtr font = mLocalCatalog->findFont( key ) )
		return font;
	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		if ( FontPtr font = catalog->findFont( key ) )
			return font;
	}
	return {};
}

FontPtr ResourceScope::findFont( ResourceNameHash hash ) const {
	if ( FontPtr font = mLocalCatalog->findFont( hash ) )
		return font;
	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		if ( FontPtr font = catalog->findFont( hash ) )
			return font;
	}
	return {};
}

std::vector<FontPtr> ResourceScope::getFonts() const {
	std::vector<FontPtr> fonts = mLocalCatalog->getFonts();
	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		std::vector<FontPtr> imported = catalog->getFonts();
		fonts.insert( fonts.end(), imported.begin(), imported.end() );
	}
	return fonts;
}

ShaderProgramPtr ResourceScope::findShaderProgram( const ResourceKey& key ) const {
	return findShaderProgram( key.value() );
}

ShaderProgramPtr ResourceScope::findShaderProgram( const std::string& key ) const {
	if ( ShaderProgramPtr program = mLocalCatalog->findShaderProgram( key ) )
		return program;
	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		if ( ShaderProgramPtr program = catalog->findShaderProgram( key ) )
			return program;
	}
	return {};
}

std::vector<ShaderProgramPtr> ResourceScope::getShaderPrograms() const {
	std::vector<ShaderProgramPtr> programs = mLocalCatalog->getShaderPrograms();
	Lock lock( mMutex );
	for ( const ResourceCatalogPtr& catalog : mImports ) {
		auto imported = catalog->getShaderPrograms();
		programs.insert( programs.end(), imported.begin(), imported.end() );
	}
	return programs;
}

std::vector<TextureRegionPtr>
ResourceScope::findTextureRegionsByPattern( const std::string& name, const std::string& extension,
											TextureAtlas* searchInTextureAtlas ) const {
	std::vector<TextureRegionPtr> regions;
	std::string suffix = extension.empty() ? "" : "." + extension;
	int padding = 0;

	auto findRegion = [&]( const std::string& key ) -> TextureRegionPtr {
		DrawablePtr drawable = searchInTextureAtlas ? searchInTextureAtlas->getByName( key )
													: findDrawableSource( key );
		return drawable && drawable->getDrawableType() == Drawable::TEXTUREREGION
				   ? std::static_pointer_cast<TextureRegion>( drawable )
				   : TextureRegionPtr{};
	};

	for ( int len = 1; len < 7 && padding == 0; ++len ) {
		for ( int i = 0; i < 2; ++i ) {
			std::string format( "%s%0" + String::toString( len ) + "d%s" );
			if ( findRegion( String::format( format.c_str(), name.c_str(), i, suffix.c_str() ) ) ) {
				padding = len;
				break;
			}
		}
	}

	if ( padding == 0 )
		return regions;

	for ( int i = 0;; ++i ) {
		std::string format( "%s%0" + String::toString( padding ) + "d%s" );
		TextureRegionPtr region =
			findRegion( String::format( format.c_str(), name.c_str(), i, suffix.c_str() ) );
		if ( region ) {
			regions.emplace_back( std::move( region ) );
		} else if ( i != 0 ) {
			break;
		}
	}
	return regions;
}

std::vector<TextureRegionPtr>
ResourceScope::findTextureRegionsByPatternId( const String::HashType& id,
											  const std::string& extension,
											  TextureAtlas* searchInTextureAtlas ) const {
	DrawablePtr drawable;
	if ( searchInTextureAtlas ) {
		drawable = searchInTextureAtlas->getById( id );
	} else {
		drawable = mLocalCatalog->findDrawable( id );
		if ( !drawable ) {
			Lock lock( mMutex );
			for ( const ResourceCatalogPtr& catalog : mImports ) {
				if ( ( drawable = catalog->findDrawable( id ) ) )
					break;
			}
		}
	}

	if ( !drawable || drawable->getDrawableType() != Drawable::TEXTUREREGION )
		return {};
	std::string name = String::removeNumbersAtEnd( FileSystem::fileRemoveExtension(
		static_cast<TextureRegion*>( drawable.get() )->getName() ) );
	return findTextureRegionsByPattern( name, extension, searchInTextureAtlas );
}

DrawablePtr ResourceScope::findDrawable( const std::string& name, bool firstSearchSprite ) const {
	if ( name.empty() )
		return {};

	auto findSprite = [this]( const std::string& pattern ) -> DrawablePtr {
		std::vector<TextureRegionPtr> textureRegions = findTextureRegionsByPattern( pattern );
		if ( textureRegions.empty() )
			return {};
		SpritePtr sprite = Sprite::New();
		sprite->createAnimation();
		for ( const TextureRegionPtr& textureRegion : textureRegions )
			sprite->addFrame( textureRegion.get() );
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
			DrawablePtr source = findDrawableSource( name.substr( 12 ) );
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

	TexturePtr texture = findTexture( name );
	return texture ? texture->clone() : DrawablePtr{};
}

DrawablePtr ResourceScope::findDrawable( ResourceNameHash hash ) const {
	DrawablePtr source = mLocalCatalog->findDrawable( hash );
	if ( !source ) {
		Lock lock( mMutex );
		for ( const ResourceCatalogPtr& catalog : mImports ) {
			if ( ( source = catalog->findDrawable( hash ) ) )
				break;
		}
	}
	return source ? source->clone() : DrawablePtr{};
}

DrawablePtr ResourceScope::findDrawable( String::HashType legacyHash ) const {
	DrawablePtr source = mLocalCatalog->findDrawable( legacyHash );
	if ( !source ) {
		Lock lock( mMutex );
		for ( const ResourceCatalogPtr& catalog : mImports ) {
			if ( ( source = catalog->findDrawable( legacyHash ) ) )
				break;
		}
	}
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

void ResourceScope::publishLocalAtlas( ResourceKey key, TextureAtlasPtr atlas ) {
	publishLocalAtlas( key.value(), std::move( atlas ) );
}

void ResourceScope::publishLocalAtlas( std::string key, TextureAtlasPtr atlas ) {
	mLocalCatalog->publishAtlas( std::move( key ), std::move( atlas ) );
}

void ResourceScope::publishLocalFont( ResourceKey key, FontPtr font ) {
	publishLocalFont( key.value(), std::move( font ) );
}

void ResourceScope::publishLocalFont( std::string key, FontPtr font ) {
	FontPtr replaced = mLocalCatalog->findFont( key );
	if ( replaced && replaced != font )
		detachFontService( replaced );
	attachFontService( font );
	mLocalCatalog->publishFont( std::move( key ), std::move( font ) );
}

void ResourceScope::publishLocalShaderProgram( ResourceKey key, ShaderProgramPtr program ) {
	publishLocalShaderProgram( key.value(), std::move( program ) );
}

void ResourceScope::publishLocalShaderProgram( std::string key, ShaderProgramPtr program ) {
	mLocalCatalog->publishShaderProgram( std::move( key ), std::move( program ) );
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

bool ResourceScope::eraseLocalAtlas( const ResourceKey& key ) {
	return mLocalCatalog->eraseAtlas( key );
}

bool ResourceScope::eraseLocalAtlas( const std::string& key ) {
	return mLocalCatalog->eraseAtlas( key );
}

bool ResourceScope::eraseLocalFont( const ResourceKey& key ) {
	return eraseLocalFont( key.value() );
}

bool ResourceScope::eraseLocalFont( const std::string& key ) {
	FontPtr font = mLocalCatalog->findFont( key );
	if ( !font )
		return false;
	detachFontService( font );
	return mLocalCatalog->eraseFont( key );
}

bool ResourceScope::eraseLocalFont( Font* font ) {
	if ( !font )
		return false;
	FontPtr handle = mLocalCatalog->findFont( font->getName() );
	if ( handle.get() != font )
		return false;
	if ( !mLocalCatalog->eraseFont( font ) )
		return false;
	detachFontService( handle );
	return true;
}

bool ResourceScope::eraseLocalShaderProgram( const ResourceKey& key ) {
	return mLocalCatalog->eraseShaderProgram( key );
}

bool ResourceScope::eraseLocalShaderProgram( const std::string& key ) {
	return mLocalCatalog->eraseShaderProgram( key );
}

void ResourceScope::clearLocal() {
	for ( const FontPtr& font : mLocalCatalog->getFonts() )
		detachFontService( font );
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
	decltype( mImports ) imports;
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
