#include <eepp/graphics/framebufferfbo.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/textureregion.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Graphics {

bool FrameBufferFBO::isSupported() {
	return GLi && 0 != GLi->isExtension( EEGL_EXT_framebuffer_object );
}

FrameBufferFBO::FrameBufferFBO( EE::Window::Window* window ) :
	FrameBuffer( window ),
	mFrameBuffer( 0 ),
	mColorBuffer( 0 ),
	mDepthBuffer( 0 ),
	mStencilBuffer( 0 ),
	mLastFB( 0 ),
	mLastCB( 0 ),
	mLastDB( 0 ),
	mLastSB( 0 ) {}

FrameBufferFBO::FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool StencilBuffer,
								bool DepthBuffer, bool useColorBuffer, const Uint32& channels,
								EE::Window::Window* window ) :
	FrameBuffer( window ),
	mFrameBuffer( 0 ),
	mColorBuffer( 0 ),
	mDepthBuffer( 0 ),
	mStencilBuffer( 0 ),
	mLastFB( 0 ),
	mLastCB( 0 ),
	mLastDB( 0 ),
	mLastSB( 0 ) {
	create( Width, Height, StencilBuffer, DepthBuffer, useColorBuffer, channels );
}

FrameBufferFBO::~FrameBufferFBO() {
	if ( !isSupported() )
		return;

	int curFB;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	if ( curFB == mFrameBuffer )
		unbind();

	if ( mDepthBuffer ) {
		unsigned int depthBuffer = static_cast<unsigned int>( mDepthBuffer );
		GLi->deleteRenderbuffers( 1, &depthBuffer );
	}

	if ( mStencilBuffer ) {
		unsigned int stencilBuffer = static_cast<unsigned int>( mStencilBuffer );
		GLi->deleteRenderbuffers( 1, &stencilBuffer );
	}

	if ( mColorBuffer ) {
		unsigned int colorBuffer = static_cast<unsigned int>( mColorBuffer );
		GLi->deleteRenderbuffers( 1, &colorBuffer );
	}

	if ( mFrameBuffer ) {
		unsigned int frameBuffer = static_cast<unsigned int>( mFrameBuffer );
		GLi->deleteFramebuffers( 1, &frameBuffer );
	}
}

bool FrameBufferFBO::create( const Uint32& Width, const Uint32& Height ) {
	return create( Width, Height, true, false, false, 4 );
}

bool FrameBufferFBO::create( const Uint32& Width, const Uint32& Height, bool StencilBuffer,
							 bool DepthBuffer, bool useColorBuffer, const Uint32& channels ) {
	if ( !isSupported() )
		return false;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mSize.x = Width;
	mSize.y = Height;
	mHasColorBuffer = useColorBuffer && GLi->version() != GLv_ES1 && GLi->version() != GLv_ES2;
	mHasStencilBuffer = StencilBuffer;
	mHasDepthBuffer = DepthBuffer;
	mChannels = channels;

	unsigned int frameBuffer = 0;

	GLi->genFramebuffers( 1, &frameBuffer );

	mFrameBuffer = static_cast<Int32>( frameBuffer );

	if ( !mFrameBuffer ) {
		Log::error( "FrameBufferFBO::create: Failed to created FrameBuffer Object" );
		return false;
	}

	bindFrameBuffer();

	if ( mHasDepthBuffer ) {
		unsigned int depth = 0;

		GLi->genRenderbuffers( 1, &depth );

		mDepthBuffer = static_cast<unsigned int>( depth );

		if ( !mDepthBuffer ) {
			Log::error( "FrameBufferFBO::create: Failed to created Depth Buffer" );
			return false;
		}

		bindDepthBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
									  mDepthBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastDB );
	}

	if ( mHasStencilBuffer ) {
		GLuint stencil = 0;
		GLi->genRenderbuffers( 1, &stencil );

		mStencilBuffer = static_cast<Uint32>( stencil );

		if ( !mStencilBuffer ) {
			Log::error( "FrameBufferFBO::create: Failed to created Stencil Buffer" );
			return false;
		}

		bindStencilBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
									  mStencilBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastSB );
	}

	if ( mHasColorBuffer ) {
		GLuint color = 0;
		GLi->genRenderbuffers( 1, &color );

		mColorBuffer = static_cast<unsigned int>( color );

		bindColorBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, channels == 3 ? GL_RGB8 : GL_RGBA8, Width,
								  Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
									  mColorBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastCB );

	} else {
		if ( NULL == mTexture ) {
			Uint32 TexId = TextureFactory::instance()->createEmptyTexture( Width, Height, channels,
																		   Color::Transparent );

			if ( TextureFactory::instance()->existsId( TexId ) ) {
				mTexture = TextureFactory::instance()->getTexture( TexId );
			} else {
				Log::error( "FrameBufferFBO::create: failed to create texture" );
				return false;
			}
		}

		GLi->framebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
								   mTexture->getHandle(), 0 );
	}

	Uint32 status = GLi->checkFramebufferStatus( GL_FRAMEBUFFER );
	if ( status != GL_FRAMEBUFFER_COMPLETE ) {
		Log::error( "FrameBufferFBO::create: Failed to attach Frame Buffer. Status: %04X", status );
		GLi->bindFramebuffer( GL_FRAMEBUFFER, mLastFB );
		return false;
	}

	GLi->bindFramebuffer( GL_FRAMEBUFFER, mLastFB );

	return true;
}

