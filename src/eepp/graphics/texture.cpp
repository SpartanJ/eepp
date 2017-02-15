#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/graphics/texturesaver.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

static BatchRenderer * sBR = NULL;

Texture::Texture() :
	Image(),
	mFilepath(""),
	mId(0),
	mTexture(0),
	mImgWidth(0),
	mImgHeight(0),
	mFlags(0),
	mClampMode( CLAMP_TO_EDGE ),
	mFilter( TEX_FILTER_LINEAR )
{
	if ( NULL == sBR ) {
		sBR = GlobalBatchRenderer::instance();
	}
}

Texture::Texture( const Texture& Copy ) :
	Image(),
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

Texture::~Texture() {
	DeleteTexture();

	if ( !TextureFactory::instance()->IsErasing() ) {
		TextureFactory::instance()->RemoveReference( this );
	}
}

void Texture::DeleteTexture() {
	if ( mTexture ) {
		unsigned int Texture = static_cast<unsigned int>(mTexture);
		glDeleteTextures(1, &Texture);

		mTexture = 0;
		mFlags = 0;

		ClearCache();
	}
}

Texture::Texture( const Uint32& texture, const unsigned int& width, const unsigned int& height, const unsigned int& imgwidth, const unsigned int& imgheight, const bool& UseMipmap, const unsigned int& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) {
	Create( texture, width, height, imgwidth, imgheight, UseMipmap, Channels, filepath, ClampMode, CompressedTexture, MemSize, data );
}

void Texture::Create( const Uint32& texture, const unsigned int& width, const unsigned int& height, const unsigned int& imgwidth, const unsigned int& imgheight, const bool& UseMipmap, const unsigned int& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) {
	mFilepath 	= filepath;
	mId 		= String::hash( mFilepath );
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

Uint8 * Texture::iLock( const bool& ForceRGBA, const bool& KeepFormat ) {
	#ifndef EE_GLES
	if ( !( mFlags & TEX_FLAG_LOCKED ) ) {
		if ( ForceRGBA )
			mChannels = 4;

		TextureSaver saver( mTexture );

		Int32 width = 0, height = 0;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

		mWidth = (unsigned int)width;
		mHeight = (unsigned int)height;
		int size = mWidth * mHeight * mChannels;

		if ( KeepFormat && ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &mInternalFormat );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size );
		}

		Allocate( (unsigned int)size );

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

		mFlags |= TEX_FLAG_LOCKED;
	}

	return &mPixels[0];
	#else
	return NULL;
	#endif
}

Uint8 * Texture::Lock( const bool& ForceRGBA ) {
	return iLock( ForceRGBA, false );
}

bool Texture::Unlock( const bool& KeepData, const bool& Modified ) {
	#ifndef EE_GLES
	if ( ( mFlags & TEX_FLAG_LOCKED ) ) {
		Int32 width = mWidth, height = mHeight;
		unsigned int NTexId = 0;

		if ( Modified || ( mFlags & TEX_FLAG_MODIFIED ) )	{
			TextureSaver saver( mTexture );

			Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
			flags = (mClampMode == CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

			NTexId = SOIL_create_OGL_texture( reinterpret_cast<Uint8*>(&mPixels[0]), &width, &height, mChannels, mTexture, flags );

			iTextureFilter(mFilter);

			mFlags &= ~TEX_FLAG_MODIFIED;

			if ( mFlags & TEX_FLAG_COMPRESSED )
				mFlags &= ~TEX_FLAG_COMPRESSED;
		}

		if ( !KeepData )
			ClearCache();

		mFlags &= ~TEX_FLAG_LOCKED;

		if ( (int)NTexId == mTexture || !Modified )
			return true;
	}

	return false;
	#else
	return false;
	#endif
}

const Uint8 * Texture::GetPixelsPtr() {
	if ( !LocalCopy() ) {
		Lock();
		Unlock(true);
	}

	return Image::GetPixelsPtr();
}

void Texture::SetPixel( const unsigned int& x, const unsigned int& y, const ColorA& Color ) {
	Image::SetPixel( x, y, Color );

	mFlags |= TEX_FLAG_MODIFIED;
}

void Texture::Bind() {
	TextureFactory::instance()->Bind( this );
}

bool Texture::SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format ) {
	bool Res = false;

	if ( mTexture ) {
		Lock();

		Res = Image::SaveToFile( filepath, Format );

		Unlock();
	}

	return Res;
}

void Texture::Filter(const EE_TEX_FILTER& filter) {
	if ( mFilter != filter ) {
		iTextureFilter( filter );
	}
}

