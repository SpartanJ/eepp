#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/graphics/texturesaver.hpp>
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

static BatchRenderer * sBR = NULL;

Uint32 Texture::getMaximumSize() {
	static bool checked = false;
	static GLint size = 0;

	if (!checked) {
		checked = true;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
	}

	return static_cast<Uint32>(size);
}

Texture::Texture() :
	Image(),
	Drawable( DRAWABLE_TEXTURE ),
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
	Drawable( DRAWABLE_TEXTURE ),
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

	setPixels( reinterpret_cast<const Uint8*>( &Copy.mPixels[0] ) );
}

Texture::Texture( const Uint32& texture, const unsigned int& width, const unsigned int& height, const unsigned int& imgwidth, const unsigned int& imgheight, const bool& UseMipmap, const unsigned int& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) :
	Drawable( DRAWABLE_TEXTURE )
{
	create( texture, width, height, imgwidth, imgheight, UseMipmap, Channels, filepath, ClampMode, CompressedTexture, MemSize, data );
}

Texture::~Texture() {
	deleteTexture();

	if ( !TextureFactory::instance()->isErasing() ) {
		TextureFactory::instance()->removeReference( this );
	}
}

void Texture::deleteTexture() {
	if ( mTexture ) {
		unsigned int Texture = static_cast<unsigned int>(mTexture);
		glDeleteTextures(1, &Texture);

		mTexture = 0;
		mFlags = 0;

		clearCache();
	}
}

void Texture::create( const Uint32& texture, const unsigned int& width, const unsigned int& height, const unsigned int& imgwidth, const unsigned int& imgheight, const bool& UseMipmap, const unsigned int& Channels, const std::string& filepath, const EE_CLAMP_MODE& ClampMode, const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) {
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

	setPixels( data );
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

		allocate( (unsigned int)size );

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
	if ( !( mFlags & TEX_FLAG_LOCKED ) ) {
		TextureSaver saver( mTexture );

		GLuint frameBuffer = 0;
		glGenFramebuffersEXT(1, &frameBuffer);

		if ( frameBuffer ) {
			allocate( mWidth * mHeight * mChannels );

			GLint previousFrameBuffer;
			glGetIntegerv( GL_FRAMEBUFFER_BINDING, &previousFrameBuffer );
			glBindFramebufferEXT( GL_FRAMEBUFFER, frameBuffer );
			glFramebufferTexture2DEXT( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture, 0 );
			glReadPixels( 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, &mPixels[0] );
			glDeleteFramebuffersEXT(1, &frameBuffer);
			glBindFramebufferEXT( GL_FRAMEBUFFER, previousFrameBuffer );

			mFlags |= TEX_FLAG_LOCKED;

			return &mPixels[0];
		}
	}

	return NULL;
	#endif
}

Uint8 * Texture::lock( const bool& ForceRGBA ) {
	return iLock( ForceRGBA, false );
}

bool Texture::unlock( const bool& KeepData, const bool& Modified ) {
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
			clearCache();

		mFlags &= ~TEX_FLAG_LOCKED;

		if ( (int)NTexId == mTexture || !Modified )
			return true;
	}

	return false;
	#else
	return false;
	#endif
}

const Uint8 * Texture::getPixelsPtr() {
	if ( !hasLocalCopy() ) {
		lock();
		unlock(true);
	}

	return Image::getPixelsPtr();
}

void Texture::setPixel( const unsigned int& x, const unsigned int& y, const Color& Color ) {
	Image::setPixel( x, y, Color );

	mFlags |= TEX_FLAG_MODIFIED;
}

void Texture::bind() {
	TextureFactory::instance()->bind( this );
}

bool Texture::saveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format ) {
	bool Res = false;

	if ( mTexture ) {
		lock();

		Res = Image::saveToFile( filepath, Format );

		unlock();
	}

	return Res;
}

