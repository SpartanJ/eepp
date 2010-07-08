#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

cTextureFactory::cTextureFactory() :
	mCurrentTexture(0),
	mIsCalcPowOfTwo(false),
	mNextKey(1),
	mMemSize(0)
{
	mTextures.clear();
	mTextures.push_back( NULL );

	Log = cLog::instance();

	mTextures.resize( 1, NULL );

	BR = cGlobalBatchRenderer::instance();
}

cTextureFactory::~cTextureFactory() {
	UnloadTextures();
}

GLint cTextureFactory::GetPrevTex() {
	GLint PreviousTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);
	return PreviousTexture;
}

void cTextureFactory::BindPrev( const GLint& PreviousTexture ) {
	glBindTexture(GL_TEXTURE_2D, PreviousTexture);
	mCurrentTexture = PreviousTexture;
}

Uint32 cTextureFactory::CreateEmptyTexture( const eeUint& Width, const eeUint& Height, const eeColorA& DefaultColor, const bool& mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	std::vector<eeColorA> tmpTex( Width * Height, DefaultColor );

	Uint32 TexId = LoadFromPixels( reinterpret_cast<unsigned char*> ( &tmpTex[0] ), Width, Height, 4, mipmap, eeRGB(true), ClampMode, CompressTexture, KeepLocalCopy );

	tmpTex.clear();

	return TexId;
}

Uint32 cTextureFactory::LoadFromPixels( const unsigned char* Surface, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const std::string& FileName ) {
	return iLoadFromPixels( Surface, Width, Height, Channels, mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy, FileName );
}

Uint32 cTextureFactory::iLoadFromPixels( const unsigned char* Surface, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const std::string& FileName, const Uint32& TexPos ) {
	Uint32 tTexId = 0;

	if ( NULL != Surface ) {
		int width = Width;
		int height = Height;

		Uint32 flags = mipmap ? SOIL_FLAG_MIPMAPS : 0;

		flags = (ClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
		flags = (CompressTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

		GLint PreviousTexture = GetPrevTex();
		tTexId = SOIL_create_OGL_texture(Surface, &width, &height, Channels, ( ( TexPos==0 ) ? SOIL_CREATE_NEW_ID : GetTexture(TexPos)->Texture() ), flags);
		BindPrev( PreviousTexture );

		if ( tTexId )
			return iPushTexture( FileName, tTexId, Width, Height, width, height, mipmap, static_cast<Uint8>( Channels ), ColorKey, ClampMode, CompressTexture, KeepLocalCopy, TexPos );

	} else {
		Log->Write( SOIL_last_result() );
	}

	return 0;
}

Uint32 cTextureFactory::LoadFromPack( cPack* Pack, const std::string& FilePackPath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy  ) {
	std::vector<Uint8> TmpData;

	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, TmpData ) )
		return LoadFromMemory( reinterpret_cast<const Uint8*> (&TmpData[0]), TmpData.size(), mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy );

	return 0;
}

Uint32 cTextureFactory::LoadFromMemory( const unsigned char* Surface, const eeUint& Size, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	int ImgWidth, ImgHeight, ImgChannels;

	unsigned char * PixelsPtr = SOIL_load_image_from_memory(Surface, Size, &ImgWidth, &ImgHeight, &ImgChannels, SOIL_LOAD_AUTO);

	if ( NULL != PixelsPtr )
		return LoadFromPixels( PixelsPtr, ImgWidth, ImgHeight, ImgChannels, mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy );
	else
		Log->Write( SOIL_last_result() );

	return 0;
}

Uint32 cTextureFactory::Load( const std::string& filepath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	return iLoad( filepath, mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy, 0 );
}

Uint32 cTextureFactory::iLoad( const std::string& filepath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const Uint32& TexPos ) {
	int ImgWidth, ImgHeight, ImgChannels;

	if ( FileExists( filepath ) ) {
		unsigned char * PixelsPtr = SOIL_load_image(filepath.c_str(), &ImgWidth, &ImgHeight, &ImgChannels, SOIL_LOAD_AUTO);

		if ( NULL != PixelsPtr ) {
			Uint32 Result = iLoadFromPixels( PixelsPtr, ImgWidth, ImgHeight, ImgChannels, mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy, filepath, TexPos );

			SOIL_free_image_data( PixelsPtr );

			return Result;
		} else
			Log->Write( SOIL_last_result() );
	}

	return 0;
}

Uint32 cTextureFactory::PushTexture( const std::string& filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& LocalCopy ) {
	return iPushTexture( filepath, TexId, Width, Height, ImgWidth, ImgHeight, Mipmap, Channels, ColorKey, ClampMode, CompressTexture, LocalCopy );
}

