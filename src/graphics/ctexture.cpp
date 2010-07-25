#include "ctexture.hpp"
#include "cglobalbatchrenderer.hpp"

namespace EE { namespace Graphics {

cTexture::cTexture() :
	mPixels(NULL),
	mFilepath(""),
	mId(0),
	mTexture(0),
	mWidth(0),
	mHeight(0),
	mImgWidth(0),
	mImgHeight(0),
	mChannels(4),
	mMipmap(true),
	mModified(false),
	mColorKey(true),
	mClampMode( EE_CLAMP_TO_EDGE ),
	mFilter( TEX_LINEAR ),
	mCompressedTexture(false),
	mLocked(false),
	mGrabed(false)
{
}

cTexture::cTexture( const cTexture& Copy ) :
	mFilepath( Copy.mFilepath ),
	mId( Copy.mId ),
	mTexture( Copy.mTexture ),
	mWidth( Copy.mWidth ),
	mHeight( Copy.mHeight ),
	mImgWidth( Copy.mImgWidth ),
	mImgHeight( Copy.mImgHeight ),
	mChannels( Copy.mChannels ),
	mMipmap( Copy.mMipmap ),
	mModified( Copy.mModified ),
	mColorKey( Copy.mColorKey ),
	mClampMode( Copy.mClampMode ),
	mFilter( Copy.mFilter ),
	mCompressedTexture( Copy.mCompressedTexture ),
	mLocked( Copy.mLocked ),
	mGrabed ( Copy.mGrabed )
{
	Pixels( reinterpret_cast<const Uint8*>( &Copy.mPixels[0] ) );
}

cTexture::~cTexture() {
	DeleteTexture();
}

cTexture& cTexture::operator =(const cTexture& Other) {
    cTexture Temp(Other);

	std::swap(mFilepath, Temp.mFilepath);
	std::swap(mId, Temp.mId);
    std::swap(mTexture, Temp.mTexture);
	std::swap(mWidth, Temp.mWidth);
	std::swap(mHeight, Temp.mHeight);
	std::swap(mImgWidth, Temp.mImgWidth);
	std::swap(mImgHeight, Temp.mImgHeight);
	std::swap(mMipmap, Temp.mMipmap);
	std::swap(mModified, Temp.mModified);
	std::swap(mColorKey, Temp.mColorKey);
	std::swap(mLocked, Temp.mLocked);
	std::swap(mFilter, Temp.mFilter);
	std::swap(mClampMode, Temp.mClampMode);
	std::swap(mCompressedTexture, Temp.mCompressedTexture);
	std::swap(mGrabed, Temp.mGrabed);
	std::swap(mChannels, Temp.mChannels);
	Pixels( reinterpret_cast<const Uint8*>( &Temp.mPixels[0] ) );

    return *this;
}

void cTexture::DeleteTexture() {
	if ( mTexture ) {
		GLuint Texture = static_cast<GLuint>(mTexture);
		glDeleteTextures(1, &Texture);

		mTexture = 0;
		mModified = false;
		mLocked = false;
		mGrabed = false;

		ClearCache();
	}
}

cTexture::cTexture( const Uint32& texture, const eeInt& width, const eeInt& height, const eeInt& imgwidth, const eeInt& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint8* data ) {
	Create( texture, width, height, imgwidth, imgheight, UseMipmap, Channels, filepath, ColorKey, ClampMode, CompressedTexture, data );
}

void cTexture::Create( const Uint32& texture, const eeInt& width, const eeInt& height, const eeInt& imgwidth, const eeInt& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const eeRGB& ColorKey, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint8* data ) {
	mFilepath = filepath;
	mId = MakeHash( mFilepath );

	mTexture = texture;
	mWidth = width;
	mHeight = height;
	mImgWidth = imgwidth;
	mImgHeight = imgheight;
	mMipmap = UseMipmap;

	mColorKey = ColorKey;
	mClampMode = ClampMode;
	mCompressedTexture = CompressedTexture;
	mChannels = Channels;

	mLocked = false;
	mGrabed = false;

	mFilter = TEX_LINEAR;

	Pixels(data);
}

void cTexture::Pixels( const Uint8* data ) {
	if ( data != NULL ) {
		eeUint size = (eeUint)mWidth * (eeUint)mHeight;

		Allocate( size );

		memcpy( reinterpret_cast<void*>( &mPixels[0] ), reinterpret_cast<const void*> ( data ), size * mChannels );
	}
}

Uint8 * cTexture::Lock( const bool& ForceRGBA ) {
	if ( !mLocked ) {
		if ( ForceRGBA )
			mChannels = 4;
		
		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, mTexture);

		Int32 width = 0, height = 0;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

		mWidth = (eeInt)width;
		mHeight = (eeInt)height;
		eeUint size = (eeUint)mWidth * (eeUint)mHeight;

		Allocate( size );

		Uint32 Channel = GL_RGBA;

		if ( 3 == mChannels )
			Channel = GL_RGB;
		else if ( 2 == mChannels )
			Channel = GL_LUMINANCE_ALPHA;
		else if ( 1 == mChannels )
			Channel = GL_ALPHA;

		glGetTexImage( GL_TEXTURE_2D, 0, Channel, GL_UNSIGNED_BYTE, reinterpret_cast<Uint8*> (&mPixels[0]) );

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);

