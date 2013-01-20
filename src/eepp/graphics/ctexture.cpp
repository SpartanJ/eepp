#include <eepp/graphics/ctexture.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/math/polygon2.hpp>

namespace EE { namespace Graphics {

static cBatchRenderer * sBR = NULL;

cTexture::cTexture() :
	cImage(),
	mFilepath(""),
	mId(0),
	mTexture(0),
	mImgWidth(0),
	mImgHeight(0),
	mFlags(0),
	mClampMode( EE_CLAMP_TO_EDGE ),
	mFilter( TEX_FILTER_LINEAR )
{
	if ( NULL == sBR ) {
		sBR = cGlobalBatchRenderer::instance();
	}
}

cTexture::cTexture( const cTexture& Copy ) :
	cImage(),
	mFilepath( Copy.mFilepath ),
	mId( Copy.mId ),
	mTexture( Copy.mTexture ),
	mImgWidth( Copy.mImgWidth ),
	mImgHeight( Copy.mImgHeight ),
	mFlags( Copy.mFlags ),
	mClampMode( Copy.mClampMode ),
	mFilter( Copy.mFilter )
{
	mWidth 		= Copy.mWidth;
	mHeight 	= Copy.mHeight;
	mChannels 	= Copy.mChannels;
	mSize 		= Copy.mSize;

	SetPixels( reinterpret_cast<const Uint8*>( &Copy.mPixels[0] ) );
}

cTexture::~cTexture() {
	DeleteTexture();

	if ( !cTextureFactory::instance()->IsErasing() ) {
		cTextureFactory::instance()->RemoveReference( this );
	}
}

void cTexture::DeleteTexture() {
	if ( mTexture ) {
		GLuint Texture = static_cast<GLuint>(mTexture);
		glDeleteTextures(1, &Texture);

		mTexture = 0;
		mFlags = 0;

		ClearCache();
	}
}

cTexture::cTexture( const Uint32& texture, const eeUint& width, const eeUint& height, const eeUint& imgwidth, const eeUint& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) {
	Create( texture, width, height, imgwidth, imgheight, UseMipmap, Channels, filepath, ClampMode, CompressedTexture, MemSize, data );
}

void cTexture::Create( const Uint32& texture, const eeUint& width, const eeUint& height, const eeUint& imgwidth, const eeUint& imgheight, const bool& UseMipmap, const eeUint& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) {
	mFilepath 	= filepath;
	mId 		= String::Hash( mFilepath );
	mTexture 	= texture;
	mWidth 		= width;
	mHeight 	= height;
	mChannels 	= Channels;
	mImgWidth 	= imgwidth;
	mImgHeight 	= imgheight;
	mSize 		= MemSize;
	mClampMode 	= ClampMode;
	mFilter 	= TEX_FILTER_LINEAR;

	if ( UseMipmap )
		mFlags |= TEX_FLAG_MIPMAP;

	if ( CompressedTexture )
		mFlags |= TEX_FLAG_COMPRESSED;

	SetPixels( data );
}

Uint8 * cTexture::iLock( const bool& ForceRGBA, const bool& KeepFormat ) {
	#ifndef EE_GLES
	if ( !( mFlags & TEX_FLAG_LOCKED ) ) {
		if ( ForceRGBA )
			mChannels = 4;

		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, mTexture);

		Int32 width = 0, height = 0;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

		mWidth = (eeUint)width;
		mHeight = (eeUint)height;
		GLint size = mWidth * mHeight * mChannels;

		if ( KeepFormat && ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &mInternalFormat );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size );
		}

		Allocate( (eeUint)size );

		if ( KeepFormat && ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			glGetCompressedTexImage( GL_TEXTURE_2D, 0, reinterpret_cast<Uint8*> (&mPixels[0]) );
		} else {
            Uint32 Channel = GL_RGBA;

            if ( 3 == mChannels )
                Channel = GL_RGB;
            else if ( 2 == mChannels )
                Channel = GL_LUMINANCE_ALPHA;
            else if ( 1 == mChannels )
                Channel = GL_ALPHA;

			glGetTexImage( GL_TEXTURE_2D, 0, Channel, GL_UNSIGNED_BYTE, reinterpret_cast<Uint8*> (&mPixels[0]) );
		}

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);

		mFlags |= TEX_FLAG_LOCKED;
	}

	return &mPixels[0];
	#else
	return NULL;
	#endif
}

Uint8 * cTexture::Lock( const bool& ForceRGBA ) {
	return iLock( ForceRGBA, false );
}

