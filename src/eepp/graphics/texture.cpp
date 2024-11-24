#include <SOIL2/src/SOIL2/SOIL2.h>
#include <SOIL2/src/SOIL2/stb_image.h>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/scopedtexture.hpp>
#include <eepp/graphics/stbi_iocb.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/window/engine.hpp>

using namespace EE::Window;
using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

static BatchRenderer* sBR = NULL;

Uint32 Texture::getMaximumSize() {
	static bool checked = false;
	static GLint size = 0;

	if ( !checked ) {
		checked = true;
		glGetIntegerv( GL_MAX_TEXTURE_SIZE, &size );
	}

	return static_cast<Uint32>( size );
}

Texture::Texture() :
	DrawableResource( Drawable::TEXTURE ),
	Image(),
	mFilepath( "" ),
	mTexture( 0 ),
	mImgWidth( 0 ),
	mImgHeight( 0 ),
	mFlags( 0 ),
	mClampMode( ClampMode::ClampToEdge ),
	mFilter( Filter::Linear ),
	mCoordinateType( CoordinateType::Normalized ) {
	if ( NULL == sBR ) {
		sBR = GlobalBatchRenderer::instance();
	}
}

Texture::Texture( const Texture& Copy ) :
	DrawableResource( Drawable::TEXTURE, Copy.mName ),
	Image(),
	mFilepath( Copy.mFilepath ),
	mTexture( Copy.mTexture ),
	mImgWidth( Copy.mImgWidth ),
	mImgHeight( Copy.mImgHeight ),
	mFlags( Copy.mFlags ),
	mClampMode( Copy.mClampMode ),
	mFilter( Copy.mFilter ) {
	mWidth = Copy.mWidth;
	mHeight = Copy.mHeight;
	mChannels = Copy.mChannels;
	mSize = Copy.mSize;

	setPixels( reinterpret_cast<const Uint8*>( &Copy.mPixels[0] ) );
}

Texture::Texture( const Uint32& texture, const unsigned int& width, const unsigned int& height,
				  const unsigned int& imgwidth, const unsigned int& imgheight,
				  const bool& UseMipmap, const unsigned int& Channels, const std::string& filepath,
				  const Texture::ClampMode& ClampMode, const bool& CompressedTexture,
				  const Uint32& MemSize, const Uint8* data ) :
	DrawableResource( Drawable::TEXTURE ) {
	create( texture, width, height, imgwidth, imgheight, UseMipmap, Channels, filepath, ClampMode,
			CompressedTexture, MemSize, data );
}

Texture::~Texture() {
	deleteTexture();

	if ( !TextureFactory::instance()->isErasing() ) {
		TextureFactory::instance()->removeReference( this );
	}
}

void Texture::deleteTexture() {
	if ( mTexture ) {
		unsigned int Texture = static_cast<unsigned int>( mTexture );
		bool threaded =
			Engine::instance()->isSharedGLContextEnabled() && !Engine::instance()->isMainThread();

		if ( threaded )
			Engine::instance()->getCurrentWindow()->setGLContextThread();

		glDeleteTextures( 1, &Texture );

		if ( threaded )
			Engine::instance()->getCurrentWindow()->unsetGLContextThread();

		mTexture = 0;
		mFlags = 0;

		clearCache();
	}
}

void Texture::create( const Uint32& texture, const unsigned int& width, const unsigned int& height,
					  const unsigned int& imgwidth, const unsigned int& imgheight,
					  const bool& UseMipmap, const unsigned int& Channels,
					  const std::string& filepath, const Texture::ClampMode& ClampMode,
					  const bool& CompressedTexture, const Uint32& MemSize, const Uint8* data ) {
	mFilepath = filepath;
	setName( mFilepath );
	mTexture = texture;
	mWidth = width;
	mHeight = height;
	mChannels = Channels;
	mImgWidth = imgwidth;
	mImgHeight = imgheight;
	mSize = MemSize;
	mClampMode = ClampMode;
	mFilter = Filter::Linear;

	if ( UseMipmap )
		mFlags |= TEX_FLAG_MIPMAP;

	if ( CompressedTexture )
		mFlags |= TEX_FLAG_COMPRESSED;

	setPixels( data );

	onResourceChange();
}

const Texture::CoordinateType& Texture::getCoordinateType() const {
	return mCoordinateType;
}

