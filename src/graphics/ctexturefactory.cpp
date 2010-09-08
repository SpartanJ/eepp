#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"
#include "ctextureloader.hpp"
#include "glhelper.hpp"

using namespace EE::Graphics::Private;

using namespace EE::Window;

namespace EE { namespace Graphics {

cTextureFactory::cTextureFactory() :
	mMemSize(0)
{
	mTextures.clear();
	mTextures.push_back( NULL );

	mAppPath = AppPath();

	memset( &mCurrentTexture[0], 0, EE_MAX_TEXTURE_UNITS );
}

cTextureFactory::~cTextureFactory() {
	UnloadTextures();
}

Uint32 cTextureFactory::CreateEmptyTexture( const eeUint& Width, const eeUint& Height, const eeColorA& DefaultColor, const bool& Mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	std::vector<eeColorA> tmpTex( Width * Height, DefaultColor );
	return LoadFromPixels( reinterpret_cast<unsigned char*> ( &tmpTex[0] ), Width, Height, 4, Mipmap, eeRGB(true), ClampMode, CompressTexture, KeepLocalCopy );
}

Uint32 cTextureFactory::LoadFromPixels( const unsigned char * Pixels, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& Mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const std::string& FileName ) {
	cTextureLoader myTex( Pixels, Width, Height, Channels, Mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy, FileName );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::LoadFromPack( cPack* Pack, const std::string& FilePackPath, const bool& Mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy  ) {
	cTextureLoader myTex( Pack, FilePackPath, Mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::LoadFromMemory( const unsigned char * ImagePtr, const eeUint& Size, const bool& Mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	cTextureLoader myTex( ImagePtr, Size, Mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::Load( const std::string& Filepath, const bool& Mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	cTextureLoader myTex( Filepath, Mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy );
	myTex.Load();
	return myTex.Id();
}

Uint32 cTextureFactory::PushTexture( const std::string& Filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& LocalCopy, const Uint32& MemSize ) {
	Lock();

	cTexture * Tex 		= NULL;
	Uint32 Pos;
	eeInt MyWidth 		= ImgWidth;
	eeInt MyHeight 		= ImgHeight;

	std::string FPath = Filepath;

	Int32 pos = StrStartsWith( mAppPath, FPath );

	if ( -1 != pos && (Uint32)(pos + 1) < FPath.size() )
		FPath = FPath.substr( pos + 1 );

	Pos = FindFreeSlot();
	Tex = mTextures[ Pos ] = eeNew( cTexture, () );

	Tex->Create( TexId, Width, Height, MyWidth, MyHeight, Mipmap, Channels, FPath, ColorKey, ClampMode, CompressTexture, MemSize );
	Tex->Id( Pos );

	if ( !ColorKey.voidRGB )
		Tex->CreateMaskFromColor( eeColor( ColorKey.R(), ColorKey.G(), ColorKey.B() ) , 0 );

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

		mVectorFreeSlots.pop();

		return Pos;
	}

	mTextures.push_back( NULL );

	return (Uint32)mTextures.size() - 1;
}

void cTextureFactory::Bind( const cTexture* Tex, const Uint32& TextureUnit ) {
	if( NULL != Tex && mCurrentTexture[ TextureUnit ] != (Int32)Tex->Handle() ) {
		if ( cGL::instance()->IsExtension( EEGL_ARB_multitexture ) )
			glActiveTextureARB( GL_TEXTURE0_ARB + TextureUnit );

		glBindTexture( GL_TEXTURE_2D, Tex->Handle() );

		mCurrentTexture[ TextureUnit ] = Tex->Handle();

		if ( cGL::instance()->IsExtension( EEGL_ARB_multitexture ) )
			glActiveTextureARB( GL_TEXTURE0_ARB );
	}
}

void cTextureFactory::Bind( const Uint32& TexId, const Uint32& TextureUnit ) {
	Bind( GetTexture( TexId ), TextureUnit );
}

void cTextureFactory::UnloadTextures() {
	try {
		for ( Uint32 i = 1; i < mTextures.size(); i++ )
			eeSAFE_DELETE( mTextures[i] );

		mTextures.clear();

		cLog::instance()->Write( "Textures Unloaded." );
	} catch (...) {
		cLog::instance()->Write("An error ocurred on: UnloadTextures.");
	}
}

bool cTextureFactory::Remove( Uint32 TexId ) {
	if ( TexId < mTextures.size() && NULL != mTextures[ TexId ] ) {
		cTexture * Tex = mTextures[ TexId ];

		mMemSize -= GetTexMemSize( TexId );

		GLint glTexId = Tex->Handle();

		eeDelete( Tex );

		mTextures[ TexId ] = NULL;

		for ( Uint32 i = 0; i < EE_MAX_TEXTURE_UNITS; i++ ) {
			if ( mCurrentTexture[ i ] == (Int32)glTexId )
				mCurrentTexture[ i ] = 0;
		}

		mVectorFreeSlots.push( TexId );

		return true;
	}

	return false;
}

GLint cTextureFactory::GetCurrentTexture( const Uint32& TextureUnit ) const {
	//assert( TextureUnit < MAX_TEXTURE_UNITS );
	return mCurrentTexture[ TextureUnit ];
}

void cTextureFactory::SetCurrentTexture( const GLint& TexId, const Uint32& TextureUnit ) {
	//assert( TextureUnit < MAX_TEXTURE_UNITS );
	mCurrentTexture[ TextureUnit ] = TexId;
}

void cTextureFactory::ReloadAllTextures() {
	try {
		for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
			cTexture* Tex = GetTexture(i);

			if ( Tex )
				Tex->Reload();
		}
		cLog::instance()->Write("Textures Reloaded.");
	} catch (...) {
		cLog::instance()->Write("An error ocurred on: ReloadAllTextures.");
	}
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

void cTextureFactory::SetBlendFunc( const EE_BLEND_FUNC& SrcFactor, const EE_BLEND_FUNC& DestFactor ) {
	glEnable( GL_BLEND );

	glBlendFunc( (GLenum)SrcFactor, (GLenum)DestFactor );

	mLastBlend = ALPHA_CUSTOM;
}

void cTextureFactory::SetPreBlendFunc( const EE_PRE_BLEND_FUNC& blend, bool force ) {
	if ( mLastBlend != blend || force ) {
		if (blend == ALPHA_NONE) {
			glDisable( GL_BLEND );
		} else {
			glEnable( GL_BLEND );

			switch (blend) {
				case ALPHA_NORMAL:
					glBlendFunc(GL_SRC_ALPHA , GL_ONE_MINUS_SRC_ALPHA);
					break;
				case ALPHA_BLENDONE:
					glBlendFunc(GL_SRC_ALPHA , GL_ONE);
					break;
				case ALPHA_BLENDTWO:
					glBlendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA);
					glBlendFunc(GL_DST_ALPHA , GL_ONE);
					break;
				case ALPHA_BLENDTHREE:
					glBlendFunc(GL_SRC_ALPHA , GL_ONE);
					glBlendFunc(GL_DST_ALPHA , GL_SRC_ALPHA);
					break;
				case ALPHA_ALPHACHANNELS:
					glBlendFunc(GL_SRC_ALPHA , GL_SRC_ALPHA);
					break;
				case ALPHA_DESTALPHA:
					glBlendFunc(GL_SRC_ALPHA , GL_DST_ALPHA);
					break;
				case ALPHA_MULTIPLY:
					glBlendFunc(GL_DST_COLOR,GL_ZERO);
					break;
				case ALPHA_NONE:
					// AVOID COMPILER WARNING
					break;
				case ALPHA_CUSTOM:
					break;
			}

		}

		mLastBlend = blend;
	}
}

void cTextureFactory::SetActiveTextureUnit( const Uint32& Unit ) {
	glActiveTextureARB( GL_TEXTURE0_ARB + Unit );
}

void cTextureFactory::SetTextureConstantColor( const eeColorA& Color ) {
	SetTextureConstantColor( eeColorAf( (eeFloat)Color.R() / 255.f, (eeFloat)Color.G() / 255.f, (eeFloat)Color.B() / 255.f, (eeFloat)Color.A() / 255.f ) );
}

void cTextureFactory::SetTextureConstantColor( const eeColorAf& Color ) {
	glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, (const GLfloat*)(&Color.Red) );
}

void cTextureFactory::SetTextureEnv( const EE_TEXTURE_PARAM& Param, const Int32& Val ) {
	GLenum lParam = (GLenum)cGL::instance()->GetTextureParamEnum( Param );

	glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB );

	if( Param == TEX_PARAM_COLOR_FUNC || Param == TEX_PARAM_ALPHA_FUNC ) {
		glTexEnvi( GL_TEXTURE_ENV, lParam, cGL::instance()->GetTextureFuncEnum( (EE_TEXTURE_FUNC)Val ) );
	} else if( Param >= TEX_PARAM_COLOR_SOURCE_0 && Param <= TEX_PARAM_ALPHA_SOURCE_2 ) {
		glTexEnvi( GL_TEXTURE_ENV, lParam, cGL::instance()->GetTextureSourceEnum( (EE_TEXTURE_SOURCE)Val ) );
	} else if( Param >= TEX_PARAM_COLOR_OP_0 && Param <= TEX_PARAM_ALPHA_OP_2 ) {
		glTexEnvi( GL_TEXTURE_ENV, lParam, cGL::instance()->GetTextureOpEnum( (EE_TEXTURE_OP)Val ) );
	} else {
		glTexEnvi( GL_TEXTURE_ENV, lParam, Val );
	}
}

const EE_PRE_BLEND_FUNC& cTextureFactory::GetPreBlendFunc() const {
	return mLastBlend;
}

eeUint cTextureFactory::GetValidTextureSize(const eeUint& Size) {
	if ( cGL::instance()->IsExtension( EEGL_ARB_texture_non_power_of_two ) )
		return Size;
	else
		return NextPowOfTwo(Size);
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
			mVectorFreeSlots.push( i );
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
	return GetByHash( MakeHash( Name ) );
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