void Texture::setFilter(const EE_TEX_FILTER& filter) {
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

const EE_TEX_FILTER& Texture::getFilter() const {
	return mFilter;
}

void Texture::replaceColor( const Color& ColorKey, const Color& NewColor ) {
	lock();

	Image::replaceColor( ColorKey, NewColor );

	unlock( false, true );
}

void Texture::createMaskFromColor( const Color& ColorKey, Uint8 Alpha ) {
	lock( true );

	Image::replaceColor( ColorKey, Color( ColorKey.r, ColorKey.g, ColorKey.b, Alpha ) );

	unlock( false, true );
}

void Texture::fillWithColor( const Color& Color ) {
	lock();

	Image::fillWithColor( Color );

	unlock( false, true );
}

void Texture::resize( const Uint32& newWidth, const Uint32& newHeight , EE_RESAMPLER_FILTER filter ) {
	lock();

	Image::resize( newWidth, newHeight, filter );

	unlock( false, true );
}

void Texture::scale( const Float& scale, EE_RESAMPLER_FILTER filter ) {
	lock();

	Image::scale( scale, filter );

	unlock( false, true );
}

void Texture::copyImage(Image * image, const Uint32& x, const Uint32& y ) {
	lock();

	Image::copyImage( image, x, y );

	unlock( false, true );
}

void Texture::flip() {
	lock();

	Image::flip();

	unlock( false, true );

	mImgWidth 	= mWidth;
	mImgHeight 	= mHeight;
}

bool Texture::hasLocalCopy() {
	return ( mPixels != NULL );
}

void Texture::setClampMode( const EE_CLAMP_MODE& clampmode ) {
	if ( mClampMode != clampmode ) {
		mClampMode = clampmode;
		applyClampMode();
	}
}

void Texture::applyClampMode() {
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

void Texture::setId( const Uint32& id ) {
	mTexId = id;
}

const Uint32& Texture::getId() const {
	return mTexId;
}

void Texture::reload()  {
	if ( hasLocalCopy() ) {
		Int32 width = (Int32)mWidth;
		Int32 height = (Int32)mHeight;

		TextureSaver saver( mTexture );

		Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
		flags = (mClampMode == CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

		if ( ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			if ( isGrabed() )
				mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags | SOIL_FLAG_COMPRESS_TO_DXT );
			else
				glCompressedTexImage2D( mTexture, 0, mInternalFormat, width, height, 0, mSize, &mPixels[0] );
		} else {
			mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8 *> ( &mPixels[0] ), &width, &height, mChannels, mTexture, flags );

			TextureFactory::instance()->mMemSize -= mSize;

			mSize = mWidth * mHeight * mChannels;

			if ( getMipmap() ) {
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
		reload();
		unlock();
	}
}

void Texture::update( const Uint8* pixels, Uint32 width, Uint32 height, Uint32 x, Uint32 y, EE_PIXEL_FORMAT pf ) {
	if ( NULL != pixels && mTexture && x + width <= mWidth && y + height <= mHeight ) {
		TextureSaver saver( mTexture );

		glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height, (unsigned int)pf, GL_UNSIGNED_BYTE, pixels );

		if ( hasLocalCopy() ) {
			Image image( pixels, width, height, mChannels );

			Image::copyImage( &image, x, y );
		}
	}
}

void Texture::update( const Uint8* pixels ) {
	update( pixels, mWidth, mHeight, 0, 0, channelsToPixelFormat( mChannels ) );
}

void Texture::update( Image *image, Uint32 x, Uint32 y ) {
	update( image->getPixelsPtr(), image->getWidth(), image->getHeight(), x, y, channelsToPixelFormat( image->getChannels() ) );
}

void Texture::replace( Image * image ) {
	Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
	flags = (mClampMode == CLAMP_REPEAT) ? (flags | SOIL_FLAG_TEXTURE_REPEATS) : flags;

	Int32 width = (Int32)image->getWidth();
	Int32 height = (Int32)image->getHeight();
	mTexture = SOIL_create_OGL_texture( image->getPixelsPtr(), &width, &height, image->getChannels(), mTexture, flags );
	mWidth = mImgWidth = width;
	mHeight = mImgHeight = height;
	mChannels = image->getChannels();

	TextureFactory::instance()->mMemSize -= mSize;
	mSize = mWidth * mHeight * mChannels;
	TextureFactory::instance()->mMemSize += mSize;

	if ( hasLocalCopy() ) {
		// Renew the local copy
		allocate( image->getMemSize(), Color(0,0,0,0), false );
		Image::copyImage( image );
	}
}

const Uint32& Texture::getHashName() const {
	return mId;
}

void Texture::setMipmap( const bool& UseMipmap ) {
	if ( mFlags & TEX_FLAG_MIPMAP ) {
		if ( !UseMipmap )
			mFlags &= ~TEX_FLAG_MIPMAP;
	} else {
		if ( UseMipmap )
			mFlags |= TEX_FLAG_MIPMAP;
	}
}

bool Texture::getMipmap() const {
	return mFlags & TEX_FLAG_MIPMAP;
}

void Texture::setGrabed( const bool& isGrabed ) {
	if ( mFlags & TEX_FLAG_GRABED ) {
		if ( !isGrabed )
			mFlags &= ~TEX_FLAG_GRABED;
	} else {
		if ( isGrabed )
			mFlags |= TEX_FLAG_GRABED;
	}
}

bool Texture::isGrabed() const {
	return 0 != ( mFlags & TEX_FLAG_GRABED );
}

bool Texture::isCompressed() const {
	return 0 != ( mFlags & TEX_FLAG_COMPRESSED );
}

void Texture::draw( const Float &x, const Float &y, const Float &Angle, const Vector2f &Scale, const Color& Color, const EE_BLEND_MODE &Blend, const EE_RENDER_MODE &Effect, OriginPoint Center, const Recti& texSector) {
	drawEx( x, y, 0, 0, Angle, Scale, Color, Color, Color, Color, Blend, Effect, Center, texSector );
}

void Texture::drawFast( const Float& x, const Float& y, const Float& Angle, const Vector2f& Scale, const Color& Color, const EE_BLEND_MODE &Blend, const Float &width, const Float &height ) {
	Float w = 0.f != width	? width		: (Float)getImageWidth();
	Float h = 0.f != height	? height	: (Float)getImageHeight();

	sBR->setTexture( this );
	sBR->setBlendMode( Blend );

	sBR->quadsBegin();
	sBR->quadsSetColor( Color );

	if ( getClampMode() == CLAMP_REPEAT ) {
		sBR->quadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );
	}

	sBR->batchQuadEx( x, y, w, h, Angle, Scale );

	sBR->drawOpt();
}

void Texture::drawEx( Float x, Float y, Float width, Float height, const Float &Angle, const Vector2f &Scale, const Color& Color0, const Color& Color1, const Color& Color2, const Color& Color3, const EE_BLEND_MODE &Blend, const EE_RENDER_MODE &Effect, OriginPoint Center, const Recti& texSector ) {
	bool renderSector	= true;
	Recti Sector		= texSector;
	Float w			= (Float)getImageWidth();
	Float h			= (Float)getImageHeight();

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

	sBR->setTexture( this );
	sBR->setBlendMode( Blend );

	sBR->quadsBegin();
	sBR->quadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Effect <= RN_FLIPMIRROR ) {
		if ( getClampMode() == CLAMP_REPEAT ) {
			if ( Effect == RN_NORMAL ) {
				if ( renderSector ) {
					sBR->quadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

					Float sw = (Float)( Sector.Right - Sector.Left );
					Float sh = (Float)( Sector.Bottom - Sector.Top );
					Float tx = width / sw;
					Float ty = height / sh;
					Int32 ttx = (Int32)tx;
					Int32 tty = (Int32)ty;
					Int32 tmpY;
					Int32 tmpX;

					sBR->draw();
					Vector2f oCenter( sBR->getBatchCenter() );
					Float oAngle = sBR->getBatchRotation();
					Vector2f oScale = sBR->getBatchScale();

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

					sBR->setBatchCenter( Center );
					sBR->setBatchRotation( Angle );
					sBR->setBatchScale( Scale );

					for ( tmpY = 0; tmpY < tty; tmpY++ ) {
						for ( tmpX = 0; tmpX < ttx; tmpX++ ) {
							sBR->batchQuad( x + tmpX * sw, y + tmpY * sh, sw, sh );
						}
					}

					if ( (Float)ttx != tx ) {
						Float swn = ( Sector.Right - Sector.Left ) * ( tx - (Float)ttx );
						Float tor = Sector.Left + swn ;

						sBR->quadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, tor / w, Sector.Bottom / h, tor / w, Sector.Top / h );

						for ( Int32 tmpY = 0; tmpY < tty; tmpY++ ) {
							sBR->batchQuad( x + ttx * sw, y + tmpY * sh, swn, sh );
						}
					}

					if ( (Float)tty != ty ) {
						Float shn = ( Sector.Bottom - Sector.Top ) * ( ty - (Float)tty );
						Float tob = Sector.Top + shn;

						sBR->quadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, tob / h, Sector.Right / w, tob / h, Sector.Right / w, Sector.Top / h );

						for ( Int32 tmpX = 0; tmpX < ttx; tmpX++ ) {
							sBR->batchQuad( x + tmpX * sw, y + tty * sh, sw, shn );
						}
					}

					sBR->draw();
					sBR->setBatchCenter( oCenter );
					sBR->setBatchRotation( oAngle );
					sBR->setBatchScale( oScale );

					return;
				} else {
					sBR->quadsSetSubsetFree( 0, 0, 0, height / h, width / w, height / h, width / w, 0 );
				}
			} else if ( Effect == RN_MIRROR ) {
				sBR->quadsSetSubsetFree( width / w, 0, width / w, height / h, 0, height / h, 0, 0 );
			} else if ( RN_FLIP ) {
				sBR->quadsSetSubsetFree( 0, height / h, 0, 0, width / w, 0, width / w, height / h );
			} else {
				sBR->quadsSetSubsetFree( width / w, height / h, width / w, 0, 0, 0, 0, height / h );
			}
		} else {
			if ( Effect == RN_NORMAL ) {
				if ( renderSector )
					sBR->quadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );
			} else if ( Effect == RN_MIRROR ) {
				if ( renderSector )
					sBR->quadsSetSubsetFree( Sector.Right / w, Sector.Top / h, Sector.Right / w, Sector.Bottom / h, Sector.Left / w, Sector.Bottom / h, Sector.Left / w, Sector.Top / h );
				else
					sBR->quadsSetSubsetFree( 1, 0, 1, 1, 0, 1, 0, 0 );
			} else if ( Effect == RN_FLIP ) {
				if ( renderSector )
					sBR->quadsSetSubsetFree( Sector.Left / w, Sector.Bottom / h, Sector.Left / w, Sector.Top / h, Sector.Right / w, Sector.Top / h, Sector.Right / w, Sector.Bottom / h );
				else
					sBR->quadsSetSubsetFree( 0, 1, 0, 0, 1, 0, 1, 1 );
			} else if ( Effect == RN_FLIPMIRROR ) {
				if ( renderSector )
					sBR->quadsSetSubsetFree( Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h, Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h );
				else
					sBR->quadsSetSubsetFree( 1, 1, 1, 0, 0, 0, 0, 1 );
			}
		}

		sBR->batchQuadEx( x, y, width, height, Angle, Scale, Center );
	} else {
		if ( renderSector )
			sBR->quadsSetSubsetFree( Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

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
				Center = TmpR.getCenter();
			} else if ( Center.OriginType == OriginPoint::OriginTopLeft ) {
				Center = TmpR.getPosition();
			} else {
				Center += TmpR.getPosition();
			}

			Q.rotate( Angle, Center );
			Q.scale( Scale, Center );
		}

		sBR->batchQuadFree( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );
	}

	sBR->drawOpt();
}

