#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"
#include "ctextureloader.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

cTextureFactory::cTextureFactory() :
	mCurrentTexture(0),
	mIsCalcPowOfTwo(false),
	mMemSize(0)
{
	mTextures.clear();
	mTextures.push_back( NULL );

	mAppPath = AppPath();
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
	Tex = mTextures[ Pos ] = new cTexture();

	Tex->Create( TexId, Width, Height, MyWidth, MyHeight, Mipmap, Channels, FPath, ColorKey, ClampMode, CompressTexture, MemSize );
	Tex->TexId( Pos );

	if ( !ColorKey.voidRGB )
		Tex->CreateMaskFromColor( eeColor( ColorKey.R(), ColorKey.G(), ColorKey.B() ) , 0 );

	if ( LocalCopy ) {
		Tex->Lock();
		Tex->Unlock( true, false );
	}

	mMemSize += GetTexMemSize( Pos );

	Unlock();

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

void cTextureFactory::Bind( const cTexture* Tex ) {
	if( NULL != Tex && mCurrentTexture != (Int32)Tex->Texture() ) {
		glBindTexture( GL_TEXTURE_2D, Tex->Texture() );
		mCurrentTexture = Tex->Texture();
	}
}

void cTextureFactory::Bind( const Uint32& TexId ) {
	Bind( GetTexture( TexId ) );
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

bool cTextureFactory::Remove( const Uint32& TexId ) {
	if ( TexId < mTextures.size() && NULL != mTextures[ TexId ] ) {
		mMemSize -= GetTexMemSize( TexId );

		GLint glTexId = mTextures[ TexId ]->Texture();

		eeSAFE_DELETE( mTextures[ TexId ] );

		if ( mCurrentTexture == (Int32)glTexId )
			mCurrentTexture = 0;

		mVectorFreeSlots.push( TexId );

		return true;
	}

	return false;
}

GLint cTextureFactory::GetCurrentTexture() const {
	return mCurrentTexture;
}

void cTextureFactory::SetCurrentTexture( const GLint& TexId ) {
	mCurrentTexture = TexId;
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

void cTextureFactory::SetBlendFunc( const EE_RENDERALPHAS& blend, const bool& force ) {
	if ( mLastBlend != blend || force ) {
		if (blend == ALPHA_NONE)
			glDisable( GL_BLEND );
		else {
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
			}

		}
		mLastBlend = blend;
	}
}

eeUint cTextureFactory::GetValidTextureSize(const eeUint& Size) {
	if (!mIsCalcPowOfTwo) {
		if ( cEngine::instance()->Running() ) { // This need the GL context initialized
			char *extensions = (char *)glGetString(GL_EXTENSIONS);

			if ( strstr(extensions, "GL_ARB_texture_non_power_of_two") )
				mPowOfTwo = false;
			else
				mPowOfTwo = true;

			mIsCalcPowOfTwo = true;
		} else
			mPowOfTwo = false;
	}

	if ( !mPowOfTwo )
		return Size;
	else
		return NextPowOfTwo(Size);
}

bool cTextureFactory::SaveImage( const std::string& filepath, const EE_SAVETYPE& Format, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const unsigned char* data ) {
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

		if ( NULL != tTex && tTex->Id() == Hash )
			return mTextures[ i ];
	}

	return NULL;
}

}}
