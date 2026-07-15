#include <SOIL2/src/SOIL2/SOIL2.h>
#include <SOIL2/src/SOIL2/stb_image.h>
#include <algorithm>
#include <atomic>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureloader.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/window/engine.hpp>
#include <jpeg-compressor/jpge.h>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( TextureFactory )

static std::atomic<Uint64> sResourceIdSequence{ 0 };

static ResourceId nextResourceId() {
	return ResourceId( sResourceIdSequence.fetch_add( 1, std::memory_order_relaxed ) + 1 );
}

TextureFactory::TextureFactory() :
	mCurrentTexture( EE_MAX_TEXTURE_UNITS ),
	mLastCoordinateType( Texture::CoordinateType::Normalized ) {}

const Texture::CoordinateType& TextureFactory::getLastCoordinateType() const {
	return mLastCoordinateType;
}

TextureFactory::~TextureFactory() {
	unloadTextures();
	collectReleasedTextures();
	diagnoseLiveTexturesAtShutdown();
}

void TextureFactory::TextureDeleter::operator()( Texture* texture ) const noexcept {
	if ( !texture )
		return;

	if ( TextureFactory* factory = TextureFactory::existsSingleton() ) {
		factory->queueReleasedTexture( texture );
		return;
	}

	// Engine shutdown defensively releases the GPU payload of contract-violating survivors. Such
	// late handles can still release their CPU object safely without recreating any singleton.
	if ( texture->getHandle() == 0 ) {
		eeDelete( texture );
		return;
	}

	eePRINTL( "Texture released after TextureFactory destruction with a live OpenGL handle" );
	eeASSERTM( false, Texture_released_after_TextureFactory_destruction );
}

Texture* TextureFactory::createEmptyTexture( const unsigned int& Width, const unsigned int& Height,
											 const unsigned int& Channels,
											 const Color& DefaultColor, const bool& Mipmap,
											 const Texture::ClampMode& ClampMode,
											 const bool& CompressTexture, const bool& KeepLocalCopy,
											 const std::string& Filename ) {
	Image TmpImg( Width, Height, Channels, DefaultColor );
	return loadFromPixels( TmpImg.getPixelsPtr(), Width, Height, Channels, Mipmap, ClampMode,
						   CompressTexture, KeepLocalCopy, Filename );
}

Texture* TextureFactory::loadFromPixels( const unsigned char* Pixels, const unsigned int& Width,
										 const unsigned int& Height, const unsigned int& Channels,
										 const bool& Mipmap, const Texture::ClampMode& ClampMode,
										 const bool& CompressTexture, const bool& KeepLocalCopy,
										 const std::string& FileName ) {
	TextureLoader myTex( Pixels, Width, Height, Channels, Mipmap, ClampMode, CompressTexture,
						 KeepLocalCopy, FileName );
	myTex.load();
	return myTex.getTexture();
}