void Texture::drawQuad( const Quad2f& Q, const Vector2f& Offset, const Float &Angle, const Vector2f &Scale, const Color& Color, const EE_BLEND_MODE &Blend, const Recti& texSector) {
	drawQuadEx( Q, Offset, Angle, Scale, Color, Color, Color, Color, Blend, texSector );
}

void Texture::drawQuadEx( Quad2f Q, const Vector2f& Offset, const Float &Angle, const Vector2f &Scale, const Color& Color0, const Color& Color1, const Color& Color2, const Color& Color3, const EE_BLEND_MODE &Blend, Recti texSector ) {
	bool renderSector = true;
	Float w =	(Float)getImageWidth();
	Float h = (Float)getImageHeight();

	if ( texSector.Right == 0 && texSector.Bottom == 0 ) {
		texSector.Left		= 0;
		texSector.Top		= 0;
		texSector.Right		= getImageWidth();
		texSector.Bottom	= getImageHeight();
	}

	renderSector = !( texSector.Left == 0 && texSector.Top == 0 && texSector.Right == w && texSector.Bottom == h );

	sBR->setTexture( this );
	sBR->setBlendMode( Blend );

	sBR->quadsBegin();
	sBR->quadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Angle != 0 ||  Scale != 1.0f ) {
		Vector2f QCenter( Q.getCenter() );
		Q.rotate( Angle, QCenter );
		Q.scale( Scale, QCenter );
	}

	if ( getClampMode() == CLAMP_REPEAT ) {
		sBR->quadsSetSubsetFree( 0, 0, 0, ( Q.V[0].y - Q.V[0].y ) / h, ( Q.V[0].x - Q.V[0].x ) / w, ( Q.V[0].y - Q.V[0].y ) / h, ( Q.V[0].x - Q.V[0].x ) / w, 0 );
	} else if ( renderSector ) {
		sBR->quadsSetSubsetFree( texSector.Left / w, texSector.Top / h, texSector.Left / w, texSector.Bottom / h, texSector.Right / w, texSector.Bottom / h, texSector.Right / w, texSector.Top / h );
	}

	Q.move( Offset );

	sBR->batchQuadFreeEx( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );

	sBR->drawOpt();
}

}}

Sizef EE::Graphics::Texture::getSize() {
	return Sizef( PixelDensity::pxToDp( mImgWidth ), PixelDensity::pxToDp( mImgHeight ) );
}

Sizei EE::Graphics::Texture::getPixelSize() {
	return Sizei( mImgWidth, mImgHeight );
}

void EE::Graphics::Texture::draw() {
	drawFast( mPosition.x, mPosition.y );
}

void EE::Graphics::Texture::draw( const Vector2f & position ) {
	drawFast( position.x, position.y );
}

void EE::Graphics::Texture::draw(const Vector2f & position, const Sizef & size) {
	drawFast( position.x, position.y, 0, Vector2f::One, mColor, ALPHA_NORMAL, size.x, size.y );
}
