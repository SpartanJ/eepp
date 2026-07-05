#include <SOIL2/src/SOIL2/SOIL2.h>
#include <SOIL2/src/SOIL2/stb_image.h>
#include <algorithm>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureloader.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <jpeg-compressor/jpge.h>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION( TextureFactory )

TextureFactory::TextureFactory() :
	mCurrentTexture( EE_MAX_TEXTURE_UNITS ),
	mMemSize( 0 ),
	mTextureIdSeq( 0 ),
	mLastCoordinateType( Texture::CoordinateType::Normalized ),
	mErasing( false ) {}

const Texture::CoordinateType& TextureFactory::getLastCoordinateType() const {
	return mLastCoordinateType;
}

TextureFactory::~TextureFactory() {
	unloadTextures();
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

Texture* TextureFactory::pushTexture( const std::string& Filepath, const Uint32& TexId,
									  const unsigned int& Width, const unsigned int& Height,
									  const unsigned int& ImgWidth, const unsigned int& ImgHeight,
									  const bool& Mipmap, const unsigned int& Channels,
									  const Texture::ClampMode& ClampMode,
									  const bool& CompressTexture, const bool& LocalCopy,
									  const Uint32& MemSize ) {
	Lock l( *this );

	Texture* Tex = NULL;
	Uint32 Pos;

	std::string FPath( Filepath );

	FileSystem::filePathRemoveProcessPath( FPath );

	Pos = ++mTextureIdSeq;
	Tex = mTextures[Pos] = eeNew( Texture, () );

	Tex->create( TexId, Width, Height, ImgWidth, ImgHeight, Mipmap, Channels, FPath, ClampMode,
				 CompressTexture, MemSize );
	Tex->setTextureId( Pos );

	if ( LocalCopy ) {
		Tex->lock();
		Tex->unlock( true, false );
	}

	mMemSize += MemSize;

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

void TextureFactory::bind( const Uint32& TexId, Texture::CoordinateType coordinateType,
						   const Uint32& textureUnit, const bool& forceRebind ) {
	bind( getTexture( TexId ), coordinateType, textureUnit, forceRebind );
}

void TextureFactory::unloadTextures() {
	Lock l( *this );

	mErasing = true;

	for ( auto& texture : mTextures )
		eeSAFE_DELETE( texture.second );

	mErasing = false;

	mTextures.clear();
	mTextureIdSeq = 0;

	Log::debug( "Textures Unloaded." );
}

bool TextureFactory::remove( Uint32 TexId ) {
	Lock l( *this );

	Texture* Tex;
	auto it = mTextures.find( TexId );

	if ( it != mTextures.end() && NULL != ( Tex = it->second ) ) {
		removeReference( Tex );

		mErasing = true;
		eeDelete( Tex );
		mErasing = false;

		return true;
	}

	return false;
}

bool TextureFactory::remove( Texture* texture ) {
	Lock l( *this );

	auto it = std::find_if( mTextures.begin(), mTextures.end(),
							[texture]( const auto& pair ) { return pair.second == texture; } );
	if ( it != mTextures.end() ) {
		removeReference( texture );

		mErasing = true;
		eeDelete( texture );
		mErasing = false;

		return true;
	}
	return false;
}

void TextureFactory::removeReference( Texture* Tex ) {
	Lock l( *this );

	auto it = mTextures.find( Tex->getTextureId() );
	if ( it == mTextures.end() || it->second != Tex )
		return;

	mMemSize -= Tex->getMemSize();

	int glTexId = Tex->getHandle();

	mTextures.erase( it );

	for ( Uint32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		if ( mCurrentTexture[i] == (Int32)glTexId )
			mCurrentTexture[i] = 0;
	}
}

void TextureFactory::updateMemorySize( Uint32 oldSize, Uint32 newSize ) {
	Lock l( *this );

	mMemSize -= oldSize;
	mMemSize += newSize;
}

bool TextureFactory::isErasing() {
	Lock l( *this );

	return mErasing;
}

int TextureFactory::getCurrentTexture( const Uint32& TextureUnit ) const {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	return mCurrentTexture[TextureUnit];
}

void TextureFactory::setCurrentTexture( const int& TexId, const Uint32& TextureUnit ) {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	mCurrentTexture[TextureUnit] = TexId;
}

std::vector<Texture*> TextureFactory::getTextures() {
	Lock l( *this );

	std::vector<Texture*> textures;
	textures.reserve( mTextures.size() );

	for ( const auto& texture : mTextures ) {
		Texture* Tex = texture.second;

		if ( Tex )
			textures.push_back( Tex );
	}

	return textures;
}

void TextureFactory::reloadAllTextures() {
	Lock l( *this );

	for ( const auto& texture : mTextures ) {
		Texture* Tex = texture.second;

		if ( Tex )
			Tex->reload();
	}

	Log::debug( "Textures Reloaded." );
}

void TextureFactory::grabTextures() {
	Lock l( *this );

	for ( const auto& texture : mTextures ) {
		Texture* Tex = texture.second;

		if ( Tex && !Tex->hasLocalCopy() ) {
			Tex->lock();
			Tex->setGrabbed( true );
		}
	}
}

void TextureFactory::ungrabTextures() {
	Lock l( *this );

	for ( const auto& texture : mTextures ) {
		Texture* Tex = texture.second;

		if ( NULL != Tex && Tex->isGrabbed() ) {
			Tex->reload();
			Tex->unlock();
			Tex->setGrabbed( false );
		}
	}
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

bool TextureFactory::existsId( const Uint32& TexId ) {
	Lock l( *this );

	return mTextures.find( TexId ) != mTextures.end();
}

bool TextureFactory::exists( const Texture* tex ) {
	Lock l( *this );

	return std::find_if( mTextures.begin(), mTextures.end(), [tex]( const auto& pair ) {
			   return pair.second == tex;
		   } ) != mTextures.end();
}

Texture* TextureFactory::getTexture( const Uint32& TexId ) {
	Lock l( *this );

	auto it = mTextures.find( TexId );
	return it != mTextures.end() ? it->second : NULL;
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

	return mMemSize;
}

Texture* TextureFactory::getByHash( const String::HashType& hash ) {
	Lock l( *this );

	Uint32 latestId = 0;
	Texture* latestTexture = NULL;
	for ( const auto& texture : mTextures ) {
		Texture* tTex = texture.second;

		if ( NULL != tTex && texture.first > latestId && tTex->getHashName() == hash ) {
			latestId = texture.first;
			latestTexture = tTex;
		}
	}

	return latestTexture;
}

}} // namespace EE::Graphics
