#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureloader.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpge.h>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(TextureFactory)

TextureFactory::TextureFactory() :
	mLastBlend(ALPHA_NORMAL),
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

Uint32 TextureFactory::createEmptyTexture( const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const ColorA& DefaultColor, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	Image TmpImg( Width, Height, Channels, DefaultColor );
	return loadFromPixels( TmpImg.getPixelsPtr(), Width, Height, Channels, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
}

Uint32 TextureFactory::loadFromPixels( const unsigned char * Pixels, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const std::string& FileName ) {
	TextureLoader myTex( Pixels, Width, Height, Channels, Mipmap, ClampMode, CompressTexture, KeepLocalCopy, FileName );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromPack( Pack* Pack, const std::string& FilePackPath, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy  ) {
	TextureLoader myTex( Pack, FilePackPath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromMemory( const unsigned char * ImagePtr, const unsigned int& Size, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	TextureLoader myTex( ImagePtr, Size, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::loadFromStream( IOStream& Stream, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	TextureLoader myTex( Stream, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::load( const std::string& Filepath, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	TextureLoader myTex( Filepath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.load();
	return myTex.getId();
}

Uint32 TextureFactory::pushTexture( const std::string& Filepath, const Uint32& TexId, const unsigned int& Width, const unsigned int& Height, const unsigned int& ImgWidth, const unsigned int& ImgHeight, const bool& Mipmap, const unsigned int& Channels, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& LocalCopy, const Uint32& MemSize ) {
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
	if( NULL != Tex && mCurrentTexture[ TextureUnit ] != (Int32)Tex->handle() ) {
		if ( TextureUnit && GLi->isExtension( EEGL_ARB_multitexture ) )
			setActiveTextureUnit( TextureUnit );

		GLi->bindTexture( GL_TEXTURE_2D, Tex->handle() );

		mCurrentTexture[ TextureUnit ] = Tex->handle();

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

	int glTexId = Tex->handle();

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

		if ( Tex && !Tex->localCopy() ) {
			Tex->lock();
			Tex->grabed(true);
		}
	}
}

void TextureFactory::ungrabTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		Texture* Tex = getTexture(i);

		if ( NULL != Tex && Tex->grabed() ) {
			Tex->reload();
			Tex->unlock();
			Tex->grabed(false);
		}
	}
}

void TextureFactory::setActiveTextureUnit( const Uint32& Unit ) {
	GLi->activeTexture( GL_TEXTURE0 + Unit );
}

void TextureFactory::setTextureEnv( const EE_TEXTURE_PARAM& Param, const Int32& Val ) {
	#ifndef EE_GLES2
	unsigned int lParam = (unsigned int)GLi->getTextureParamEnum( Param );

	GLi->texEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );

	if( Param == TEX_PARAM_COLOR_FUNC || Param == TEX_PARAM_ALPHA_FUNC ) {
		GLi->texEnvi( GL_TEXTURE_ENV, lParam, GLi->getTextureFuncEnum( (EE_TEXTURE_FUNC)Val ) );
	} else if( Param >= TEX_PARAM_COLOR_SOURCE_0 && Param <= TEX_PARAM_ALPHA_SOURCE_2 ) {
		GLi->texEnvi( GL_TEXTURE_ENV, lParam, GLi->getTextureSourceEnum( (EE_TEXTURE_SOURCE)Val ) );
	} else if( Param >= TEX_PARAM_COLOR_OP_0 && Param <= TEX_PARAM_ALPHA_OP_2 ) {
		GLi->texEnvi( GL_TEXTURE_ENV, lParam, GLi->getTextureOpEnum( (EE_TEXTURE_OP)Val ) );
	} else {
		GLi->texEnvi( GL_TEXTURE_ENV, lParam, Val );
	}
	#endif
}

unsigned int TextureFactory::getValidTextureSize( const unsigned int& Size ) {
	if ( GLi->isExtension( EEGL_ARB_texture_non_power_of_two ) )
		return Size;
	else
		return Math::nextPowOfTwo(Size);
}

bool TextureFactory::saveImage( const std::string& filepath, const EE_SAVE_TYPE& Format, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels, const unsigned char* data ) {
	bool Res;

	if ( SAVE_TYPE_JPG != Format ) {
		Res = 0 != SOIL_save_image ( filepath.c_str(), Format, Width, Height, Channels, data );
	} else {
		jpge::params params;
		params.m_quality = Image::jpegQuality();
		Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), Width, Height, Channels, data, params);
	}

	return Res;
}

bool TextureFactory::textureIdExists( const Uint32& TexId ) {
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

		if ( NULL != tTex && tTex->hashName() == Hash )
			return mTextures[ i ];
	}

	return NULL;
}

}}