void Texture::iTextureFilter( const EE_TEX_FILTER& filter ) {
	if (mTexture) {
		mFilter = filter;

		TextureSaver saver( mTexture );

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, (mFilter == TEX_FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);

		if ( mFlags & TEX_FLAG_MIPMAP )
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_FILTER_LINEAR) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
		else
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, (mFilter == TEX_FILTER_LINEAR) ? GL_LINEAR : GL_NEAREST);
	}
}

const EE_TEX_FILTER& Texture::Filter() const {
	return mFilter;
}

void Texture::ReplaceColor( const ColorA& ColorKey, const ColorA& NewColor ) {
	Lock();

	Image::ReplaceColor( ColorKey, NewColor );

	Unlock( false, true );
}

void Texture::CreateMaskFromColor( const ColorA& ColorKey, Uint8 Alpha ) {
	Lock( true );

	Image::ReplaceColor( ColorKey, ColorA( ColorKey.r(), ColorKey.g(), ColorKey.b(), Alpha ) );

	Unlock( false, true );
}

void Texture::FillWithColor( const ColorA& Color ) {
	Lock();

	Image::FillWithColor( Color );

	Unlock( false, true );
}

void Texture::Resize( const Uint32& newWidth, const Uint32& newHeight , EE_RESAMPLER_FILTER filter ) {
	Lock();

	Image::Resize( newWidth, newHeight, filter );

	Unlock( false, true );
}

void Texture::Scale( const Float& scale, EE_RESAMPLER_FILTER filter ) {
	Lock();

	Image::Scale( scale, filter );

	Unlock( false, true );
}

void Texture::CopyImage(Image * image, const Uint32& x, const Uint32& y ) {
	Lock();

	Image::CopyImage( image, x, y );

	Unlock( false, true );
}

void Texture::Flip() {
	Lock();

	Image::Flip();

	Unlock( false, true );

	mImgWidth 	= mWidth;
	mImgHeight 	= mHeight;
}

bool Texture::LocalCopy() {
	return ( mPixels != NULL );
}

void Texture::ClampMode( const EE_CLAMP_MODE& clampmode ) {
	if ( mClampMode != clampmode ) {
		mClampMode = clampmode;
		ApplyClampMode();
	}
}

void Texture::ApplyClampMode() {
	if (mTexture) {
		TextureSaver saver( mTexture );

		if( mClampMode == CLAMP_REPEAT ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		} else {
			unsigned int clamp_mode = 0x812F; // GL_CLAMP_TO_EDGE
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_mode );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_mode );
		}
	}
}

void Texture::Id( const Uint32& id ) {
	mTexId = id;
}

const Uint32& Texture::Id() const {
	return mTexId;
}

void Texture::Reload()  {
	if ( LocalCopy() ) {
		Int32 width = (Int32)mWidth;
		Int32 height = (Int32)mHeight;

		TextureSaver saver( mTexture );

		Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
		flags = (mClampMode == CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

		if ( ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			if ( Grabed() )
				mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags | SOIL_FLAG_COMPRESS_TO_DXT );
			else
				glCompressedTexImage2D( mTexture, 0, mInternalFormat, width, height, 0, mSize, &mPixels[0] );
		} else {
			mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags );

			TextureFactory::instance()->mMemSize -= mSize;

			mSize = mWidth * mHeight * mChannels;

			if ( Mipmap() ) {
				int w = mWidth;
				int h = mHeight;

				while( w > 2 && h > 2 ) {
					w>>=1;
					h>>=1;
					mSize += ( w * h * mChannels );
				}
			}

			TextureFactory::instance()->mMemSize += mSize;
		}

		iTextureFilter( mFilter );
	} else {
		iLock(false,true);
		Reload();
		Unlock();
	}
}

void Texture::Update( const Uint8* pixels, Uint32 width, Uint32 height, Uint32 x, Uint32 y, EE_PIXEL_FORMAT pf ) {
	if ( NULL != pixels && mTexture && x + width <= mWidth && y + height <= mHeight ) {
		TextureSaver saver( mTexture );

		glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, (unsigned int)pf, GL_UNSIGNED_BYTE, pixels );
	}
}

void Texture::Update( const Uint8* pixels ) {
	Update( pixels, mWidth, mHeight, 0, 0, ChannelsToPixelFormat( mChannels ) );
}

void Texture::Update( Image *image, Uint32 x, Uint32 y ) {
	Update( image->GetPixelsPtr(), image->Width(), image->Height(), x, y, ChannelsToPixelFormat( image->Channels() ) );
}