		mLocked = true;
	}

	return &mPixels[0];
}

bool cTexture::Unlock( const bool& KeepData, const bool& Modified ) {
	if ( mLocked ) {
		Int32 width = 0, height = 0;
		GLuint NTexId = 0;

		if ( Modified || mModified )	{
			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, mTexture);

			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

			Uint32 flags = mMipmap ? SOIL_FLAG_MIPMAPS : 0;
			flags = (mClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
			flags = (mCompressedTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

			NTexId = SOIL_create_OGL_texture( reinterpret_cast<Uint8*>(&mPixels[0]), &width, &height, mChannels, mTexture, flags );

			SetTextureFilter(mFilter);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);

			mModified = false;
		}

		if (!KeepData) {
			ClearCache();
		}

		mLocked = false;

		if ( (eeInt)NTexId == mTexture || !Modified )
			return true;
	}

	return false;
}

const Uint8* cTexture::GetPixelsPtr() {
	if ( !LocalCopy() ) {
		Lock();
		Unlock(true);
	}

	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

eeColorA cTexture::GetPixel( const eeUint& x, const eeUint& y ) {
	if ( mPixels == NULL || (eeInt)x > mWidth || (eeInt)y > mHeight ) {
		return eeColorA::Black;
	}

	eeUint Pos = ( x + y * mWidth ) * mChannels;
	
	if ( 4 == mChannels )
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], mPixels[ Pos + 3 ] );
	else if ( 3 == mChannels )
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], 255 );
	else if ( 2 == mChannels )
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], 255, 255 );
	else
		return eeColorA( mPixels[ Pos ], 255, 255, 255 );
}

void cTexture::SetPixel(const eeUint& x, const eeUint& y, const eeColorA& Color) {
	if ( mPixels == NULL || (eeInt)x > mWidth || (eeInt)y > mHeight ) {
		return;
	}

	eeUint Pos = ( x + y * mWidth ) * mChannels;

	for ( Uint32 i = 0; i < mChannels; i++ ) {
		if ( 0 == i )
			mPixels[ Pos + i ] = Color.R();
		else if ( 1 == i )
			mPixels[ Pos + i ] = Color.G();
		else if ( 2 == i )
			mPixels[ Pos + i ] = Color.B();
		else if ( 3 == i )
			mPixels[ Pos + i ] = Color.A();
	}

	mModified = true;
}