Uint32 cTextureFactory::iPushTexture( const std::string& filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& LocalCopy, const Uint32& TexPos ) {
	cTexture * Tex 		= NULL;
	Uint32 Pos 			= mNextKey;
	eeInt MyWidth 		= ImgWidth;
	eeInt MyHeight 		= ImgHeight;

	if ( TexPos != 0 ) {
		Pos = TexPos;
		Tex = GetTexture( TexPos );

		// Recover the real image size
		if ( NULL != Tex && TexId == Tex->Texture() ) {
			Tex->Width( Width );
			Tex->Height( Height );

			MyWidth = Tex->ImgWidth();
			MyHeight = Tex->ImgHeight();

			mMemSize -= GetTexMemSize( TexPos );
		}
	} else {
		mNextKey++;
	}

	if ( Pos == mTextures.size() ) {
		mTextures.push_back( new cTexture() );
	} else if ( mTextures[ Pos ] == NULL ) {
		mTextures[ Pos ] = new cTexture();
	}

	Tex = GetTexture( Pos );
	Tex->Create( TexId, Width, Height, MyWidth, MyHeight, Mipmap, Channels, filepath, ColorKey, ClampMode, CompressTexture );
	Tex->TexId( Pos );

	if ( !ColorKey.voidRGB )
		Tex->CreateMaskFromColor( eeColor( ColorKey.R(), ColorKey.G(), ColorKey.B() ) , 0 );

	if ( LocalCopy ) {
		Tex->Lock();
		Tex->Unlock( true, false );
	}

	mMemSize += GetTexMemSize( Pos );

	return Pos;
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
			if ( mTextures[i] != NULL )
				delete mTextures[i];

		mTextures.clear();

		Log->Write( "Textures Unloaded." );
	} catch (...) {
		Log->Write("An error ocurred on: UnloadTextures.");
	}
}

bool cTextureFactory::Remove( const Uint32& TexId ) {
	if ( TexId < mTextures.size() && TexId > 0 ) {
		mMemSize -= GetTexMemSize( TexId );

		GLint glTexId = mTextures[ TexId ]->Texture();

		eeSAFE_DELETE( mTextures[ TexId ] );

		if ( mCurrentTexture == (Int32)glTexId )
			mCurrentTexture = 0;

		return true;
	}
	return false;
}

Uint32 cTextureFactory::Reload( const Uint32& TexId, const std::string& filepath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture ) {
	Uint32 Id = 0;
	if ( !filepath.empty() ) {
		Id = iLoad(filepath, mipmap, ColorKey, ClampMode, CompressTexture, TexId);
		return Id;
	} else {
		cTexture* Tex = GetTexture(TexId);

		if ( Tex->LocalCopy() )
			Id = iLoadFromPixels( Tex->GetPixelsPtr(), (int)Tex->Width(), (int)Tex->Height(), SOIL_LOAD_RGBA, Tex->Mipmap(), Tex->ColorKey(), Tex->ClampMode(), Tex->Compressed(), ( Tex->LocalCopy() && !Tex->Grabed() ), Tex->Filepath(), TexId );
		else
			Id = iLoad( Tex->Filepath(), Tex->Mipmap(), Tex->ColorKey(), Tex->ClampMode(), Tex->Compressed(), TexId );

		return Id;
	}
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
			if ( Tex ) {
				if ( ( Tex->Filepath() != "" && FileExists( Tex->Filepath() ) ) || Tex->LocalCopy() )
					Reload(i);
				else {
					Tex->Lock();
					Reload(i);
					Tex->Unlock(false, false);
				}
			}
		}
		Log->Write("Textures Reloaded.");
	} catch (...) {
		Log->Write("An error ocurred on: ReloadAllTextures.");
	}
}

void cTextureFactory::GrabTextures() {
	for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
		cTexture* Tex = GetTexture(i);
		if ( Tex ) {
			if ( !( Tex->Filepath() != "" || Tex->LocalCopy() ) ) {
				Tex->Lock();
				Tex->Unlock(true, false);
				Tex->Grabed(true);
			}
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
					// COMPILER WARNING MY BALLS
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
	}
}

eeUint cTextureFactory::GetTexMemSize( const eeUint& TexId ) {
	eeUint Size = 0;

	if ( TexId < mTextures.size() && TexId > 0 ) {
		cTexture* Tex = mTextures[ TexId ];

		if ( Tex != NULL ) {
			eeUint w = static_cast<eeUint>( Tex->Width() );
			eeUint h = static_cast<eeUint>( Tex->Height() );
			Size = ( w * h * Tex->Channels() );

			if( Tex->Mipmap() ) {
				while(w > 2 && h > 2) {
					w>>=1;
					h>>=1;
					Size += ( w * h * Tex->Channels() );
				}
			}
		}

		return Size;
	}

	return 0;
}

}}