const Uint32& Texture::HashName() const {
	return mId;
}

void Texture::Mipmap( const bool& UseMipmap ) {
	if ( mFlags & TEX_FLAG_MIPMAP ) {
		if ( !UseMipmap )
			mFlags &= ~TEX_FLAG_MIPMAP;
	} else {
		if ( UseMipmap )
			mFlags |= TEX_FLAG_MIPMAP;
	}
}

bool Texture::Mipmap() const {
	return mFlags & TEX_FLAG_MIPMAP;
}

void Texture::Grabed( const bool& isGrabed ) {
	if ( mFlags & TEX_FLAG_GRABED ) {
		if ( !isGrabed )
			mFlags &= ~TEX_FLAG_GRABED;
	} else {
		if ( isGrabed )
			mFlags |= TEX_FLAG_GRABED;
	}
}

bool Texture::Grabed() const {
	return 0 != ( mFlags & TEX_FLAG_GRABED );
}

bool Texture::IsCompressed() const {
	return 0 != ( mFlags & TEX_FLAG_COMPRESSED );
}

void Texture::Draw( const Float &x, const Float &y, const Float &Angle, const Vector2f &Scale, const ColorA& Color, const EE_BLEND_MODE &Blend, const EE_RENDER_MODE &Effect, OriginPoint Center, const Recti& texSector) {
	DrawEx( x, y, 0, 0, Angle, Scale, Color, Color, Color, Color, Blend, Effect, Center, texSector );
}

void Texture::DrawFast( const Float& x, const Float& y, const Float& Angle, const Vector2f& Scale, const ColorA& Color, const EE_BLEND_MODE &Blend, const Float &width, const Float &height ) {
	Float w = 0.f != width	? width		: (Float)ImgWidth();
	Float h = 0.f != height	? height	: (Float)ImgHeight();

	sBR->SetTexture( this );
	sBR->SetBlendMode( Blend );

	sBR->QuadsBegin();
	sBR->QuadsSetColor( Color );

	if ( ClampMode() == CLAMP_REPEAT ) {
		sBR->QuadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );
	}

	sBR->BatchQuadEx( x, y, w, h, Angle, Scale );

	sBR->DrawOpt();
}

void Texture::DrawEx( Float x, Float y, Float width, Float height, const Float &Angle, const Vector2f &Scale, const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const EE_BLEND_MODE &Blend, const EE_RENDER_MODE &Effect, OriginPoint Center, const Recti& texSector ) {
	bool renderSector	= true;
	Recti Sector		= texSector;
	Float w			= (Float)ImgWidth();
	Float h			= (Float)ImgHeight();

	if ( Sector.Right == 0 && Sector.Bottom == 0 ) {
		Sector.Left		= 0;
		Sector.Top		= 0;
		Sector.Right	= w;
		Sector.Bottom	= h;
	}

	if ( 0.f == width && 0.f == height ) {
		width	= static_cast<Float> (Sector.Right - Sector.Left);
		height	= static_cast<Float> (Sector.Bottom - Sector.Top);
	}

	renderSector = !( Sector.Left == 0 && Sector.Top == 0 && Sector.Right == w && Sector.Bottom == h );

	sBR->SetTexture( this );
	sBR->SetBlendMode( Blend );

	sBR->QuadsBegin();
	sBR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Effect <= RN_FLIPMIRROR ) {
		if ( ClampMode() == CLAMP_REPEAT ) {
			if ( Effect == RN_NORMAL ) {
				if ( renderSector ) {
					sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

					Float sw = (Float)( Sector.Right - Sector.Left );
					Float sh = (Float)( Sector.Bottom - Sector.Top );
					Float tx = width / sw;
					Float ty = height / sh;
					Int32 ttx = (Int32)tx;
					Int32 tty = (Int32)ty;
					Int32 tmpY;
					Int32 tmpX;

					sBR->Draw();
					Vector2f oCenter( sBR->BatchCenter() );
					Float oAngle = sBR->BatchRotation();
					Vector2f oScale = sBR->BatchScale();

					if ( Center.OriginType == OriginPoint::OriginCenter ) {
						Center.x = x + width  * 0.5f;
						Center.y = y + height * 0.5f;
					} else if ( Center.OriginType == OriginPoint::OriginTopLeft ) {
						Center.x = x;
						Center.y = y;
					} else {
						Center.x += x;
						Center.y += y;
					}

					sBR->BatchCenter( Center );
					sBR->BatchRotation( Angle );
					sBR->BatchScale( Scale );

					for ( tmpY = 0; tmpY < tty; tmpY++ ) {
						for ( tmpX = 0; tmpX < ttx; tmpX++ ) {
							sBR->BatchQuad( x + tmpX * sw, y + tmpY * sh, sw, sh );
						}
					}

					if ( (Float)ttx != tx ) {
						Float swn = ( Sector.Right - Sector.Left ) * ( tx - (Float)ttx );
						Float tor = Sector.Left + swn ;

						sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, tor / w, Sector.Bottom / h, tor / w, Sector.Top / h );

						for ( Int32 tmpY = 0; tmpY < tty; tmpY++ ) {
							sBR->BatchQuad( x + ttx * sw, y + tmpY * sh, swn, sh );
						}
					}

					if ( (Float)tty != ty ) {
						Float shn = ( Sector.Bottom - Sector.Top ) * ( ty - (Float)tty );
						Float tob = Sector.Top + shn;

						sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, tob / h, Sector.Right / w, tob / h, Sector.Right / w, Sector.Top / h );

						for ( Int32 tmpX = 0; tmpX < ttx; tmpX++ ) {
							sBR->BatchQuad( x + tmpX * sw, y + tty * sh, sw, shn );
						}
					}

					sBR->Draw();
					sBR->BatchCenter( oCenter );
					sBR->BatchRotation( oAngle );
					sBR->BatchScale( oScale );

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

		sBR->BatchQuadEx( x, y, width, height, Angle, Scale, Center );
	} else {
		if ( renderSector )
			sBR->QuadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

		Rectf TmpR( x, y, x + width, y + height );
		Quad2f Q = Quad2f( Vector2f( TmpR.Left, TmpR.Top ), Vector2f( TmpR.Left, TmpR.Bottom ), Vector2f( TmpR.Right, TmpR.Bottom ), Vector2f( TmpR.Right, TmpR.Top ) );

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

		if ( Angle != 0.f || Scale != 1.f ) {
			if ( Center.OriginType == OriginPoint::OriginCenter ) {
				Center = TmpR.center();
			} else if ( Center.OriginType == OriginPoint::OriginTopLeft ) {
				Center = TmpR.pos();
			} else {
				Center += TmpR.pos();
			}

			Q.rotate( Angle, Center );
			Q.scale( Scale, Center );
		}

		sBR->BatchQuadFree( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );
	}

	sBR->DrawOpt();
}

