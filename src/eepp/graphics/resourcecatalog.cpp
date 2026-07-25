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
	ResourceNameHash hash = resourceNameHash( key );
	{
		Lock lock( mMutex );
		auto it = mDrawables.find( key );
		if ( it == mDrawables.end() ) {
			mDrawables.emplace( std::move( key ), drawable );
			mDrawablesByNameHash[hash] = drawable;
			return;
		}

		previous = std::move( it->second );
		it->second = drawable;
		mDrawablesByNameHash[hash] = drawable;
	}

	previous.reset();
}

void ResourceCatalog::publishAtlas( ResourceKey key, TextureAtlasPtr atlas ) {
	publishAtlas( key.value(), std::move( atlas ) );
}

void ResourceCatalog::publishAtlas( std::string key, TextureAtlasPtr atlas ) {
	if ( key.empty() )
		return;

	if ( !atlas ) {
		eraseAtlas( key );
		return;
	}

	TextureAtlasPtr previous;
	{
		Lock lock( mMutex );
		auto it = mAtlases.find( key );
		if ( it == mAtlases.end() ) {
			mAtlases.emplace( std::move( key ), std::move( atlas ) );
			return;
		}
		previous = std::move( it->second );
		it->second = std::move( atlas );
	}
	previous.reset();
}

void ResourceCatalog::publishFont( ResourceKey key, FontPtr font ) {
	publishFont( key.value(), std::move( font ) );
}

void ResourceCatalog::publishFont( std::string key, FontPtr font ) {
	if ( key.empty() )
		return;
	if ( !font ) {
		eraseFont( key );
		return;
	}

	FontPtr previous;
	ResourceNameHash hash = resourceNameHash( key );
	{
		Lock lock( mMutex );
		auto it = mFonts.find( key );
		if ( it == mFonts.end() ) {
			mFonts.emplace( std::move( key ), font );
			mFontsByNameHash[hash] = font;
			return;
		}
		previous = std::move( it->second );
		it->second = font;
		mFontsByNameHash[hash] = font;
	}
	previous.reset();
}

void ResourceCatalog::publishShaderProgram( ResourceKey key, ShaderProgramPtr program ) {
	publishShaderProgram( key.value(), std::move( program ) );
}