void Texture::setCoordinateType( const CoordinateType& coordinateType ) {
	mCoordinateType = coordinateType;
}

Uint8* Texture::iLock( const bool& ForceRGBA, const bool& KeepFormat ) {
	bool threaded =
		Engine::instance()->isSharedGLContextEnabled() && !Engine::instance()->isMainThread();

#ifndef EE_GLES
	if ( !( mFlags & TEX_FLAG_LOCKED ) ) {
		if ( threaded )
			Engine::instance()->getCurrentWindow()->setGLContextThread();

		if ( ForceRGBA )
			mChannels = 4;

		ScopedTexture saver( mTexture );

		Int32 width = 0, height = 0;
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width );
		glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height );

		mWidth = (unsigned int)width;
		mHeight = (unsigned int)height;
		int size = mWidth * mHeight * mChannels;

		if ( KeepFormat && ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT,
									  &mInternalFormat );
			glGetTexLevelParameteriv( GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &size );
		}

		allocate( (unsigned int)size );

		if ( KeepFormat && ( mFlags & TEX_FLAG_COMPRESSED ) ) {
			glGetCompressedTexImage( GL_TEXTURE_2D, 0, reinterpret_cast<Uint8*>( &mPixels[0] ) );
		} else {
			Uint32 Channel = GL_RGBA;

			if ( 3 == mChannels )
				Channel = GL_RGB;
			else if ( 2 == mChannels )
				Channel = GL_LUMINANCE_ALPHA;
			else if ( 1 == mChannels )
				Channel = GL_ALPHA;

			glGetTexImage( GL_TEXTURE_2D, 0, Channel, GL_UNSIGNED_BYTE,
						   reinterpret_cast<Uint8*>( &mPixels[0] ) );
		}

		mFlags |= TEX_FLAG_LOCKED;
	}

	if ( threaded )
		Engine::instance()->getCurrentWindow()->unsetGLContextThread();

	return &mPixels[0];
#else
	if ( !( mFlags & TEX_FLAG_LOCKED ) ) {
		if ( threaded )
			Engine::instance()->getCurrentWindow()->setGLContextThread();

		{
			ScopedTexture scopedTexture( mTexture );

			GLuint frameBuffer = 0;
			GLi->genFramebuffers( 1, &frameBuffer );

			if ( frameBuffer ) {
				allocate( mWidth * mHeight * 4 );

				GLint previousFrameBuffer;
				glGetIntegerv( GL_FRAMEBUFFER_BINDING, &previousFrameBuffer );
				GLi->bindFramebuffer( GL_FRAMEBUFFER, frameBuffer );
				GLi->framebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
										   mTexture, 0 );
				glReadPixels( 0, 0, mWidth, mHeight, GL_RGBA, GL_UNSIGNED_BYTE, &mPixels[0] );
				GLi->deleteFramebuffers( 1, &frameBuffer );
				GLi->bindFramebuffer( GL_FRAMEBUFFER, previousFrameBuffer );

				mFlags |= TEX_FLAG_LOCKED;
			}
		}

		if ( threaded )
			Engine::instance()->getCurrentWindow()->unsetGLContextThread();

		return NULL != mPixels ? &mPixels[0] : NULL;
	}

	return NULL;
#endif
}

Uint8* Texture::lock( const bool& ForceRGBA ) {
	return iLock( ForceRGBA, false );
}