void FrameBufferFBO::bind() {
	if ( mFrameBuffer ) {
		GlobalBatchRenderer::instance()->draw();

		bindFrameBuffer();

		setBufferView();
	}
}

void FrameBufferFBO::unbind() {
	if ( mFrameBuffer ) {
		recoverView();

		GLi->bindFramebuffer( GL_FRAMEBUFFER, mLastFB );
	}
}

void FrameBufferFBO::reload() {
	create( mSize.getWidth(), mSize.getHeight(), mHasStencilBuffer, mHasDepthBuffer,
			mHasColorBuffer, mChannels );
}

void FrameBufferFBO::resize( const Uint32& Width, const Uint32& Height ) {
	if ( Sizei( Width, Height ) == mSize )
		return;

	mSize.x = Width;
	mSize.y = Height;

	bindFrameBuffer();

	if ( mHasDepthBuffer ) {
		bindDepthBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
									  mDepthBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastDB );
	}

	if ( mHasStencilBuffer ) {
		bindStencilBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
									  mStencilBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastSB );
	}

	if ( NULL != mTexture ) {
		Image newImage( Width, Height, mChannels );
		mTexture->replace( &newImage );
	} else if ( mColorBuffer ) {
		bindColorBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, mChannels == 3 ? GL_RGB8 : GL_RGBA8, Width,
								  Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
									  mColorBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastCB );
	}

	GLi->bindFramebuffer( GL_FRAMEBUFFER, mLastFB );
}

void FrameBufferFBO::draw( const Vector2f& position, const Sizef& size ) {
	if ( NULL != mTexture ) {
		mTexture->draw( position, size );
	} else if ( mColorBuffer ) {
		GLi->bindFramebuffer( GL_READ_FRAMEBUFFER, mFrameBuffer );
		GLi->bindFramebuffer( GL_DRAW_FRAMEBUFFER, mLastFB );

		GLi->blitFrameBuffer( 0, 0, (Int32)mSize.getWidth(), (Int32)mSize.getHeight(), position.x,
							  position.y, position.x + size.x, position.y + size.y,
							  GL_COLOR_BUFFER_BIT, GL_LINEAR );
	}
}

void FrameBufferFBO::draw( Rect src, Rect dst ) {
	if ( NULL != mTexture ) {
		TextureRegion textureRegion( getTexture()->getTextureId(), src );
		Sizei size( dst.getSize() );
		textureRegion.setDestSize( Sizef( size.x, size.y ) );
		textureRegion.draw( dst.Left, dst.Top, Color::White );
	} else if ( mColorBuffer ) {
		GLi->bindFramebuffer( GL_READ_FRAMEBUFFER, mFrameBuffer );
		GLi->bindFramebuffer( GL_DRAW_FRAMEBUFFER, mLastFB );

		GLi->blitFrameBuffer( src.Left, 0 == mLastFB ? src.Bottom : src.Top, src.Right,
							  0 == mLastFB ? src.Top : src.Bottom, dst.Left, dst.Top, dst.Right,
							  dst.Bottom, GL_COLOR_BUFFER_BIT, GL_LINEAR );

		GLi->bindFramebuffer( GL_READ_FRAMEBUFFER, mLastFB );
	}
}

bool FrameBufferFBO::created() const {
	return mFrameBuffer != 0;
}

const Int32& FrameBufferFBO::getFrameBufferId() const {
	return mFrameBuffer;
}

void FrameBufferFBO::bindFrameBuffer() {
	int curFB;

	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	mLastFB = (Int32)curFB;

	GLi->bindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );

	if ( !mDepthBuffer && !mStencilBuffer ) {
		const GLenum discards[] = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
		GLi->discardFramebuffer( GL_FRAMEBUFFER, 2, discards );
	} else if ( !mDepthBuffer ) {
		const GLenum discards[] = {GL_DEPTH_ATTACHMENT};
		GLi->discardFramebuffer( GL_FRAMEBUFFER, 1, discards );
	} else if ( !mStencilBuffer ) {
		const GLenum discards[] = {GL_STENCIL_ATTACHMENT};
		GLi->discardFramebuffer( GL_FRAMEBUFFER, 1, discards );
	}
}

void FrameBufferFBO::bindDepthBuffer() {
	if ( mDepthBuffer ) {
		int curDB;
		glGetIntegerv( GL_RENDERBUFFER_BINDING, &curDB );

		mLastDB = (Int32)curDB;

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mDepthBuffer );
	}
}

void FrameBufferFBO::bindStencilBuffer() {
	if ( mStencilBuffer ) {
		int curSB;
		glGetIntegerv( GL_RENDERBUFFER_BINDING, &curSB );

		mLastSB = (Int32)curSB;

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mStencilBuffer );
	}
}

void FrameBufferFBO::bindColorBuffer() {
	if ( mColorBuffer ) {
		int curCB;
		glGetIntegerv( GL_RENDERBUFFER_BINDING, &curCB );

		mLastCB = (Int32)curCB;

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mColorBuffer );
	}
}

}} // namespace EE::Graphics