void ResourceCatalog::publishShaderProgram( std::string key, ShaderProgramPtr program ) {
	if ( key.empty() )
		return;
	if ( !program ) {
		eraseShaderProgram( key );
		return;
	}
	ShaderProgramPtr previous;
	{
		Lock lock( mMutex );
		auto it = mShaderPrograms.find( key );
		if ( it == mShaderPrograms.end() ) {
			mShaderPrograms.emplace( std::move( key ), std::move( program ) );
			return;
		}
		previous = std::move( it->second );
		it->second = std::move( program );
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

DrawablePtr ResourceCatalog::findDrawable( ResourceNameHash hash ) const {
	Lock lock( mMutex );
	auto it = mDrawablesByNameHash.find( hash );
	return it != mDrawablesByNameHash.end() ? it->second.lock() : DrawablePtr{};
}

DrawablePtr ResourceCatalog::findDrawable( String::HashType legacyHash ) const {
	Lock lock( mMutex );
	for ( const auto& [key, drawable] : mDrawables ) {
		if ( String::hash( key ) == legacyHash )
			return drawable;
	}
	return {};
}

TextureAtlasPtr ResourceCatalog::findAtlas( const ResourceKey& key ) const {
	return findAtlas( key.value() );
}

TextureAtlasPtr ResourceCatalog::findAtlas( const std::string& key ) const {
	Lock lock( mMutex );
	auto it = mAtlases.find( key );
	return it != mAtlases.end() ? it->second : TextureAtlasPtr{};
}

std::vector<TextureAtlasPtr> ResourceCatalog::getAtlases() const {
	std::vector<TextureAtlasPtr> atlases;
	Lock lock( mMutex );
	atlases.reserve( mAtlases.size() );
	for ( const auto& atlas : mAtlases )
		atlases.emplace_back( atlas.second );
	return atlases;
}

FontPtr ResourceCatalog::findFont( const ResourceKey& key ) const {
	return findFont( key.value() );
}

FontPtr ResourceCatalog::findFont( const std::string& key ) const {
	Lock lock( mMutex );
	auto it = mFonts.find( key );
	return it != mFonts.end() ? it->second : FontPtr{};
}

FontPtr ResourceCatalog::findFont( ResourceNameHash hash ) const {
	Lock lock( mMutex );
	auto it = mFontsByNameHash.find( hash );
	return it != mFontsByNameHash.end() ? it->second.lock() : FontPtr{};
}

std::vector<FontPtr> ResourceCatalog::getFonts() const {
	std::vector<FontPtr> fonts;
	Lock lock( mMutex );
	fonts.reserve( mFonts.size() );
	for ( const auto& font : mFonts )
		fonts.emplace_back( font.second );
	return fonts;
}

ShaderProgramPtr ResourceCatalog::findShaderProgram( const ResourceKey& key ) const {
	return findShaderProgram( key.value() );
}

ShaderProgramPtr ResourceCatalog::findShaderProgram( const std::string& key ) const {
	Lock lock( mMutex );
	auto it = mShaderPrograms.find( key );
	return it != mShaderPrograms.end() ? it->second : ShaderProgramPtr{};
}

std::vector<ShaderProgramPtr> ResourceCatalog::getShaderPrograms() const {
	std::vector<ShaderProgramPtr> programs;
	Lock lock( mMutex );
	programs.reserve( mShaderPrograms.size() );
	for ( const auto& program : mShaderPrograms )
		programs.emplace_back( program.second );
	return programs;
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
		mDrawablesByNameHash.erase( resourceNameHash( key ) );
	}

	drawable.reset();
	return true;
}

bool ResourceCatalog::eraseAtlas( const ResourceKey& key ) {
	return eraseAtlas( key.value() );
}

bool ResourceCatalog::eraseAtlas( const std::string& key ) {
	TextureAtlasPtr atlas;
	{
		Lock lock( mMutex );
		auto it = mAtlases.find( key );
		if ( it == mAtlases.end() )
			return false;
		atlas = std::move( it->second );
		mAtlases.erase( it );
	}
	atlas.reset();
	return true;
}

bool ResourceCatalog::eraseFont( const ResourceKey& key ) {
	return eraseFont( key.value() );
}

bool ResourceCatalog::eraseFont( const std::string& key ) {
	FontPtr font;
	{
		Lock lock( mMutex );
		auto it = mFonts.find( key );
		if ( it == mFonts.end() )
			return false;
		font = std::move( it->second );
		mFonts.erase( it );
		mFontsByNameHash.erase( resourceNameHash( key ) );
	}
	font.reset();
	return true;
}

bool ResourceCatalog::eraseFont( Font* font ) {
	if ( !font )
		return false;
	FontPtr removed;
	{
		Lock lock( mMutex );
		auto it = mFonts.find( font->getName() );
		if ( it == mFonts.end() || it->second.get() != font )
			return false;
		removed = std::move( it->second );
		mFonts.erase( it );
		mFontsByNameHash.erase( resourceNameHash( font->getName() ) );
	}
	removed.reset();
	return true;
}

bool ResourceCatalog::eraseShaderProgram( const ResourceKey& key ) {
	return eraseShaderProgram( key.value() );
}

bool ResourceCatalog::eraseShaderProgram( const std::string& key ) {
	ShaderProgramPtr program;
	{
		Lock lock( mMutex );
		auto it = mShaderPrograms.find( key );
		if ( it == mShaderPrograms.end() )
			return false;
		program = std::move( it->second );
		mShaderPrograms.erase( it );
	}
	program.reset();
	return true;
}

void ResourceCatalog::clear() {
	UnorderedMap<std::string, TexturePtr> textures;
	UnorderedMap<std::string, DrawablePtr> drawables;
	UnorderedMap<ResourceNameHash, DrawableWeakPtr> drawablesByNameHash;
	UnorderedMap<std::string, TextureAtlasPtr> atlases;
	UnorderedMap<std::string, FontPtr> fonts;
	UnorderedMap<ResourceNameHash, FontWeakPtr> fontsByNameHash;
	UnorderedMap<std::string, ShaderProgramPtr> shaderPrograms;
	{
		Lock lock( mMutex );
		textures = std::move( mTextures );
		drawables = std::move( mDrawables );
		drawablesByNameHash = std::move( mDrawablesByNameHash );
		atlases = std::move( mAtlases );
		fonts = std::move( mFonts );
		fontsByNameHash = std::move( mFontsByNameHash );
		shaderPrograms = std::move( mShaderPrograms );
	}

	textures.clear();
	drawables.clear();
	drawablesByNameHash.clear();
	atlases.clear();
	fonts.clear();
	fontsByNameHash.clear();
	shaderPrograms.clear();
}

std::size_t ResourceCatalog::size() const {
	Lock lock( mMutex );
	return mTextures.size() + mDrawables.size() + mAtlases.size() + mFonts.size() +
		   mShaderPrograms.size();
}

}} // namespace EE::Graphics
