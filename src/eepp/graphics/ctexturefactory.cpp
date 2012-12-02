#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/ctextureloader.hpp>
#include <eepp/graphics/glhelper.hpp>
#include <eepp/graphics/ctexture.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>

namespace EE { namespace Graphics {

SINGLETON_DECLARE_IMPLEMENTATION(cTextureFactory)

cTextureFactory::cTextureFactory() :
	mLastBlend(ALPHA_NORMAL),
	mMemSize(0),
	mErasing(false)
{
	mTextures.clear();
	mTextures.push_back( NULL );

	memset( &mCurrentTexture[0], 0, EE_MAX_TEXTURE_UNITS );
}

cTextureFactory::~cTextureFactory() {
	UnloadTextures();
}

Uint32 cTextureFactory::CreateEmptyTexture( const eeUint& Width, const eeUint& Height, const eeUint& Channels, const eeColorA& DefaultColor, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	cImage TmpImg( Width, Height, Channels, DefaultColor );
	return LoadFromPixels( TmpImg.GetPixelsPtr(), Width, Height, Channels, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
}

Uint32 cTextureFactory::LoadFromPixels( const unsigned char * Pixels, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const std::string& FileName ) {
	cTextureLoader myTex( Pixels, Width, Height, Channels, Mipmap, ClampMode, CompressTexture, KeepLocalCopy, FileName );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::LoadFromPack( cPack* Pack, const std::string& FilePackPath, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy  ) {
	cTextureLoader myTex( Pack, FilePackPath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::LoadFromMemory( const unsigned char * ImagePtr, const eeUint& Size, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	cTextureLoader myTex( ImagePtr, Size, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::LoadFromStream( cIOStream& Stream, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	cTextureLoader myTex( Stream, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::Load( const std::string& Filepath, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	cTextureLoader myTex( Filepath, Mipmap, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::PushTexture( const std::string& Filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& LocalCopy, const Uint32& MemSize ) {
	Lock();

	cTexture * Tex 		= NULL;
	Uint32 Pos;
	eeInt MyWidth 		= ImgWidth;
	eeInt MyHeight 		= ImgHeight;

	std::string FPath( Filepath );

	FileSystem::FilePathRemoveProcessPath( FPath );

	Pos = FindFreeSlot();
	Tex = mTextures[ Pos ] = eeNew( cTexture, () );

	Tex->Create( TexId, Width, Height, MyWidth, MyHeight, Mipmap, Channels, FPath, ClampMode, CompressTexture, MemSize );
	Tex->Id( Pos );

	if ( LocalCopy ) {
		Tex->Lock();
		Tex->Unlock( true, false );
	}

	mMemSize += GetTexMemSize( Pos );

	Unlock();

	cLog::instance()->Write( "Texture " + Filepath + " loaded." );

	return Pos;
}

Uint32 cTextureFactory::FindFreeSlot() {
	if ( mVectorFreeSlots.size() ) {
		Uint32 Pos = mVectorFreeSlots.front();

		mVectorFreeSlots.pop_front();

		return Pos;
	}

	mTextures.push_back( NULL );

	return (Uint32)mTextures.size() - 1;
}

void cTextureFactory::Bind( const cTexture* Tex, const Uint32& TextureUnit ) {
	if( NULL != Tex && mCurrentTexture[ TextureUnit ] != (Int32)Tex->Handle() ) {
		if ( TextureUnit && GLi->IsExtension( EEGL_ARB_multitexture ) )
			SetActiveTextureUnit( TextureUnit );

		GLi->BindTexture( GL_TEXTURE_2D, Tex->Handle() );

		mCurrentTexture[ TextureUnit ] = Tex->Handle();

		if ( TextureUnit && GLi->IsExtension( EEGL_ARB_multitexture ) )
			SetActiveTextureUnit( 0 );
	}
}

void cTextureFactory::Bind( const Uint32& TexId, const Uint32& TextureUnit ) {
	Bind( GetTexture( TexId ), TextureUnit );
}

void cTextureFactory::UnloadTextures() {
	mErasing = true;

	for ( Uint32 i = 1; i < mTextures.size(); i++ )
		eeSAFE_DELETE( mTextures[i] );

	mErasing = false;

	mTextures.clear();

	cLog::instance()->Write( "Textures Unloaded." );
}

bool cTextureFactory::Remove( Uint32 TexId ) {
	cTexture * Tex;

	if ( TexId < mTextures.size() && NULL != ( Tex = mTextures[ TexId ] ) ) {
		RemoveReference( mTextures[ TexId ] );

		mErasing = true;
		eeDelete( Tex );
		mErasing = false;

		return true;
	}

	return false;
}

void cTextureFactory::RemoveReference( cTexture * Tex ) {
	mMemSize -= GetTexMemSize( Tex->Id() );

	GLint glTexId = Tex->Handle();

	mTextures[ Tex->Id() ] = NULL;

	for ( Uint32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
		if ( mCurrentTexture[ i ] == (Int32)glTexId )
			mCurrentTexture[ i ] = 0;
	}

	mVectorFreeSlots.push_back( Tex->Id() );
}

const bool& cTextureFactory::IsErasing() const {
	return mErasing;
}

GLint cTextureFactory::GetCurrentTexture( const Uint32& TextureUnit ) const {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	return mCurrentTexture[ TextureUnit ];
}

void cTextureFactory::SetCurrentTexture( const GLint& TexId, const Uint32& TextureUnit ) {
	eeASSERT( TextureUnit < EE_MAX_TEXTURE_UNITS );
	mCurrentTexture[ TextureUnit ] = TexId;
}

void cTextureFactory::ReloadAllTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		cTexture* Tex = GetTexture(i);

		if ( Tex )
			Tex->Reload();
	}

	cLog::instance()->Write("Textures Reloaded.");
}

void cTextureFactory::GrabTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		cTexture* Tex = GetTexture(i);

		if ( Tex && !Tex->LocalCopy() ) {
            Tex->Lock();
            Tex->Grabed(true);
		}
	}
}

void cTextureFactory::UngrabTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		cTexture* Tex = GetTexture(i);

		if ( NULL != Tex && Tex->Grabed() ) {
            Tex->Reload();
            Tex->Unlock();
            Tex->Grabed(false);
		}
	}
}

void cTextureFactory::SetActiveTextureUnit( const Uint32& Unit ) {
	GLi->ActiveTexture( GL_TEXTURE0 + Unit );
}

void cTextureFactory::SetTextureEnv( const EE_TEXTURE_PARAM& Param, const Int32& Val ) {
	#ifndef EE_GLES2
	GLenum lParam = (GLenum)GLi->GetTextureParamEnum( Param );

	GLi->TexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );

	if( Param == TEX_PARAM_COLOR_FUNC || Param == TEX_PARAM_ALPHA_FUNC ) {
		GLi->TexEnvi( GL_TEXTURE_ENV, lParam, GLi->GetTextureFuncEnum( (EE_TEXTURE_FUNC)Val ) );
	} else if( Param >= TEX_PARAM_COLOR_SOURCE_0 && Param <= TEX_PARAM_ALPHA_SOURCE_2 ) {
		GLi->TexEnvi( GL_TEXTURE_ENV, lParam, GLi->GetTextureSourceEnum( (EE_TEXTURE_SOURCE)Val ) );
	} else if( Param >= TEX_PARAM_COLOR_OP_0 && Param <= TEX_PARAM_ALPHA_OP_2 ) {
		GLi->TexEnvi( GL_TEXTURE_ENV, lParam, GLi->GetTextureOpEnum( (EE_TEXTURE_OP)Val ) );
	} else {
		GLi->TexEnvi( GL_TEXTURE_ENV, lParam, Val );
	}
	#endif
}

eeUint cTextureFactory::GetValidTextureSize( const eeUint& Size ) {
	if ( GLi->IsExtension( EEGL_ARB_texture_non_power_of_two ) )
		return Size;
	else
		return Math::NextPowOfTwo(Size);
}

bool cTextureFactory::SaveImage( const std::string& filepath, const EE_SAVE_TYPE& Format, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const unsigned char* data ) {
	return 0 != SOIL_save_image ( filepath.c_str(), Format, Width, Height, Channels, data );
}

bool cTextureFactory::TextureIdExists( const Uint32& TexId ) {
	return ( TexId < mTextures.size() && TexId > 0 && NULL != mTextures[ TexId ] );
}

cTexture * cTextureFactory::GetTexture( const Uint32& TexId ) {
	return mTextures[TexId];
}

void cTextureFactory::Allocate( const eeUint& size ) {
	if ( size > mTextures.size() ) {
		mTextures.resize( size + 1, NULL );

		for ( eeUint i = 1; i < mTextures.size(); i++ )
			mVectorFreeSlots.push_back( i );
	}
}

eeUint cTextureFactory::GetTexMemSize( const eeUint& TexId ) {
	eeUint Size = 0;

	if ( TexId < mTextures.size() && TexId > 0 ) {
		cTexture* Tex = mTextures[ TexId ];

		if ( Tex != NULL ) {
			eeUint w = Tex->Width();
			eeUint h = Tex->Height();
			eeUint c = Tex->Channels();

			if ( 0 != Tex->Size() )
				Size = Tex->Size();
			else
				Size = ( w * h * c );

			if( Tex->Mipmap() ) {
				while( w > 2 && h > 2 ) {
					w>>=1;
					h>>=1;
					Size += ( w * h * c );
				}
			}
		}

		return Size;
	}

	return 0;
}

cTexture * cTextureFactory::GetByName( const std::string& Name ) {
	return GetByHash( String::Hash( Name ) );
}

cTexture * cTextureFactory::GetByHash( const Uint32& Hash ) {
	cTexture * tTex = NULL;

	for ( Uint32 i = (Uint32)mTextures.size() - 1; i > 0; i-- ) {
		tTex = mTextures[ i ];

		if ( NULL != tTex && tTex->HashName() == Hash )
			return mTextures[ i ];
	}

	return NULL;
}

}}
