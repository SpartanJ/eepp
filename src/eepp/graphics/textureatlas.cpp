#include <eepp/core/debug.hpp>
#include <eepp/graphics/textureatlas.hpp>
#include <eepp/system/lock.hpp>

using namespace EE::System;

namespace EE { namespace Graphics {

TextureAtlasPtr TextureAtlas::New( const std::string& name ) {
	return makeResource<TextureAtlas>( name );
}

TextureAtlas::TextureAtlas( const std::string& name ) {
	setName( name );
}

TextureAtlas::~TextureAtlas() {}

const std::string& TextureAtlas::getName() const {
	return mName;
}

void TextureAtlas::setName( const std::string& name ) {
	mName = name;
	mId = String::hash( mName );
}

const std::string& TextureAtlas::getPath() const {
	return mPath;
}

void TextureAtlas::setPath( const std::string& path ) {
	mPath = path;
}

const String::HashType& TextureAtlas::getId() const {
	return mId;
}

TextureRegionPtr TextureAtlas::add( TextureRegionPtr textureRegion ) {
	if ( !textureRegion )
		return {};

	std::string realName( textureRegion->getName() );
	Uint32 count = 1;
	for ( ;; ) {
		{
			Lock lock( mMutex );
			if ( mResources.find( textureRegion->getId() ) == mResources.end() ) {
				mResources[textureRegion->getId()] = textureRegion;
				return textureRegion;
			}
		}

		// setName() can notify listeners. Never invoke callbacks while holding the atlas mutex.
		textureRegion->setName( realName + String::toString( ++count ) );
	}
}

TextureRegionPtr TextureAtlas::add( ResourceId textureId, const std::string& Name ) {
	return add( TextureRegion::New( textureId, Name ) );
}

TextureRegionPtr TextureAtlas::add( ResourceId textureId, const Rect& SrcRect,
									const std::string& Name ) {
	return add( TextureRegion::New( textureId, SrcRect, Name ) );
}

TextureRegionPtr TextureAtlas::add( ResourceId textureId, const Rect& SrcRect,
									const Sizef& DestSize, const std::string& Name ) {
	return add( TextureRegion::New( textureId, SrcRect, DestSize, Name ) );
}

TextureRegionPtr TextureAtlas::add( ResourceId textureId, const Rect& SrcRect,
									const Sizef& DestSize, const Vector2i& Offset,
									const std::string& Name ) {
	return add( TextureRegion::New( textureId, SrcRect, DestSize, Offset, Name ) );
}

TextureRegionPtr TextureAtlas::add( TexturePtr tex, const std::string& Name ) {
	return add( TextureRegion::New( std::move( tex ), Name ) );
}

TextureRegionPtr TextureAtlas::add( TexturePtr tex, const Rect& SrcRect, const std::string& Name ) {
	return add( TextureRegion::New( std::move( tex ), SrcRect, Name ) );
}

TextureRegionPtr TextureAtlas::add( TexturePtr tex, const Rect& SrcRect, const Sizef& DestSize,
									const std::string& Name ) {
	return add( TextureRegion::New( std::move( tex ), SrcRect, DestSize, Name ) );
}

TextureRegionPtr TextureAtlas::add( TexturePtr tex, const Rect& SrcRect, const Sizef& DestSize,
									const Vector2i& Offset, const std::string& Name ) {
	return add( TextureRegion::New( std::move( tex ), SrcRect, DestSize, Offset, Name ) );
}

TextureRegionPtr TextureAtlas::getByName( const std::string& name ) const {
	return getById( String::hash( name ) );
}

TextureRegionPtr TextureAtlas::getById( const String::HashType& id ) const {
	Lock lock( mMutex );
	auto it = mResources.find( id );
	return it != mResources.end() ? it->second : TextureRegionPtr{};
}

bool TextureAtlas::remove( const TextureRegionPtr& textureRegion ) {
	return textureRegion && removeById( textureRegion->getId() );
}

bool TextureAtlas::removeByName( const std::string& name ) {
	return removeById( String::hash( name ) );
}

bool TextureAtlas::removeById( const String::HashType& id ) {
	TextureRegionPtr textureRegion;
	{
		Lock lock( mMutex );
		auto it = mResources.find( id );
		if ( it == mResources.end() )
			return false;
		textureRegion = std::move( it->second );
		mResources.erase( it );
	}
	textureRegion.reset();
	return true;
}

bool TextureAtlas::exists( const std::string& name ) const {
	return existsId( String::hash( name ) );
}

bool TextureAtlas::existsId( const String::HashType& id ) const {
	Lock lock( mMutex );
	return mResources.find( id ) != mResources.end();
}

void TextureAtlas::clear() {
	UnorderedMap<String::HashType, TextureRegionPtr> resources;
	{
		Lock lock( mMutex );
		resources = std::move( mResources );
	}
	resources.clear();
}

void TextureAtlas::printNames() const {
	Lock lock( mMutex );
	for ( const auto& resource : mResources )
		eePRINTL( "'%s'", resource.second->getName().c_str() );
}

const UnorderedMap<String::HashType, TextureRegionPtr>& TextureAtlas::getResources() const {
	return mResources;
}

Uint32 TextureAtlas::getCount() const {
	Lock lock( mMutex );
	return static_cast<Uint32>( mResources.size() );
}

void TextureAtlas::setTextures( std::vector<TexturePtr> textures ) {
	mTextures = std::move( textures );
}

const TexturePtr& TextureAtlas::getTexture( const Uint32& texnum ) const {
	eeASSERT( texnum < mTextures.size() );
	return mTextures[texnum];
}

Uint32 TextureAtlas::getTexturesCount() const {
	return static_cast<Uint32>( mTextures.size() );
}

}} // namespace EE::Graphics