bool Texture::unlock( const bool& KeepData, const bool& Modified ) {
#ifndef EE_GLES
	if ( ( mFlags & TEX_FLAG_LOCKED ) ) {
		Int32 width = mWidth, height = mHeight;
		unsigned int NTexId = 0;

		if ( Modified || ( mFlags & TEX_FLAG_MODIFIED ) ) {
			ScopedTexture saver( mTexture );

			Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
			flags = ( mClampMode == ClampMode::ClampRepeat ) ? ( flags | SOIL_FLAG_TEXTURE_REPEATS )
															 : flags;

			NTexId = SOIL_create_OGL_texture( reinterpret_cast<Uint8*>( &mPixels[0] ), &width,
											  &height, mChannels, mTexture, flags );

			iTextureFilter( mFilter );

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
	if ( ( mFlags & TEX_FLAG_LOCKED ) ) {
		mFlags &= ~TEX_FLAG_LOCKED;
		return true;
	}

	return false;
#endif
}

const Uint8* Texture::getPixelsPtr() {
	if ( !hasLocalCopy() ) {
		lock();
		unlock( true );
	}

	return Image::getPixelsPtr();
}

void Texture::setPixel( const unsigned int& x, const unsigned int& y, const Color& color ) {
	Image::setPixel( x, y, color );

	mFlags |= TEX_FLAG_MODIFIED;
}

void Texture::bind( CoordinateType coordinateType, const Uint32& textureUnit ) {
	TextureFactory::instance()->bind( this, coordinateType, textureUnit );
}

void Texture::bind( const Uint32& textureUnit ) {
	TextureFactory::instance()->bind( this, mCoordinateType, textureUnit );
}

bool Texture::saveToFile( const std::string& filepath, const SaveType& Format ) {
	bool Res = false;

	if ( mTexture ) {
		lock();

		Res = Image::saveToFile( filepath, Format );

		unlock();
	}

	return Res;
}

void Texture::setFilter( const Filter& filter ) {
	if ( mFilter != filter ) {
		iTextureFilter( filter );
	}
}

void Texture::iTextureFilter( const Filter& filter ) {
	if ( mTexture ) {
		mFilter = filter;

		bool threaded =
			Engine::instance()->isSharedGLContextEnabled() && !Engine::instance()->isMainThread();

		if ( threaded )
			Engine::instance()->getCurrentWindow()->setGLContextThread();

		ScopedTexture saver( mTexture );

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
						 ( mFilter == Filter::Linear ) ? GL_LINEAR : GL_NEAREST );

		if ( mFlags & TEX_FLAG_MIPMAP )
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
							 ( mFilter == Filter::Linear ) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST );
		else
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
							 ( mFilter == Filter::Linear ) ? GL_LINEAR : GL_NEAREST );

		if ( threaded )
			Engine::instance()->getCurrentWindow()->unsetGLContextThread();
	}
}

const Texture::Filter& Texture::getFilter() const {
	return mFilter;
}

void Texture::replaceColor( const Color& ColorKey, const Color& NewColor ) {
	lock();

	Image::replaceColor( ColorKey, NewColor );

	unlock( false, true );

	onResourceChange();
}

void Texture::createMaskFromColor( const Color& ColorKey, Uint8 Alpha ) {
	lock( true );

	Image::replaceColor( ColorKey, Color( ColorKey.r, ColorKey.g, ColorKey.b, Alpha ) );

	unlock( false, true );

	onResourceChange();
}

void Texture::fillWithColor( const Color& Color ) {
	lock();

	Image::fillWithColor( Color );

	unlock( false, true );

	onResourceChange();
}

void Texture::resize( const Uint32& newWidth, const Uint32& newHeight, ResamplerFilter filter ) {
	lock();

	Image::resize( newWidth, newHeight, filter );

	unlock( false, true );

	onResourceChange();
}

void Texture::scale( const Float& scale, ResamplerFilter filter ) {
	lock();

	Image::scale( scale, filter );

	unlock( false, true );

	onResourceChange();
}

void Texture::copyImage( Image* image, const Uint32& x, const Uint32& y ) {
	lock();

	Image::copyImage( image, x, y );

	unlock( false, true );

	onResourceChange();
}

void Texture::flip() {
	lock();

	Image::flip();

	unlock( false, true );

	mImgWidth = mWidth;
	mImgHeight = mHeight;
}

bool Texture::hasLocalCopy() {
	return ( mPixels != NULL );
}

void Texture::setClampMode( const Texture::ClampMode& clampmode ) {
	if ( mClampMode != clampmode ) {
		mClampMode = clampmode;
		applyClampMode();
	}
}

void Texture::applyClampMode() {
	if ( mTexture ) {
		ScopedTexture saver( mTexture );

		if ( mClampMode == ClampMode::ClampRepeat ) {
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		} else {
			unsigned int clamp_mode = 0x812F; // GL_CLAMP_TO_EDGE
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp_mode );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp_mode );
		}
	}
}

void Texture::setTextureId( const Uint32& id ) {
	mTexId = id;
}

const Uint32& Texture::getTextureId() const {
	return mTexId;
}