Texture*
TextureFactory::loadFromPack( Pack* Pack, const std::string& FilePackPath, const bool& Mipmap,
							  const Texture::ClampMode& ClampMode, const bool& CompressTexture,
							  const bool& KeepLocalCopy,
							  const Image::FormatConfiguration& imageformatConfiguration ) {
	TextureLoader myTex( Pack, FilePackPath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.setFormatConfiguration( imageformatConfiguration );
	myTex.load();
	return myTex.getTexture();
}

Texture*
TextureFactory::loadFromMemory( const unsigned char* ImagePtr, const unsigned int& Size,
								const bool& Mipmap, const Texture::ClampMode& ClampMode,
								const bool& CompressTexture, const bool& KeepLocalCopy,
								const Image::FormatConfiguration& imageformatConfiguration ) {
	TextureLoader myTex( ImagePtr, Size, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.setFormatConfiguration( imageformatConfiguration );
	myTex.load();
	return myTex.getTexture();
}

Texture*
TextureFactory::loadFromStream( IOStream& Stream, const bool& Mipmap,
								const Texture::ClampMode& ClampMode, const bool& CompressTexture,
								const bool& KeepLocalCopy,
								const Image::FormatConfiguration& imageformatConfiguration ) {
	TextureLoader myTex( Stream, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.setFormatConfiguration( imageformatConfiguration );
	myTex.load();
	return myTex.getTexture();
}

Texture*
TextureFactory::loadFromFile( const std::string& Filepath, const bool& Mipmap,
							  const Texture::ClampMode& ClampMode, const bool& CompressTexture,
							  const bool& KeepLocalCopy,
							  const Image::FormatConfiguration& imageformatConfiguration ) {
	TextureLoader myTex( Filepath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.setFormatConfiguration( imageformatConfiguration );
	myTex.load();
	return myTex.getTexture();
}

Texture* TextureFactory::pushTexture( const std::string& Filepath, const Uint32& textureHandle,
									  const unsigned int& Width, const unsigned int& Height,
									  const unsigned int& ImgWidth, const unsigned int& ImgHeight,
									  const bool& Mipmap, const unsigned int& Channels,
									  const Texture::ClampMode& ClampMode,
									  const bool& CompressTexture, const bool& LocalCopy,
									  const Uint32& MemSize ) {
	Lock l( *this );

	std::string FPath( Filepath );

	FileSystem::filePathRemoveProcessPath( FPath );

	const ResourceId resourceId = nextResourceId();
	TexturePtr texture( eeNew( Texture, () ), TextureDeleter() );
	Texture* Tex = texture.get();
	Tex->setTextureId( resourceId );

	Tex->create( textureHandle, Width, Height, ImgWidth, ImgHeight, Mipmap, Channels, FPath,
				 ClampMode, CompressTexture, MemSize );
	TextureWeakPtr weakTexture( texture );
	mTextures.emplace( resourceId.value(), std::move( texture ) );
	mLiveTextures.emplace( resourceId.value(),
						   LiveTextureRecord{ resourceId, std::move( weakTexture ) } );
	mLiveTextureGeneration.fetch_add( 1, std::memory_order_release );

	if ( LocalCopy ) {
		Tex->lock();
		Tex->unlock( true, false );
	}

	return Tex;
}

void TextureFactory::bind( const Texture* texture, Texture::CoordinateType coordinateType,
						   const Uint32& TextureUnit, const bool& forceRebind ) {
	if ( NULL != texture ) {
		if ( mCurrentTexture[TextureUnit] != (Int32)texture->getHandle() || forceRebind ) {
			if ( TextureUnit && GLi->isExtension( EEGL_ARB_multitexture ) )
				setActiveTextureUnit( TextureUnit );

			GLi->bindTexture( GL_TEXTURE_2D, texture->getHandle() );

			mCurrentTexture[TextureUnit] = texture->getHandle();

			if ( TextureUnit && GLi->isExtension( EEGL_ARB_multitexture ) )
				setActiveTextureUnit( 0 );
		}

		if ( coordinateType == Texture::CoordinateType::Pixels ) {
			GLfloat matrix[16] = { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f,
								   0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f };

			matrix[0] = 1.f / const_cast<Texture*>( texture )->getPixelsSize().x;
			matrix[5] = 1.f / const_cast<Texture*>( texture )->getPixelsSize().y;

			GLi->matrixMode( GL_TEXTURE );
			GLi->loadMatrixf( matrix );
			GLi->matrixMode( GL_MODELVIEW );

			mLastCoordinateType = coordinateType;

			return;
		}
	} else {
		mCurrentTexture[TextureUnit] = 0;
	}

	if ( Texture::CoordinateType::Normalized != mLastCoordinateType ) {
		mLastCoordinateType = coordinateType;

		GLi->matrixMode( GL_TEXTURE );
		GLi->loadIdentity();
		GLi->matrixMode( GL_MODELVIEW );
	}
}

void TextureFactory::bind( ResourceId textureId, Texture::CoordinateType coordinateType,
						   const Uint32& textureUnit, const bool& forceRebind ) {
	bind( getTexture( textureId ), coordinateType, textureUnit, forceRebind );
}

void TextureFactory::unloadTextures() {
	TextureMap textures;
	{
		Lock l( *this );
		textures = std::move( mTextures );
		std::fill( mCurrentTexture.begin(), mCurrentTexture.end(), 0 );
	}

	// DrawableResource destruction emits callbacks, so release factory ownership without holding
	// the registry/factory mutex.
	textures.clear();

	Log::debug( "Textures Unloaded." );
}

bool TextureFactory::remove( ResourceId textureId ) {
	TexturePtr texture;
	{
		Lock l( *this );
		auto it = mTextures.find( textureId.value() );
		if ( it == mTextures.end() )
			return false;

		texture = std::move( it->second );
		mTextures.erase( it );
		resetTextureBinding( texture.get() );
	}

	texture.reset();
	return true;
}

bool TextureFactory::remove( Texture* texture ) {
	TexturePtr removed;
	{
		Lock l( *this );
		auto it = std::find_if( mTextures.begin(), mTextures.end(), [texture]( const auto& pair ) {
			return pair.second.get() == texture;
		} );
		if ( it == mTextures.end() )
			return false;

		removed = std::move( it->second );
		mTextures.erase( it );
		resetTextureBinding( removed.get() );
	}

	removed.reset();
	return true;
}

void TextureFactory::resetTextureBinding( const Texture* texture ) {
	if ( !texture )
		return;

	const int glTexId = texture->getHandle();
	for ( Uint32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		if ( mCurrentTexture[i] == (Int32)glTexId )
			mCurrentTexture[i] = 0;
	}
}

int TextureFactory::getCurrentTexture( const Uint32& TextureUnit ) const {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	return mCurrentTexture[TextureUnit];
}

void TextureFactory::setCurrentTexture( const int& textureHandle, const Uint32& TextureUnit ) {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	mCurrentTexture[TextureUnit] = textureHandle;
}

std::vector<Texture*> TextureFactory::getTextures() {
	Lock l( *this );

	std::vector<Texture*> textures;
	textures.reserve( mTextures.size() );

	for ( const auto& texture : mTextures ) {
		Texture* Tex = texture.second.get();

		if ( Tex )
			textures.push_back( Tex );
	}

	return textures;
}

TextureRegistrySnapshot TextureFactory::snapshotTextures() {
	struct LockedTextureRecord {
		LiveTextureRecord record;
		TexturePtr texture;
	};

	std::vector<LockedTextureRecord> liveTextures;
	{
		Lock l( *this );
		liveTextures.reserve( mLiveTextures.size() );
		for ( auto it = mLiveTextures.begin(); it != mLiveTextures.end(); ) {
			TexturePtr texture = it->second.texture.lock();
			if ( !texture ) {
				it = mLiveTextures.erase( it );
				continue;
			}

			liveTextures.push_back( { it->second, std::move( texture ) } );
			++it;
		}
	}

	TextureRegistrySnapshot snapshot;
	snapshot.reserve( liveTextures.size() );
	for ( const auto& live : liveTextures ) {
		snapshot.push_back( { live.record.id, live.texture->getName(), live.record.texture,
							  live.texture->getMemSize() } );
	}
	return snapshot;
}

void TextureFactory::purgeExpiredTextures() {
	Lock l( *this );
	for ( auto it = mLiveTextures.begin(); it != mLiveTextures.end(); ) {
		if ( it->second.texture.expired() )
			it = mLiveTextures.erase( it );
		else
			++it;
	}
}

Uint64 TextureFactory::getLiveTextureGeneration() const {
	return mLiveTextureGeneration.load( std::memory_order_acquire );
}

void TextureFactory::queueReleasedTexture( Texture* texture ) {
	eeASSERTM( Window::Engine::existsSingleton() && Window::Engine::isMainThread(),
			   Texture_final_release_must_run_on_the_graphics_thread );

	Lock l( *this );
	mReleasedTextures.push_back( texture );
	mLiveTextureGeneration.fetch_add( 1, std::memory_order_release );
}

void TextureFactory::collectReleasedTextures() {
	eeASSERTM( Window::Engine::existsSingleton() && Window::Engine::isMainThread(),
			   Texture_collection_must_run_on_the_graphics_thread );

	std::vector<Texture*> releasedTextures;
	for ( ;; ) {
		{
			Lock l( *this );
			if ( mReleasedTextures.empty() ) {
				if ( releasedTextures.capacity() > mReleasedTextures.capacity() )
					mReleasedTextures.swap( releasedTextures );
				break;
			}
			releasedTextures.swap( mReleasedTextures );
			for ( Texture* texture : releasedTextures )
				resetTextureBinding( texture );
		}

		for ( Texture* texture : releasedTextures )
			eeDelete( texture );

		releasedTextures.clear();
	}

	purgeExpiredTextures();
}

std::size_t TextureFactory::getPendingReleaseCount() {
	Lock l( *this );
	return mReleasedTextures.size();
}

void TextureFactory::diagnoseLiveTexturesAtShutdown() {
	std::vector<TexturePtr> survivors;
	{
		Lock l( *this );
		for ( const auto& record : mLiveTextures ) {
			if ( TexturePtr texture = record.second.texture.lock() )
				survivors.emplace_back( std::move( texture ) );
		}
	}

	if ( survivors.empty() )
		return;

	Log::error( "TextureFactory shutdown found %zu externally retained texture(s).",
				survivors.size() );
	for ( const TexturePtr& texture : survivors ) {
		Log::error( "Texture %llu ('%s') survived shutdown with %zu external owner(s).",
					static_cast<unsigned long long>( texture->getTextureId().value() ),
					texture->getName().c_str(), texture.use_count() - 1 );
		texture->deleteTexture();
	}

	eeASSERTM( false, Texture_handles_must_be_released_before_Engine_shutdown );
	survivors.clear();
	collectReleasedTextures();
}

void TextureFactory::setActiveTextureUnit( const Uint32& Unit ) {
	GLi->activeTexture( GL_TEXTURE0 + Unit );
}

unsigned int TextureFactory::getValidTextureSize( const unsigned int& Size ) {
	if ( GLi->isExtension( EEGL_ARB_texture_non_power_of_two ) )
		return Size;
	else
		return Math::nextPowOfTwo( Size );
}

bool TextureFactory::existsId( ResourceId textureId ) {
	Lock l( *this );

	return mTextures.find( textureId.value() ) != mTextures.end();
}

bool TextureFactory::exists( const Texture* tex ) {
	Lock l( *this );

	return std::find_if( mTextures.begin(), mTextures.end(), [tex]( const auto& pair ) {
			   return pair.second.get() == tex;
		   } ) != mTextures.end();
}

Texture* TextureFactory::getTexture( ResourceId textureId ) {
	Lock l( *this );

	auto it = mTextures.find( textureId.value() );
	return it != mTextures.end() ? it->second.get() : NULL;
}

Texture* TextureFactory::getByName( const std::string& Name ) {
	return getByHash( String::hash( Name ) );
}

Uint32 TextureFactory::getTextureCount() {
	Lock l( *this );

	return (Uint32)mTextures.size();
}

unsigned int TextureFactory::getTextureMemorySize() {
	Lock l( *this );

	std::size_t memorySize = 0;
	for ( const auto& texture : mLiveTextures ) {
		if ( TexturePtr liveTexture = texture.second.texture.lock() )
			memorySize += liveTexture->getMemSize();
	}
	return static_cast<unsigned int>( memorySize );
}

Texture* TextureFactory::getByHash( const String::HashType& hash ) {
	Lock l( *this );

	Uint64 latestId = 0;
	Texture* latestTexture = NULL;
	for ( const auto& texture : mTextures ) {
		Texture* tTex = texture.second.get();

		if ( NULL != tTex && texture.first > latestId && tTex->getHashName() == hash ) {
			latestId = texture.first;
			latestTexture = tTex;
		}
	}

	return latestTexture;
}

}} // namespace EE::Graphics
