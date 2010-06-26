#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"

using namespace EE::Window;

namespace EE { namespace Graphics {

cTextureFactory::cTextureFactory() : mCurrentTexture(0), mIsCalcPowOfTwo(false), mNextKey(1), mMemSize(0) {
	mTextures.clear();
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

eeUint cTextureFactory::CreateEmptyTexture( const eeUint& Width, const eeUint& Height, const eeColorA& DefaultColor, const bool& mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	Uint32 flags = mipmap ? SOIL_FLAG_MIPMAPS : 0;
	Uint32 TexId;
	std::vector<eeColorA> tmpTex( Width * Height, DefaultColor );

	int width = (int)Width;
	int height = (int)Height;

	flags = (ClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
	flags = (CompressTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

	GLint PreviousTexture = GetPrevTex();
	TexId = SOIL_create_OGL_texture( reinterpret_cast<unsigned char*>( &tmpTex[0] ), &width, &height, SOIL_LOAD_RGBA, SOIL_CREATE_NEW_ID, flags);
	BindPrev( PreviousTexture );

	if ( TexId ) {
		tmpTex.clear();
		Log->Write( "Empty Texture created." );
		return PushTexture("", TexId, width, height, width, height, mipmap, SOIL_LOAD_RGBA, eeRGB(true), ClampMode, CompressTexture, 0, KeepLocalCopy);
	}
	
	Log->Write( SOIL_last_result() );
	
	return 0;
}

eeUint cTextureFactory::Load( const std::string& filepath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture ) {
	return iLoad(filepath, mipmap, ColorKey, ClampMode, CompressTexture, 0);
}

eeUint cTextureFactory::iLoad( const std::string& filepath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const Uint32& TexPos ) {
	int ImgWidth, ImgHeight, ImgChannels;
	Uint32 flags = mipmap ? SOIL_FLAG_MIPMAPS : 0;

	Uint32 TexId = 0;

	if ( FileExists( filepath ) ) {
		unsigned char* PixelsPtr = SOIL_load_image(filepath.c_str(), &ImgWidth, &ImgHeight, &ImgChannels, SOIL_LOAD_AUTO);

		eeInt RealImgWidth = ImgWidth;
		eeInt RealImgHeight = ImgHeight;

		if (PixelsPtr) {
			flags = (ClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
			flags = (CompressTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

			GLint PreviousTexture = GetPrevTex();
			TexId = SOIL_create_OGL_texture(PixelsPtr, &ImgWidth, &ImgHeight, ImgChannels, ( ( TexPos==0 ) ? SOIL_CREATE_NEW_ID : GetTexture(TexPos)->Texture() ), flags);

			SOIL_free_image_data(PixelsPtr);
			BindPrev( PreviousTexture );

			Log->Write( "Texture " + filepath + " loaded." );

			return PushTexture(filepath, TexId, ImgWidth, ImgHeight, RealImgWidth, RealImgHeight, mipmap, static_cast<Uint8>( ImgChannels ), ColorKey, ClampMode, CompressTexture, TexPos );
		} else {
			Log->Write( SOIL_last_result() );
		}
	}
	
	return 0;
}

eeUint cTextureFactory::LoadFromPixels( const unsigned char* Surface, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	return iLoadFromPixels( Surface, Width, Height, Channels, mipmap, ClampMode, CompressTexture, KeepLocalCopy );
}

eeUint cTextureFactory::iLoadFromPixels( const unsigned char* Surface, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const bool& mipmap, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy, const Uint32& TexPos ) {
	Uint32 cTexId = 0;
	
	if ( NULL != Surface ) {
		int width = Width;
		int height = Height;
		
		Uint32 flags = mipmap ? SOIL_FLAG_MIPMAPS : 0;
		
		flags = (ClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
		flags = (CompressTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;
		
		GLint PreviousTexture = GetPrevTex();
		cTexId = SOIL_create_OGL_texture(Surface, &width, &height, Channels, ( ( TexPos==0 ) ? SOIL_CREATE_NEW_ID : GetTexture(TexPos)->Texture() ), flags);
		BindPrev( PreviousTexture );

		if (cTexId) {
			Log->Write( "Texture loaded from memory ( RAW format )." );

			return PushTexture("", cTexId, width, height, width, height, mipmap, static_cast<Uint8>( Channels ), eeRGB(true), ClampMode, CompressTexture, TexPos, KeepLocalCopy);
		}
	} else {
		Log->Write( SOIL_last_result() );
	}
	
	return 0;
}

eeUint cTextureFactory::LoadFromPack( cPack* Pack, const std::string& FilePackPath, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy  ) {
	std::vector<Uint8> TmpData;

	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, TmpData ) )
		return LoadFromMemory( reinterpret_cast<const Uint8*> (&TmpData[0]), TmpData.size(), mipmap, ColorKey, ClampMode, CompressTexture, KeepLocalCopy );

	return 0;
}

eeUint cTextureFactory::LoadFromMemory( const unsigned char* Surface, const eeUint& Size, const bool& mipmap, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const bool& KeepLocalCopy ) {
	Uint32 cTexId = 0;
	int ImgWidth, ImgHeight, ImgChannels;

	unsigned char* PixelsPtr = SOIL_load_image_from_memory(Surface, Size, &ImgWidth, &ImgHeight, &ImgChannels, SOIL_LOAD_AUTO);

	eeInt RealImgWidth = ImgWidth;
	eeInt RealImgHeight = ImgHeight;

	if (PixelsPtr) {
		Uint32 flags = mipmap ? SOIL_FLAG_MIPMAPS : 0;

		flags = (ClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
		flags = (CompressTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

		GLint PreviousTexture = GetPrevTex();
		cTexId = SOIL_create_OGL_texture(PixelsPtr, &ImgWidth, &ImgHeight, ImgChannels, SOIL_CREATE_NEW_ID, flags);

		SOIL_free_image_data(PixelsPtr);
		BindPrev( PreviousTexture );

		if (cTexId) {
			Log->Write( "Texture loaded from memory." );

			return PushTexture("", cTexId, ImgWidth, ImgHeight, RealImgWidth, RealImgHeight, mipmap, static_cast<Uint8>( ImgChannels ), ColorKey, ClampMode, CompressTexture, 0, KeepLocalCopy);
		}
	} else
		Log->Write( SOIL_last_result() );

	return 0;
}

eeUint cTextureFactory::PushTexture(const std::string& filepath, const Uint32& TexId, const eeUint& Width, const eeUint& Height, const eeUint& ImgWidth, const eeUint& ImgHeight, const bool& Mipmap, const eeUint& Channels, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressTexture, const Uint32& TexPos, const bool& LocalCopy ) {
	Uint32 Pos = mNextKey;
	eeInt MyWidth = ImgWidth;
	eeInt MyHeight = ImgHeight;

	if (TexPos != 0) {
		Pos = TexPos;
		cTexture* Tex = GetTexture(TexPos);

		// Recover the real image size
		if ( Tex && TexId == Tex->Texture() ) {
			Tex->Width( Width );
			Tex->Height( Height );

			MyWidth = Tex->ImgWidth();
			MyHeight = Tex->ImgHeight();

			mMemSize -= GetTexMemSize( TexPos );
		}
	} else
		mNextKey++;

	if ( mNextKey >= mTextures.size() )
		mTextures.push_back( new cTexture() );
	else if ( mTextures[ mNextKey ] == NULL )
		mTextures[ mNextKey ] = new cTexture();

	cTexture* Tex = GetTexture(Pos);
	Tex->Create( TexId, Width, Height, MyWidth, MyHeight, Mipmap, Channels, filepath, ColorKey, ClampMode, CompressTexture );

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
	if( mCurrentTexture != (Int32)Tex->Texture() ) {
		glBindTexture( GL_TEXTURE_2D, Tex->Texture() );
		mCurrentTexture = Tex->Texture();
	}
}

void cTextureFactory::Bind( const Uint32& TexId ) {
	if (TexId < 1) return;
	cTexture* Tex = GetTexture(TexId);

	if( mCurrentTexture != (Int32)Tex->Texture() ) {
		glBindTexture( GL_TEXTURE_2D, Tex->Texture() );
		mCurrentTexture =  Tex->Texture();
	}
}

void cTextureFactory::Draw(const Uint32 &TexId, const eeFloat &x, const eeFloat &y, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color, const EE_RENDERALPHAS &blend, const EE_RENDERTYPE &Effect, const bool &ScaleCentered, const eeRecti& texSector) {
	DrawEx(TexId, x, y, 0, 0, Angle, Scale, Color, Color, Color, Color, blend, Effect, ScaleCentered, texSector);
}

void cTextureFactory::DrawFast( const Uint32& TexId, const eeFloat& x, const eeFloat& y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color, const EE_RENDERALPHAS &blend, const eeFloat &width, const eeFloat &height ) {
	cTexture* Tex = GetTexture(TexId);
	eeFloat w = width, h = height;
	if (!w) w = (eeFloat)Tex->ImgWidth();
	if (!h) h = (eeFloat)Tex->ImgHeight();

	BR->SetTexture( TexId );
	BR->SetBlendFunc( blend );

	BR->QuadsBegin();
	BR->QuadsSetColor( Color );

	if ( Tex->ClampMode() == EE_CLAMP_REPEAT )
		BR->QuadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );

	BR->BatchQuadEx( x, y, w, h, Angle, Scale );

	BR->DrawOpt();
}

void cTextureFactory::DrawEx(const Uint32 &TexId, const eeFloat &x, const eeFloat &y, const eeFloat &width, const eeFloat &height, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_RENDERALPHAS &blend, const EE_RENDERTYPE &Effect, const bool &ScaleCentered, const eeRecti& texSector) {
	bool renderdiv = true;
	eeFloat mx = x;
	eeFloat my = y;
	eeFloat iwidth, iheight;

	eeRecti Sector = texSector;
	cTexture* Tex = GetTexture(TexId);

	eeFloat w =	(eeFloat)Tex->ImgWidth();
	eeFloat h = (eeFloat)Tex->ImgHeight();

	if (Sector.Right == 0 && Sector.Bottom == 0) {
		Sector.Left = 0;
		Sector.Top = 0;
		Sector.Right = Tex->ImgWidth();
		Sector.Bottom = Tex->ImgHeight();
	}

	if (!width && !height) {
		iwidth = static_cast<eeFloat> (Sector.Right - Sector.Left);
		iheight = static_cast<eeFloat> (Sector.Bottom - Sector.Top);
	} else {
		iwidth = width;
		iheight = height;
	}

	if ( Scale != 1.0f ) {
		if ( ScaleCentered ) {
			eeFloat halfW = w * 0.5f;
			eeFloat halfH = h * 0.5f;
			mx = mx + halfW - halfW * Scale;
			my = my + halfH - halfH * Scale;
		}
		iwidth *= Scale;
		iheight *= Scale;
	}

	if ( Sector.Left == 0 && Sector.Top == 0 && Sector.Right == w && Sector.Bottom == h )
		renderdiv = false;

	BR->SetTexture( TexId );
	BR->SetBlendFunc( blend );

	BR->QuadsBegin();
	BR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Effect <= 3 ) {
		if ( Tex->ClampMode() == EE_CLAMP_REPEAT ) {
			if ( Effect == RN_NORMAL )
				BR->QuadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );
			else if ( Effect == RN_MIRROR )
				BR->QuadsSetSubsetFree( width / w, 0, width / w, height / h, 0, height / h, 0, 0 );
			else if ( RN_FLIP )
				BR->QuadsSetSubsetFree( 0, height / h, 0, 0, width / w, 0, width / w, height / h );
			else
				BR->QuadsSetSubsetFree( width / w, height / h, width / w, 0, 0, 0, 0, height / h );
		} else {
			if ( Effect == RN_NORMAL ) {
				if ( renderdiv )
					BR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );
			} else if ( Effect == RN_MIRROR ) {
				if ( renderdiv )
					BR->QuadsSetSubsetFree( Sector.Right / w, Sector.Top / h, Sector.Right / w, Sector.Bottom / h, Sector.Left / w, Sector.Bottom / h, Sector.Left / w, Sector.Top / h );
				else
					BR->QuadsSetSubsetFree( 1, 0, 1, 1, 0, 1, 0, 0 );
			} else if ( Effect == RN_FLIP ) {
				if ( renderdiv )
					BR->QuadsSetSubsetFree( Sector.Left / w, Sector.Bottom / h, Sector.Left / w, Sector.Top / h, Sector.Right / w, Sector.Top / h, Sector.Right / w, Sector.Bottom / h );
				else
					BR->QuadsSetSubsetFree( 0, 1, 0, 0, 1, 0, 1, 1 );
			} else if ( Effect == RN_FLIPMIRROR ) {
				if ( renderdiv )
					BR->QuadsSetSubsetFree( Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h, Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h );
				else
					BR->QuadsSetSubsetFree( 1, 1, 1, 0, 0, 0, 0, 1 );
			}
		}

		BR->BatchQuad( mx, my, iwidth, iheight, Angle );
	} else {
		eeRectf TmpR( mx, my, mx + iwidth, my + iheight );
		eeQuad2f Q = eeQuad2f( eeVector2f( TmpR.Left, TmpR.Top ), eeVector2f( TmpR.Left, TmpR.Bottom ), eeVector2f( TmpR.Right, TmpR.Bottom ), eeVector2f( TmpR.Right, TmpR.Top ) );

		if ( Effect == RN_ISOMETRIC ) {
			Q.V[0].x += ( TmpR.Right - TmpR.Left );
			Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
			Q.V[3].x += ( TmpR.Right - TmpR.Left );
			Q.V[3].y += ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
		} else if ( Effect == RN_ISOMETRICVERTICAL ) {
			Q.V[0].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
			Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
		} else if ( Effect == RN_ISOMETRICVERTICALNEGATIVE ) {
			Q.V[2].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
			Q.V[3].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
		}
		if ( Angle != 0.0f ) {
			eeVector2f Center = eeVector2f ( TmpR.Left + (TmpR.Right - TmpR.Left) * 0.5f , TmpR.Top + (TmpR.Bottom - TmpR.Top) * 0.5f );
			Q.Rotate( Angle, Center );
		}

		BR->BatchQuadFree( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );
	}

	BR->DrawOpt();
}

void cTextureFactory::DrawQuad(const Uint32 &TexId, const eeQuad2f& Q, const eeFloat &offsetx, const eeFloat &offsety, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color, const EE_RENDERALPHAS &blend, const eeRecti& texSector) {
	DrawQuadEx(TexId, Q, offsetx, offsety, Angle, Scale, Color, Color, Color, Color, blend, texSector);
}

void cTextureFactory::DrawQuadEx(const Uint32 &TexId, const eeQuad2f& Q, const eeFloat &offsetx, const eeFloat &offsety, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_RENDERALPHAS &blend, const eeRecti& texSector ) {
	bool renderdiv = true;
	eeQuad2f mQ = Q;
	eeFloat MinX = mQ.V[0].x, MaxX = mQ.V[0].x, MinY = mQ.V[0].y, MaxY = mQ.V[0].y;
	eeVector2f QCenter;

	eeRecti Sector = texSector;
	cTexture* Tex = GetTexture(TexId);

	eeFloat w =	(eeFloat)Tex->ImgWidth();
	eeFloat h = (eeFloat)Tex->ImgHeight();

	if (Sector.Right == 0 && Sector.Bottom == 0) {
		Sector.Left = 0;
		Sector.Top = 0;
		Sector.Right = Tex->ImgWidth();
		Sector.Bottom = Tex->ImgHeight();
	}

	if ( Sector.Left == 0 && Sector.Top == 0 && Sector.Right == w && Sector.Bottom == h )
		renderdiv = false;

	BR->SetTexture( TexId );
	BR->SetBlendFunc( blend );

	BR->QuadsBegin();
	BR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Angle != 0 ||  Scale != 1.0f || Tex->ClampMode() == EE_CLAMP_REPEAT ) {
		for (Uint8 i = 1; i < 4; i++ ) {
			if ( MinX > Q.V[i].x ) MinX = Q.V[i].x;
			if ( MaxX < Q.V[i].x ) MaxX = Q.V[i].x;
			if ( MinY > Q.V[i].y ) MinY = Q.V[i].y;
			if ( MaxY < Q.V[i].y ) MaxY = Q.V[i].y;
		}

		QCenter.x = MinX + ( MaxX - MinX ) * 0.5f;
		QCenter.y = MinY + ( MaxY - MinY ) * 0.5f;
	}

	if ( Scale != 1.0f ) {
		for (Uint8 i = 0; i < 4; i++ ) {
			if ( mQ.V[i].x < QCenter.x )
				mQ.V[i].x = QCenter.x - fabs(QCenter.x - mQ.V[i].x) * Scale;
			else
				mQ.V[i].x = QCenter.x + fabs(QCenter.x - mQ.V[i].x) * Scale;

			if ( mQ.V[i].y < QCenter.y )
				mQ.V[i].y = QCenter.y - fabs(QCenter.y - mQ.V[i].y) * Scale;
			else
				mQ.V[i].y = QCenter.y + fabs(QCenter.y - mQ.V[i].y) * Scale;
		}
	}

	if ( Angle != 0.0f )
		mQ.Rotate( Angle, QCenter );

	if ( Tex->ClampMode() == EE_CLAMP_REPEAT )
		BR->QuadsSetSubsetFree( 0, 0, 0, (MaxY - MinY) / h, ( MaxX - MinX ) / w, (MaxY - MinY) / h, ( MaxX - MinX ) / w, 0 );
	else if ( renderdiv )
		BR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

	BR->BatchQuadFreeEx( offsetx + mQ[0].x, offsety + mQ[0].y, offsetx + mQ[1].x, offsety + mQ[1].y, offsetx + mQ[2].x, offsety + mQ[2].y, offsetx + mQ[3].x, offsety + mQ[3].y );

	BR->DrawOpt();
}

Uint32 cTextureFactory::GetTextureId(const Uint32& TexId) {
	return GetTexture(TexId)->Texture();
}

eeFloat cTextureFactory::GetTextureWidth(const Uint32& TexId) {
	return (eeFloat)GetTexture(TexId)->Width();

}

eeFloat cTextureFactory::GetTextureHeight(const Uint32& TexId) {
	return (eeFloat)GetTexture(TexId)->Height();
}

std::string cTextureFactory::GetTexturePath(const Uint32& TexId) {
	return GetTexture(TexId)->Filepath();
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
		
		delete mTextures[ TexId ];
		mTextures[ TexId ] = NULL;
		
		if ( mCurrentTexture == (Int32)TexId )
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
			Id = iLoadFromPixels( Tex->GetPixelsPtr(), (int)Tex->Width(), (int)Tex->Height(), SOIL_LOAD_RGBA, Tex->Mipmap(), Tex->ClampMode(), Tex->Compressed(), ( Tex->LocalCopy() && !Tex->Grabed() ), TexId );
		else
			Id = iLoad( Tex->Filepath(), Tex->Mipmap(), Tex->ColorKey(), Tex->ClampMode(), Tex->Compressed(), TexId );

		return Id;
	}
}

Uint32 cTextureFactory::GetCurrentTexture() const {
	return mCurrentTexture;
}

void cTextureFactory::SetCurrentTexture(const Uint32& TexId) {
	if ( TextureIdExists( TexId ) )
		mCurrentTexture = GetTexture(TexId)->Texture();
}

void cTextureFactory::ReloadAllTextures() {
	try {
		for ( Uint32 i = 1; i < mTextures.size(); i++ ) {
			cTexture* Tex = GetTexture(i);
			if ( Tex ) {
				if ( Tex->Filepath() != "" || Tex->LocalCopy() )
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

bool cTextureFactory::SaveImage(const std::string& filepath, const EE_SAVETYPE& Format, const eeUint& Width, const eeUint& Height, const eeUint& Channels, const unsigned char* data) {
	return 0 != SOIL_save_image ( filepath.c_str(), Format, Width, Height, Channels, data );
}

bool cTextureFactory::TextureIdExists( const Uint32& TexId ) {
	return ( TexId < mTextures.size() && TexId > 0 );
}

cTexture* cTextureFactory::GetTexture( const Uint32& TexId ) {
	return mTextures[TexId];
}

void cTextureFactory::Allocate( const eeUint& size ) {
	if ( size > mTextures.size() ) {
		mTextures.resize( size+1, NULL ); // 0 will be always null, so we add 1
		Log->Write( "Texture space allocated" );
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