bool cTexture::Unlock( const bool& KeepData, const bool& Modified ) {
	#ifndef EE_GLES
	if ( ( mFlags & TEX_FLAG_LOCKED ) ) {
		Int32 width = mWidth, height = mHeight;
		GLuint NTexId = 0;

		if ( Modified || ( mFlags & TEX_FLAG_MODIFIED ) )	{
			GLint PreviousTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, mTexture);

			Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
			flags = (mClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

			NTexId = SOIL_create_OGL_texture( reinterpret_cast<Uint8*>(&mPixels[0]), &width, &height, mChannels, mTexture, flags );

			iTextureFilter(mFilter);

			if ( PreviousTexture != (GLint)mTexture )
				glBindTexture(GL_TEXTURE_2D, PreviousTexture);

			mFlags &= ~TEX_FLAG_MODIFIED;

			if ( mFlags & TEX_FLAG_COMPRESSED )
				mFlags &= ~TEX_FLAG_COMPRESSED;
		}

		if ( !KeepData )
			ClearCache();

		mFlags &= ~TEX_FLAG_LOCKED;

		if ( (eeInt)NTexId == mTexture || !Modified )
			return true;
	}

	return false;
	#else
	return false;
	#endif
}

const Uint8 * cTexture::GetPixelsPtr() {
	if ( !LocalCopy() ) {
		Lock();
		Unlock(true);
	}

	return cImage::GetPixelsPtr();
}

void cTexture::SetPixel( const eeUint& x, const eeUint& y, const eeColorA& Color ) {
	cImage::SetPixel( x, y, Color );

	mFlags |= TEX_FLAG_MODIFIED;
}

void cTexture::Bind() {
	cTextureFactory::instance()->Bind( this );
}

bool cTexture::SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format ) {
	bool Res = false;

	if ( mTexture ) {
		Lock();

		Res = cImage::SaveToFile( filepath, Format );

		Unlock();
	}

	return Res;
}

void cTexture::TextureFilter(const EE_TEX_FILTER& filter) {
	if ( mFilter != filter ) {
		iTextureFilter( filter );
	}
}

void cTexture::iTextureFilter( const EE_TEX_FILTER& filter ) {
	if (mTexture) {
		mFilter = filter;

		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, mTexture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mFilter == TEX_FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);

		if ( mFlags & TEX_FLAG_MIPMAP )
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_FILTER_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);

		if ( PreviousTexture != (GLint)mTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);
	}
}

const EE_TEX_FILTER& cTexture::TextureFilter() const {
	return mFilter;
}

void cTexture::ReplaceColor( const eeColorA& ColorKey, const eeColorA& NewColor ) {
	Lock();

	cImage::ReplaceColor( ColorKey, NewColor );

	Unlock( false, true );
}

void cTexture::CreateMaskFromColor( const eeColorA& ColorKey, Uint8 Alpha ) {
	Lock( true );

	cImage::ReplaceColor( ColorKey, eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );

	Unlock( false, true );
}

void cTexture::FillWithColor( const eeColorA& Color ) {
	Lock();

	cImage::FillWithColor( Color );

	Unlock( false, true );
}

void cTexture::Resize( const eeUint& new_width, const eeUint& new_height ) {
	Lock();

	cImage::Resize( new_width, new_height );

	Unlock( false, true );
}

void cTexture::CopyImage( cImage * Img, const eeUint& x, const eeUint& y ) {
	Lock();

	cImage::CopyImage( Img, x, y );

	Unlock( false, true );
}

void cTexture::Flip() {
	Lock();

	cImage::Flip();

	Unlock( false, true );

	mImgWidth 	= mWidth;
	mImgHeight 	= mHeight;
}

bool cTexture::LocalCopy() {
	return ( mPixels != NULL );
}

void cTexture::ClampMode( const EE_CLAMP_MODE& clampmode ) {
	if ( mClampMode != clampmode ) {
		mClampMode = clampmode;
		ApplyClampMode();
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

void cTexture::Id( const Uint32& id ) {
	mTexId = id;
}

const Uint32& cTexture::Id() const {
	return mTexId;
}

void cTexture::Reload()  {
	if ( LocalCopy() ) {
		Int32 width = (Int32)mWidth;
		Int32 height = (Int32)mHeight;

		GLint PreviousTexture;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &PreviousTexture);

		Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
		flags = (mClampMode == EE_CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

		if ( ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			if ( mTexture != PreviousTexture )
				glBindTexture( GL_TEXTURE_2D, mTexture );

            if ( Grabed() )
                mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags | SOIL_FLAG_COMPRESS_TO_DXT );
            else
                glCompressedTexImage2D( mTexture, 0, mInternalFormat, width, height, 0, mSize, &mPixels[0] );
		} else
			mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags );

		iTextureFilter( mFilter );

		if ( mTexture != PreviousTexture )
			glBindTexture(GL_TEXTURE_2D, PreviousTexture);
	} else {
		iLock(false,true);
		Reload();
		Unlock();
	}
}