bool cTexture::SaveToFile(const std::string& filepath, const EE_SAVETYPE& Format) {
	bool Res = false;

	Lock();

	if (mTexture)
		Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, (Int32)mWidth, (Int32)mHeight, 4, GetPixelsPtr() ) );

	Unlock();

	return Res;
}

void cTexture::SetTextureFilter(const EE_TEX_FILTER& filter) {
	if (mTexture) {
		if ( mFilter != filter ) {
			mFilter = filter;

			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, mTexture);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mFilter == TEX_LINEAR) ? GL_LINEAR : GL_NEAREST);

			if ( mMipmap )
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
			else
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_LINEAR) ? GL_LINEAR : GL_NEAREST);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);
		}
	}
}

void cTexture::ReplaceColor(eeColorA ColorKey, eeColorA NewColor) {
	Lock( true );

	eeUint Pos = 0;

	for ( eeInt i = 0; i < mWidth * mHeight; i++ ) {
		Pos = i * mChannels;

		if ( mPixels[ Pos ] == ColorKey.R() && mPixels[ Pos + 1 ] == ColorKey.G() && mPixels[ Pos + 2 ] == ColorKey.B() && mPixels[ Pos + 3 ] == ColorKey.A() ) {
			mPixels[ Pos ] 		= NewColor.R();
			mPixels[ Pos + 1 ]	= NewColor.G();
			mPixels[ Pos + 2 ]	= NewColor.B();
			mPixels[ Pos + 3 ]	= NewColor.A();
		}
	}

	Unlock(false, true);
}

void cTexture::CreateMaskFromColor(eeColorA ColorKey, Uint8 Alpha) {
	ReplaceColor( ColorKey, eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );
}

void cTexture::CreateMaskFromColor(eeColor ColorKey, Uint8 Alpha) {
	CreateMaskFromColor( eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), 255 ), Alpha );
}

bool cTexture::LocalCopy() {
	return ( mPixels != NULL );
}

void cTexture::ClampMode( const EE_CLAMP_MODE& clampmode ) {
	if ( mClampMode != clampmode ) {
		ApplyClampMode();
		mClampMode = clampmode;
	}
}

void cTexture::ApplyClampMode() {
	if (mTexture) {
		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, mTexture);

		if( mClampMode == EE_CLAMP_REPEAT ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		} else {
			unsigned int clamp_mode = 0x812F; // GL_CLAMP_TO_EDGE
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_mode );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_mode );
		}

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);
	}
}

void cTexture::ClearCache() {
	eeSAFE_DELETE_ARRAY( mPixels );
}


void cTexture::TexId( const Uint32& id ) {
	mTexId = id;
}

const Uint32& cTexture::TexId() const {
	return mTexId;
}

void cTexture::Allocate( const Uint32& size ) {
	if ( eeARRAY_SIZE( mPixels ) != size ) {
		ClearCache();
	
		mPixels = new unsigned char[ size * mChannels ];
	}
}

void cTexture::Reload()  {
	if ( LocalCopy() ) {
		Int32 width = mWidth;
		Int32 height = mHeight;

		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		Uint32 flags = mMipmap ? SOIL_FLAG_MIPMAPS : 0;
		flags = (mClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;
		flags = (mCompressedTexture) ? ( flags | SOIL_FLAG_COMPRESS_TO_DXT ) : flags;

		mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags );

		glBindTexture(GL_TEXTURE_2D, PreviousTexture);
	} else {
		Lock();
		Reload();
		Unlock();
	}
}

const Uint32& cTexture::Id() const {
	return mId;
}

void cTexture::Draw( const eeFloat &x, const eeFloat &y, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color, const EE_RENDERALPHAS &blend, const EE_RENDERTYPE &Effect, const bool &ScaleCentered, const eeRecti& texSector) {
	DrawEx( x, y, 0, 0, Angle, Scale, Color, Color, Color, Color, blend, Effect, ScaleCentered, texSector);
}