void Texture::reload() {
	if ( hasLocalCopy() ) {
		Int32 width = (Int32)mWidth;
		Int32 height = (Int32)mHeight;

		bool threaded =
			Engine::instance()->isSharedGLContextEnabled() && !Engine::instance()->isMainThread();

		if ( threaded )
			Engine::instance()->getCurrentWindow()->setGLContextThread();

		{
			ScopedTexture saver( mTexture );

			Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
			flags = ( mClampMode == ClampMode::ClampRepeat ) ? ( flags | SOIL_FLAG_TEXTURE_REPEATS )
															 : flags;

			if ( ( mFlags & TEX_FLAG_COMPRESSED ) ) {
				if ( isGrabed() )
					mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8*>( &mPixels[0] ),
														&width, &height, mChannels, mTexture,
														flags | SOIL_FLAG_COMPRESS_TO_DXT );
				else
					glCompressedTexImage2D( mTexture, 0, mInternalFormat, width, height, 0, mSize,
											&mPixels[0] );
			} else {
				mTexture = SOIL_create_OGL_texture( reinterpret_cast<Uint8*>( &mPixels[0] ), &width,
													&height, mChannels, mTexture, flags );

				TextureFactory::instance()->mMemSize -= mSize;

				mSize = mWidth * mHeight * mChannels;

				if ( getMipmap() ) {
					int w = mWidth;
					int h = mHeight;

					while ( w > 2 && h > 2 ) {
						w >>= 1;
						h >>= 1;
						mSize += ( w * h * mChannels );
					}
				}

				TextureFactory::instance()->mMemSize += mSize;
			}

			iTextureFilter( mFilter );
		}

		if ( threaded )
			Engine::instance()->getCurrentWindow()->unsetGLContextThread();
	} else {
		iLock( false, true );
		reload();
		unlock();
	}

	onResourceChange();
}

static unsigned int convertPixelFormatToGLFormat( Image::PixelFormat pf ) {
	switch ( pf ) {
		case Image::PixelFormat::PIXEL_FORMAT_RED:
			return 0x1903;
		case Image::PixelFormat::PIXEL_FORMAT_RG:
			return 0x8227;
		case Image::PixelFormat::PIXEL_FORMAT_RGB:
			return 0x1907;
		case Image::PixelFormat::PIXEL_FORMAT_BGR:
			return 0x80E0;
		case Image::PixelFormat::PIXEL_FORMAT_BGRA:
			return 0x80E1;
		case Image::PixelFormat::PIXEL_FORMAT_RGBA:
			return 0x1908;
		default:
			return 0x1908;
	}
}

void Texture::update( const Uint8* pixels, Uint32 width, Uint32 height, Uint32 x, Uint32 y,
					  PixelFormat pf ) {
	if ( NULL != pixels && mTexture && x + width <= mWidth && y + height <= mHeight ) {
		bool threaded =
			Engine::instance()->isSharedGLContextEnabled() && !Engine::instance()->isMainThread();

		if ( threaded )
			Engine::instance()->getCurrentWindow()->setGLContextThread();

		{
			ScopedTexture saver( mTexture );

			glTexSubImage2D( GL_TEXTURE_2D, 0, x, y, width, height,
							 (unsigned int)convertPixelFormatToGLFormat( pf ), GL_UNSIGNED_BYTE,
							 pixels );

			if ( hasLocalCopy() ) {
				Image image( pixels, width, height, mChannels );

				Image::copyImage( &image, x, y );
			}
		}

		if ( threaded )
			Engine::instance()->getCurrentWindow()->unsetGLContextThread();

		onResourceChange();
	}
}

void Texture::update( const Uint8* pixels ) {
	update( pixels, mWidth, mHeight, 0, 0, channelsToPixelFormat( mChannels ) );
}

void Texture::update( Image* image, Uint32 x, Uint32 y ) {
	update( image->getPixelsPtr(), image->getWidth(), image->getHeight(), x, y,
			channelsToPixelFormat( image->getChannels() ) );
}