const Uint32& cTexture::HashName() const {
	return mId;
}

void cTexture::Mipmap( const bool& UseMipmap ) {
	if ( mFlags & TEX_FLAG_MIPMAP ) {
		if ( !UseMipmap )
			mFlags &= ~TEX_FLAG_MIPMAP;
	} else {
		if ( UseMipmap )
			mFlags |= TEX_FLAG_MIPMAP;
	}
}

bool cTexture::Mipmap() const {
	return mFlags & TEX_FLAG_MIPMAP;
}

void cTexture::Grabed( const bool& isGrabed ) {
	if ( mFlags & TEX_FLAG_GRABED ) {
		if ( !isGrabed )
			mFlags &= ~TEX_FLAG_GRABED;
	} else {
		if ( isGrabed )
			mFlags |= TEX_FLAG_GRABED;
	}
}

bool cTexture::Grabed() const {
	return 0 != ( mFlags & TEX_FLAG_GRABED );
}

bool cTexture::IsCompressed() const {
	return 0 != ( mFlags & TEX_FLAG_COMPRESSED );
}

void cTexture::Draw( const eeFloat &x, const eeFloat &y, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color, const EE_BLEND_MODE &Blend, const EE_RENDER_MODE &Effect, const bool &ScaleCentered, const eeRecti& texSector) {
	DrawEx( x, y, 0, 0, Angle, Scale, Color, Color, Color, Color, Blend, Effect, ScaleCentered, texSector);
}

void cTexture::DrawFast( const eeFloat& x, const eeFloat& y, const eeFloat& Angle, const eeFloat& Scale, const eeColorA& Color, const EE_BLEND_MODE &Blend, const eeFloat &width, const eeFloat &height ) {
	eeFloat w = width, h = height;
	if (!w) w = (eeFloat)ImgWidth();
	if (!h) h = (eeFloat)ImgHeight();

	sBR->SetTexture( this );
	sBR->SetBlendMode( Blend );

	sBR->QuadsBegin();
	sBR->QuadsSetColor( Color );

	if ( ClampMode() == EE_CLAMP_REPEAT )
		sBR->QuadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );

	sBR->BatchQuadEx( x, y, w, h, Angle, Scale );

	sBR->DrawOpt();
}