void Texture::DrawQuad( const Quad2f& Q, const Vector2f& Offset, const Float &Angle, const Vector2f &Scale, const ColorA& Color, const EE_BLEND_MODE &Blend, const Recti& texSector) {
	DrawQuadEx( Q, Offset, Angle, Scale, Color, Color, Color, Color, Blend, texSector );
}

void Texture::DrawQuadEx( Quad2f Q, const Vector2f& Offset, const Float &Angle, const Vector2f &Scale, const ColorA& Color0, const ColorA& Color1, const ColorA& Color2, const ColorA& Color3, const EE_BLEND_MODE &Blend, Recti texSector ) {
	bool renderSector = true;
	Float w =	(Float)ImgWidth();
	Float h = (Float)ImgHeight();

	if ( texSector.Right == 0 && texSector.Bottom == 0 ) {
		texSector.Left		= 0;
		texSector.Top		= 0;
		texSector.Right		= ImgWidth();
		texSector.Bottom	= ImgHeight();
	}

	renderSector = !( texSector.Left == 0 && texSector.Top == 0 && texSector.Right == w && texSector.Bottom == h );

	sBR->SetTexture( this );
	sBR->SetBlendMode( Blend );

	sBR->QuadsBegin();
	sBR->QuadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Angle != 0 ||  Scale != 1.0f ) {
		Vector2f QCenter( Q.getCenter() );
		Q.rotate( Angle, QCenter );
		Q.scale( Scale, QCenter );
	}

	if ( ClampMode() == CLAMP_REPEAT ) {
		sBR->QuadsSetSubsetFree( 0, 0, 0, ( Q.V[0].y - Q.V[0].y ) / h, ( Q.V[0].x - Q.V[0].x ) / w, ( Q.V[0].y - Q.V[0].y ) / h, ( Q.V[0].x - Q.V[0].x ) / w, 0 );
	} else if ( renderSector ) {
		sBR->QuadsSetSubsetFree( texSector.Left / w, texSector.Top / h, texSector.Left / w, texSector.Bottom / h, texSector.Right / w, texSector.Bottom / h, texSector.Right / w, texSector.Top / h );
	}

	Q.move( Offset );

	sBR->BatchQuadFreeEx( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );

	sBR->DrawOpt();
}

}}