void Texture::replace( Image* image ) {
	bool threaded =
		Engine::instance()->isSharedGLContextEnabled() && !Engine::instance()->isMainThread();

	if ( threaded )
		Engine::instance()->getCurrentWindow()->setGLContextThread();

	{
		Uint32 flags = ( mFlags & TEX_FLAG_MIPMAP ) ? SOIL_FLAG_MIPMAPS : 0;
		flags = ( mClampMode == ClampMode::ClampRepeat ) ? ( flags | SOIL_FLAG_TEXTURE_REPEATS )
														 : flags;

		ScopedTexture scopedTexture;

		Int32 width = (Int32)image->getWidth();
		Int32 height = (Int32)image->getHeight();
		mTexture = SOIL_create_OGL_texture( image->getPixelsPtr(), &width, &height,
											image->getChannels(), mTexture, flags );
		mWidth = mImgWidth = width;
		mHeight = mImgHeight = height;
		mChannels = image->getChannels();

		TextureFactory::instance()->mMemSize -= mSize;
		mSize = mWidth * mHeight * mChannels;
		TextureFactory::instance()->mMemSize += mSize;

		if ( hasLocalCopy() ) {
			// Renew the local copy
			allocate( image->getMemSize(), Color( 0, 0, 0, 0 ), false );
			Image::copyImage( image );
		}
	}

	if ( threaded )
		Engine::instance()->getCurrentWindow()->unsetGLContextThread();

	onResourceChange();
}