void cTexture::DrawEx( const eeFloat &x, const eeFloat &y, const eeFloat &width, const eeFloat &height, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_BLEND_MODE &blend, const EE_RENDER_MODE &Effect, const bool &ScaleCentered, const eeRecti& texSector) {
	bool renderSector = true;
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
		renderSector = false;

	sBR->SetTexture( this );
	sBR->SetBlendMode( blend );

	sBR->QuadsBegin();
	sBR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Effect <= RN_FLIPMIRROR ) {
		if ( ClampMode() == EE_CLAMP_REPEAT ) {
			if ( Effect == RN_NORMAL ) {
				if ( renderSector ) {
					sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

					eeFloat sw = (eeFloat)( Sector.Right - Sector.Left );
					eeFloat sh = (eeFloat)( Sector.Bottom - Sector.Top );
					eeFloat tx = width / sw;
					eeFloat ty = height / sh;
					Int32 ttx = (Int32)tx;
					Int32 tty = (Int32)ty;
					Int32 tmpY;
					Int32 tmpX;

					for ( tmpY = 0; tmpY < tty; tmpY++ ) {
						for ( tmpX = 0; tmpX < ttx; tmpX++ ) {
							sBR->BatchQuad( mx + tmpX * sw, my + tmpY * sh, sw, sh );
						}
					}

					if ( (eeFloat)ttx != tx ) {
						eeFloat swn = ( Sector.Right - Sector.Left ) * ( tx - (eeFloat)ttx );
						eeFloat tor = Sector.Left + swn ;

						sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, tor / w, Sector.Bottom / h, tor / w, Sector.Top / h );

						for ( Int32 tmpY = 0; tmpY < tty; tmpY++ ) {
							sBR->BatchQuad( mx + ttx * sw, my + tmpY * sh, swn, sh );
						}
					}

					if ( (eeFloat)tty != ty ) {
						eeFloat shn = ( Sector.Bottom - Sector.Top ) * ( ty - (eeFloat)tty );
						eeFloat tob = Sector.Top + shn;

						sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, tob / h, Sector.Right / w, tob / h, Sector.Right / w, Sector.Top / h );

						for ( Int32 tmpX = 0; tmpX < ttx; tmpX++ ) {
							sBR->BatchQuad( mx + tmpX * sw, my + tty * sh, sw, shn );
						}
					}

					sBR->DrawOpt();

					return;
				} else {
					sBR->QuadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );
				}
			} else if ( Effect == RN_MIRROR ) {
				sBR->QuadsSetSubsetFree( width / w, 0, width / w, height / h, 0, height / h, 0, 0 );
			} else if ( RN_FLIP ) {
				sBR->QuadsSetSubsetFree( 0, height / h, 0, 0, width / w, 0, width / w, height / h );
			} else {
				sBR->QuadsSetSubsetFree( width / w, height / h, width / w, 0, 0, 0, 0, height / h );
			}
		} else {
			if ( Effect == RN_NORMAL ) {
				if ( renderSector )
					sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );
			} else if ( Effect == RN_MIRROR ) {
				if ( renderSector )
					sBR->QuadsSetSubsetFree( Sector.Right / w, Sector.Top / h, Sector.Right / w, Sector.Bottom / h, Sector.Left / w, Sector.Bottom / h, Sector.Left / w, Sector.Top / h );
				else
					sBR->QuadsSetSubsetFree( 1, 0, 1, 1, 0, 1, 0, 0 );
			} else if ( Effect == RN_FLIP ) {
				if ( renderSector )
					sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Bottom / h, Sector.Left / w, Sector.Top / h, Sector.Right / w, Sector.Top / h, Sector.Right / w, Sector.Bottom / h );
				else
					sBR->QuadsSetSubsetFree( 0, 1, 0, 0, 1, 0, 1, 1 );
			} else if ( Effect == RN_FLIPMIRROR ) {
				if ( renderSector )
					sBR->QuadsSetSubsetFree( Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h, Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h );
				else
					sBR->QuadsSetSubsetFree( 1, 1, 1, 0, 0, 0, 0, 1 );
			}
		}

		sBR->BatchQuad( mx, my, iwidth, iheight, Angle );
	} else {
		if ( renderSector )
			sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

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

		sBR->BatchQuadFree( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );
	}

	sBR->DrawOpt();
}

void cTexture::DrawQuad( const eeQuad2f& Q, const eeFloat &offsetx, const eeFloat &offsety, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color, const EE_BLEND_MODE &blend, const eeRecti& texSector) {
	DrawQuadEx( Q, offsetx, offsety, Angle, Scale, Color, Color, Color, Color, blend, texSector);
}

void cTexture::DrawQuadEx( const eeQuad2f& Q, const eeFloat &offsetx, const eeFloat &offsety, const eeFloat &Angle, const eeFloat &Scale, const eeColorA& Color0, const eeColorA& Color1, const eeColorA& Color2, const eeColorA& Color3, const EE_BLEND_MODE &blend, const eeRecti& texSector ) {
	bool renderSector = true;
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
		renderSector = false;

	sBR->SetTexture( this );
	sBR->SetBlendMode( blend );

	sBR->QuadsBegin();
	sBR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

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
				mQ.V[i].x = QCenter.x - eeabs(QCenter.x - mQ.V[i].x) * Scale;
			else
				mQ.V[i].x = QCenter.x + eeabs(QCenter.x - mQ.V[i].x) * Scale;

			if ( mQ.V[i].y < QCenter.y )
				mQ.V[i].y = QCenter.y - eeabs(QCenter.y - mQ.V[i].y) * Scale;
			else
				mQ.V[i].y = QCenter.y + eeabs(QCenter.y - mQ.V[i].y) * Scale;
		}
	}

	if ( Angle != 0.0f )
		mQ.Rotate( Angle, QCenter );

	if ( ClampMode() == EE_CLAMP_REPEAT )
		sBR->QuadsSetSubsetFree( 0, 0, 0, (MaxY - MinY) / h, ( MaxX - MinX ) / w, (MaxY - MinY) / h, ( MaxX - MinX ) / w, 0 );
	else if ( renderSector )
		sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

	sBR->BatchQuadFreeEx( offsetx + mQ[0].x, offsety + mQ[0].y, offsetx + mQ[1].x, offsety + mQ[1].y, offsetx + mQ[2].x, offsety + mQ[2].y, offsetx + mQ[3].x, offsety + mQ[3].y );

	sBR->DrawOpt();
}

}}
