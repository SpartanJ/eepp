#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureloader.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/system/filesystem.hpp>
#include <SOIL2/src/SOIL2/stb_image.h>
#include <SOIL2/src/SOIL2/SOIL2.h>
#include <jpeg-compressor/jpge.h>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(TextureFactory)

TextureFactory::TextureFactory() :
	mLastBlend(BlendAlpha),
	mMemSize(0),
	mErasing(false)
{
	mTextures.clear();
	mTextures.push_back( NULL );

	memset( &mCurrentTexture[0], 0, EE_MAX_TEXTURE_UNITS );
}

TextureFactory::~TextureFactory() {
	unloadTextures();
}

Uint32 TextureFactory::createEmptyTexture( const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const Color& DefaultColor, const bool& Mipmap, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	Image TmpImg( Width, Height, Channels, DefaultColor );
	return loadFromPixels( TmpImg.getPixelsPtr(), Width, Height, Channels, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
}

Uint32 TextureFactory::loadFromPixels( const unsigned char * Pixels, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const bool& Mipmap, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const std::string& FileName ) {
	TextureLoader myTex( Pixels, Width, Height, Channels, Mipmap, ClampMode, CompressTexture, KeepLocalCopy, FileName );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromPack( Pack* Pack, const std::string& FilePackPath, const bool& Mipmap, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy  ) {
	TextureLoader myTex( Pack, FilePackPath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromMemory( const unsigned char * ImagePtr, const unsigned int& Size, const bool& Mipmap, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	TextureLoader myTex( ImagePtr, Size, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromStream( IOStream& Stream, const bool& Mipmap, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	TextureLoader myTex( Stream, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromFile( const std::string& Filepath, const bool& Mipmap, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	TextureLoader myTex( Filepath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::pushTexture( const std::string& Filepath, const Uint32& TexId, const unsigned int& Width, const unsigned int& Height, const unsigned int& ImgWidth, const unsigned int& ImgHeight, const bool& Mipmap, const unsigned int& Channels, const Texture::ClampMode& ClampMode, const bool& CompressTexture, const bool& LocalCopy, const Uint32& MemSize ) {
	lock();

	Texture * Tex 		= NULL;
	Uint32 Pos;

	std::string FPath( Filepath );

	FileSystem::filePathRemoveProcessPath( FPath );

	Pos = findFreeSlot();
	Tex = mTextures[ Pos ] = eeNew( Texture, () );

	Tex->create( TexId, Width, Height, ImgWidth, ImgHeight, Mipmap, Channels, FPath, ClampMode, CompressTexture, MemSize );
	Tex->setId( Pos );

	if ( LocalCopy ) {
		Tex->lock();
		Tex->unlock( true, false );
	}

	mMemSize += MemSize;

	unlock();

	return Pos;
}

Uint32 TextureFactory::findFreeSlot() {
	if ( mVectorFreeSlots.size() ) {
		Uint32 Pos = mVectorFreeSlots.front();

		mVectorFreeSlots.pop_front();

		return Pos;
	}

	mTextures.push_back( NULL );

	return (Uint32)mTextures.size() - 1;
}

void TextureFactory::bind( const Texture* Tex, const Uint32& TextureUnit ) {
	if( NULL != Tex && mCurrentTexture[ TextureUnit ] != (Int32)Tex->getHandle() ) {
		if ( TextureUnit && GLi->isExtension( EEGL_ARB_multitexture ) )
			setActiveTextureUnit( TextureUnit );

		GLi->bindTexture( GL_TEXTURE_2D, Tex->getHandle() );

		mCurrentTexture[ TextureUnit ] = Tex->getHandle();

		if ( TextureUnit && GLi->isExtension( EEGL_ARB_multitexture ) )
			setActiveTextureUnit( 0 );
	}
}

void TextureFactory::bind( const Uint32& TexId, const Uint32& TextureUnit ) {
	bind( getTexture( TexId ), TextureUnit );
}

void TextureFactory::unloadTextures() {
	mErasing = true;

	for ( Uint32 i = 1; i < mTextures.size(); i++ )
		eeSAFE_DELETE( mTextures[i] );

	mErasing = false;

	mTextures.clear();

	eePRINTL( "Textures Unloaded." );
}

bool TextureFactory::remove( Uint32 TexId ) {
	Texture * Tex;

	if ( TexId < mTextures.size() && NULL != ( Tex = mTextures[ TexId ] ) ) {
		removeReference( mTextures[ TexId ] );

		mErasing = true;
		eeDelete( Tex );
		mErasing = false;

		return true;
	}

	return false;
}

void TextureFactory::removeReference( Texture * Tex ) {
	mMemSize -= Tex->getMemSize();

	int glTexId = Tex->getHandle();

	mTextures[ Tex->getId() ] = NULL;

	for ( Uint32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		if ( mCurrentTexture[ i ] == (Int32)glTexId )
			mCurrentTexture[ i ] = 0;
	}

	mVectorFreeSlots.push_back( Tex->getId() );
}

const bool& TextureFactory::isErasing() const {
	return mErasing;
}

int TextureFactory::getCurrentTexture( const Uint32& TextureUnit ) const {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	return mCurrentTexture[ TextureUnit ];
}

void TextureFactory::setCurrentTexture( const int& TexId, const Uint32& TextureUnit ) {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	mCurrentTexture[ TextureUnit ] = TexId;
}

std::vector<Texture*> TextureFactory::getTextures() {
	std::vector<Texture*> textures;

	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		Texture* Tex = getTexture(i);

		if ( Tex )
			textures.push_back( Tex );
	}

	return textures;
}

void TextureFactory::reloadAllTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		Texture* Tex = getTexture(i);

		if ( Tex )
			Tex->reload();
	}

	eePRINTL("Textures Reloaded.");
}

void TextureFactory::grabTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		Texture* Tex = getTexture(i);

		if ( Tex && !Tex->hasLocalCopy() ) {
			Tex->lock();
			Tex->setGrabed(true);
		}
	}
}

void TextureFactory::ungrabTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		Texture* Tex = getTexture(i);

		if ( NULL != Tex && Tex->isGrabed() ) {
			Tex->reload();
			Tex->unlock();
			Tex->setGrabed(false);
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
		return Math::nextPowOfTwo(Size);
}

bool TextureFactory::saveImage(const std::string& filepath, const Image::SaveType & Format, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const unsigned char* data ) {
	bool Res;

	if ( Image::SaveType::SAVE_TYPE_JPG != Format ) {
		Res = 0 != SOIL_save_image ( filepath.c_str(), Format, Width, Height, Channels, data );
	} else {
		jpge::params params;
		params.m_quality = Image::jpegQuality();
		Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), Width, Height, Channels, data, params);
	}

	return Res;
}

bool TextureFactory::existsId( const Uint32& TexId ) {
	return ( TexId < mTextures.size() && TexId > 0 && NULL != mTextures[ TexId ] );
}

Texture * TextureFactory::getTexture( const Uint32& TexId ) {
	return mTextures[TexId];
}

void TextureFactory::allocate( const unsigned int& size ) {
	if ( size > mTextures.size() ) {
		mTextures.resize( size + 1, NULL );

		for ( unsigned int i = 1; i < mTextures.size(); i++ )
			mVectorFreeSlots.push_back( i );
	}
}

Texture * TextureFactory::getByName( const std::string& Name ) {
	return getByHash( String::hash( Name ) );
}

Texture * TextureFactory::getByHash( const Uint32& Hash ) {
	Texture * tTex = NULL;

	for ( Uint32 i = (Uint32)mTextures.size() - 1; i > 0; i-- ) {
		tTex = mTextures[ i ];

		if ( NULL != tTex && tTex->getHashName() == Hash )
			return mTextures[ i ];
	}

	return NULL;
}

}}