const String::HashType& Texture::getHashName() const {
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

void Texture::draw( const Float& x, const Float& y, const Float& Angle, const Vector2f& Scale,
					const Color& Color, const BlendMode& Blend, const RenderMode& Effect,
					OriginPoint Center, const Rect& texSector ) {
	drawEx( x, y, 0, 0, Angle, Scale, Color, Color, Color, Color, Blend, Effect, Center,
			texSector );
}

void Texture::drawFast( const Float& x, const Float& y, const Float& Angle, const Vector2f& Scale,
						const Color& Color, const BlendMode& Blend, const Float& width,
						const Float& height ) {
	Float w = 0.f != width ? width : (Float)getImageWidth();
	Float h = 0.f != height ? height : (Float)getImageHeight();

	sBR->setTexture( this );
	sBR->setBlendMode( Blend );

	sBR->quadsBegin();
	sBR->quadsSetColor( Color );

	if ( getClampMode() == ClampMode::ClampRepeat ) {
		Float iw = (Float)getImageWidth();
		Float ih = (Float)getImageHeight();
		sBR->quadsSetTexCoordFree( 0, 0, 0, height / ih, width / iw, height / ih, width / iw, 0 );
	}

	sBR->batchQuadEx( x, y, w, h, Angle, Scale );

	sBR->drawOpt();
}

void Texture::drawEx( Float x, Float y, Float width, Float height, const Float& Angle,
					  const Vector2f& Scale, const Color& Color0, const Color& Color1,
					  const Color& Color2, const Color& Color3, const BlendMode& Blend,
					  const RenderMode& Effect, OriginPoint Center, const Rect& texSector ) {
	bool renderSector = true;
	Rect Sector = texSector;
	Float w = (Float)getImageWidth();
	Float h = (Float)getImageHeight();

	if ( Sector.Right == 0 && Sector.Bottom == 0 ) {
		Sector.Left = 0;
		Sector.Top = 0;
		Sector.Right = w;
		Sector.Bottom = h;
	}

	if ( 0.f == width && 0.f == height ) {
		width = static_cast<Float>( Sector.Right - Sector.Left );
		height = static_cast<Float>( Sector.Bottom - Sector.Top );
	}

	renderSector =
		!( Sector.Left == 0 && Sector.Top == 0 && Sector.Right == w && Sector.Bottom == h );

	sBR->setTexture( this );
	sBR->setBlendMode( Blend );

	sBR->quadsBegin();
	sBR->quadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Effect <= RENDER_FLIPPED_MIRRORED ) {
		if ( getClampMode() == ClampMode::ClampRepeat ) {
			if ( Effect == RENDER_NORMAL ) {
				if ( renderSector ) {
					sBR->quadsSetTexCoordFree(
						Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h,
						Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );

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
						Center.x = x + width * 0.5f;
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
						Float tor = Sector.Left + swn;

						sBR->quadsSetTexCoordFree( Sector.Left / w, Sector.Top / h, Sector.Left / w,
												   Sector.Bottom / h, tor / w, Sector.Bottom / h,
												   tor / w, Sector.Top / h );

						for ( Int32 tmpY = 0; tmpY < tty; tmpY++ ) {
							sBR->batchQuad( x + ttx * sw, y + tmpY * sh, swn, sh );
						}
					}

					if ( (Float)tty != ty ) {
						Float shn = ( Sector.Bottom - Sector.Top ) * ( ty - (Float)tty );
						Float tob = Sector.Top + shn;

						sBR->quadsSetTexCoordFree( Sector.Left / w, Sector.Top / h, Sector.Left / w,
												   tob / h, Sector.Right / w, tob / h,
												   Sector.Right / w, Sector.Top / h );

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
					sBR->quadsSetTexCoordFree( 0, 0, 0, height / h, width / w, height / h,
											   width / w, 0 );
				}
			} else if ( Effect == RENDER_MIRROR ) {
				sBR->quadsSetTexCoordFree( width / w, 0, width / w, height / h, 0, height / h, 0,
										   0 );
			} else if ( Effect == RENDER_FLIPPED ) {
				sBR->quadsSetTexCoordFree( 0, height / h, 0, 0, width / w, 0, width / w,
										   height / h );
			} else {
				sBR->quadsSetTexCoordFree( width / w, height / h, width / w, 0, 0, 0, 0,
										   height / h );
			}
		} else {
			if ( Effect == RENDER_NORMAL ) {
				if ( renderSector )
					sBR->quadsSetTexCoordFree(
						Sector.Left / w, Sector.Top / h, Sector.Left / w, Sector.Bottom / h,
						Sector.Right / w, Sector.Bottom / h, Sector.Right / w, Sector.Top / h );
			} else if ( Effect == RENDER_MIRROR ) {
				if ( renderSector )
					sBR->quadsSetTexCoordFree( Sector.Right / w, Sector.Top / h, Sector.Right / w,
											   Sector.Bottom / h, Sector.Left / w,
											   Sector.Bottom / h, Sector.Left / w, Sector.Top / h );
				else
					sBR->quadsSetTexCoordFree( 1, 0, 1, 1, 0, 1, 0, 0 );
			} else if ( Effect == RENDER_FLIPPED ) {
				if ( renderSector )
					sBR->quadsSetTexCoordFree( Sector.Left / w, Sector.Bottom / h, Sector.Left / w,
											   Sector.Top / h, Sector.Right / w, Sector.Top / h,
											   Sector.Right / w, Sector.Bottom / h );
				else
					sBR->quadsSetTexCoordFree( 0, 1, 0, 0, 1, 0, 1, 1 );
			} else if ( Effect == RENDER_FLIPPED_MIRRORED ) {
				if ( renderSector )
					sBR->quadsSetTexCoordFree( Sector.Right / w, Sector.Bottom / h,
											   Sector.Right / w, Sector.Top / h, Sector.Left / w,
											   Sector.Top / h, Sector.Left / w, Sector.Bottom / h );
				else
					sBR->quadsSetTexCoordFree( 1, 1, 1, 0, 0, 0, 0, 1 );
			}
		}

		sBR->batchQuadEx( x, y, width, height, Angle, Scale, Center );
	} else {
		if ( renderSector )
			sBR->quadsSetTexCoordFree( Sector.Left / w, Sector.Top / h, Sector.Left / w,
									   Sector.Bottom / h, Sector.Right / w, Sector.Bottom / h,
									   Sector.Right / w, Sector.Top / h );

		Rectf TmpR( x, y, x + width, y + height );
		Quad2f Q = Quad2f( Vector2f( TmpR.Left, TmpR.Top ), Vector2f( TmpR.Left, TmpR.Bottom ),
						   Vector2f( TmpR.Right, TmpR.Bottom ), Vector2f( TmpR.Right, TmpR.Top ) );

		if ( Effect == RENDER_ISOMETRIC ) {
			Q.V[0].x += ( TmpR.Right - TmpR.Left );
			Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
			Q.V[3].x += ( TmpR.Right - TmpR.Left );
			Q.V[3].y += ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
		} else if ( Effect == RENDER_ISOMETRIC_VERTICAL ) {
			Q.V[0].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
			Q.V[1].y -= ( ( TmpR.Bottom - TmpR.Top ) * 0.5f );
		} else if ( Effect == RENDER_ISOMETRIC_VERTICAL_NEGATIVE ) {
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

void Texture::drawQuad( const Quad2f& Q, const Vector2f& Offset, const Float& Angle,
						const Vector2f& Scale, const Color& Color, const BlendMode& Blend,
						const Rect& texSector ) {
	drawQuadEx( Q, Offset, Angle, Scale, Color, Color, Color, Color, Blend, texSector );
}

void Texture::drawQuadEx( Quad2f Q, const Vector2f& Offset, const Float& Angle,
						  const Vector2f& Scale, const Color& Color0, const Color& Color1,
						  const Color& Color2, const Color& Color3, const BlendMode& Blend,
						  Rect texSector ) {
	bool renderSector = true;
	Float w = (Float)getImageWidth();
	Float h = (Float)getImageHeight();

	if ( texSector.Right == 0 && texSector.Bottom == 0 ) {
		texSector.Left = 0;
		texSector.Top = 0;
		texSector.Right = getImageWidth();
		texSector.Bottom = getImageHeight();
	}

	renderSector = !( texSector.Left == 0 && texSector.Top == 0 && texSector.Right == w &&
					  texSector.Bottom == h );

	sBR->setTexture( this );
	sBR->setBlendMode( Blend );

	sBR->quadsBegin();
	sBR->quadsSetColorFree( Color0, Color1, Color2, Color3 );

	if ( Angle != 0 || Scale != 1.0f ) {
		Vector2f QCenter( Q.getCenter() );
		Q.rotate( Angle, QCenter );
		Q.scale( Scale, QCenter );
	}

	if ( getClampMode() == ClampMode::ClampRepeat ) {
		sBR->quadsSetTexCoordFree( 0, 0, 0, ( Q.V[0].y - Q.V[0].y ) / h,
								   ( Q.V[0].x - Q.V[0].x ) / w, ( Q.V[0].y - Q.V[0].y ) / h,
								   ( Q.V[0].x - Q.V[0].x ) / w, 0 );
	} else if ( renderSector ) {
		sBR->quadsSetTexCoordFree( texSector.Left / w, texSector.Top / h, texSector.Left / w,
								   texSector.Bottom / h, texSector.Right / w, texSector.Bottom / h,
								   texSector.Right / w, texSector.Top / h );
	}

	Q.move( Offset );

	sBR->batchQuadFreeEx( Q[0].x, Q[0].y, Q[1].x, Q[1].y, Q[2].x, Q[2].y, Q[3].x, Q[3].y );

	sBR->drawOpt();
}

Sizef Texture::getSize() {
	return Sizef( PixelDensity::pxToDp( mImgWidth ), PixelDensity::pxToDp( mImgHeight ) );
}

Sizef Texture::getPixelsSize() {
	return Sizef( mImgWidth, mImgHeight );
}

void Texture::draw() {
	drawFast( mPosition.x, mPosition.y );
}

void Texture::draw( const Vector2f& position ) {
	drawFast( position.x, position.y );
}

void Texture::draw( const Vector2f& position, const Sizef& size ) {
	drawFast( position.x, position.y, 0, Vector2f::One, mColor, BlendMode::Alpha(), size.x,
			  size.y );
}

std::pair<std::vector<Texture*>, int> Texture::loadGif( IOStream& stream ) {
	stbi_io_callbacks callbacks;
	callbacks.read = &IOCb::read;
	callbacks.skip = &IOCb::skip;
	callbacks.eof = &IOCb::eof;
	stream.seek( 0 );
	auto type = stbi_test_from_callbacks( &callbacks, &stream );
	if ( type != STBI_gif )
		return {};
	stream.seek( 0 );
	std::vector<Texture*> gif;
	ScopedBuffer buf( stream.getSize() );
	stream.read( (char*)buf.get(), buf.size() );
	int width, height, frames, comp;
	int* delays = NULL;
	unsigned char* data = stbi_load_gif_from_memory( buf.get(), buf.size(), &delays, &width,
													 &height, &frames, &comp, 0 );

	if ( data == nullptr )
		return {};

	gif.reserve( frames );

	unsigned char* start = data;
	size_t frame_size = width * height * sizeof( unsigned char ) * comp;
	for ( int i = 0; i < frames; ++i ) {
		gif.emplace_back( TextureFactory::instance()->loadFromPixels( (const Uint8*)start, width,
																	  height, comp ) );
		start += frame_size;
	}

	auto delay = delays[0];
	if ( delay == 0 ) {
		for ( int i = 0; i < frames; i++ ) {
			if ( delays[i] != 0 ) {
				delay = delays[i];
				break;
			}
		}
	}
	free( data );
	free( delays );
	return { std::move( gif ), delay ? delay : 100 };
}

}} // namespace EE::Graphics