void cTexture::DrawFast( const eeFloat& x, const eeFloat& y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color, const EE_RENDERALPHAS &blend, const eeFloat &width, const eeFloat &height ) {
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	eeFloat w = width, h = height;
	if (!w) w = (eeFloat)ImgWidth();
	if (!h) h = (eeFloat)ImgHeight();

	BR->SetTexture( this );
	BR->SetBlendFunc( blend );

	BR->QuadsBegin();
	BR->QuadsSetColor( Color );

	if ( ClampMode() == EE_CLAMP_REPEAT )
		BR->QuadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );

	BR->BatchQuadEx( x, y, w, h, Angle, Scale );

	BR->DrawOpt();
}

void cTexture::DrawEx( const eeFloat &x, const eeFloat &y, const eeFloat &width, const eeFloat &height, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_RENDERALPHAS &blend, const EE_RENDERTYPE &Effect, const bool &ScaleCentered, const eeRecti& texSector) {
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	bool renderdiv = true;
	eeFloat mx = x;
	eeFloat my = y;
	eeFloat iwidth, iheight;

	eeRecti Sector = texSector;

	eeFloat w =	(eeFloat)ImgWidth();
	eeFloat h = (eeFloat)ImgHeight();

	if (Sector.Right == 0 && Sector.Bottom == 0) {
		Sector.Left = 0;
		Sector.Top = 0;
		Sector.Right = ImgWidth();
		Sector.Bottom = ImgHeight();
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

	BR->SetTexture( this );
	BR->SetBlendFunc( blend );

	BR->QuadsBegin();
	BR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Effect <= 3 ) {
		if ( ClampMode() == EE_CLAMP_REPEAT ) {
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

void cTexture::DrawQuad( const eeQuad2f& Q, const eeFloat &offsetx, const eeFloat &offsety, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color, const EE_RENDERALPHAS &blend, const eeRecti& texSector) {
	DrawQuadEx( Q, offsetx, offsety, Angle, Scale, Color, Color, Color, Color, blend, texSector);
}

void cTexture::DrawQuadEx( const eeQuad2f& Q, const eeFloat &offsetx, const eeFloat &offsety, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_RENDERALPHAS &blend, const eeRecti& texSector ) {
	cBatchRenderer * BR = cGlobalBatchRenderer::instance();

	bool renderdiv = true;
	eeQuad2f mQ = Q;
	eeFloat MinX = mQ.V[0].x, MaxX = mQ.V[0].x, MinY = mQ.V[0].y, MaxY = mQ.V[0].y;
	eeVector2f QCenter;

	eeRecti Sector = texSector;

	eeFloat w =	(eeFloat)ImgWidth();
	eeFloat h = (eeFloat)ImgHeight();

	if (Sector.Right == 0 && Sector.Bottom == 0) {
		Sector.Left = 0;
		Sector.Top = 0;
		Sector.Right = ImgWidth();
		Sector.Bottom = ImgHeight();
	}

	if ( Sector.Left == 0 && Sector.Top == 0 && Sector.Right == w && Sector.Bottom == h )
		renderdiv = false;

	BR->SetTexture( this );
	BR->SetBlendFunc( blend );

	BR->QuadsBegin();
	BR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Angle != 0 ||  Scale != 1.0f || ClampMode() == EE_CLAMP_REPEAT ) {
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

	if ( ClampMode() == EE_CLAMP_REPEAT )
		BR->QuadsSetSubsetFree( 0, 0, 0, (MaxY - MinY) / h, ( MaxX - MinX ) / w, (MaxY - MinY) / h, ( MaxX - MinX ) / w, 0 );
	else if ( renderdiv )
		BR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

	BR->BatchQuadFreeEx( offsetx + mQ[0].x, offsety + mQ[0].y, offsetx + mQ[1].x, offsety + mQ[1].y, offsetx + mQ[2].x, offsety + mQ[2].y, offsetx + mQ[3].x, offsety + mQ[3].y );

	BR->DrawOpt();
}

}}
