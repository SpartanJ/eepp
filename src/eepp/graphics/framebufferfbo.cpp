#include <eepp/graphics/framebufferfbo.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/renderer/openglext.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>

namespace EE { namespace Graphics {

bool FrameBufferFBO::isSupported() {
	return 0 != GLi->isExtension( EEGL_EXT_framebuffer_object );
}

FrameBufferFBO::FrameBufferFBO( EE::Window::Window * window ) :
	FrameBuffer( window ),
	mFrameBuffer(0),
	mDepthBuffer(0),
	mStencilBuffer(0),
	mLastFB(0),
	mLastDB(0),
	mLastSB(0)
{}

FrameBufferFBO::FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool DepthBuffer, const Uint32& channels, EE::Window::Window * window ) :
	FrameBuffer( window ),
	mFrameBuffer(0),
	mDepthBuffer(0),
	mStencilBuffer(0),
	mLastFB(0),
	mLastDB(0),
	mLastSB(0)
{
	create( Width, Height, StencilBuffer, DepthBuffer, channels );
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

	if ( mFrameBuffer ) {
		unsigned int frameBuffer = static_cast<unsigned int>( mFrameBuffer );
		GLi->deleteFramebuffers( 1, &frameBuffer );
	}
}

bool FrameBufferFBO::create( const Uint32& Width, const Uint32& Height ) {
	return create( Width, Height, true, false, 4 );
}

bool FrameBufferFBO::create( const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool DepthBuffer, const Uint32& channels ) {
	if ( !isSupported() )
		return false;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mSize.x = Width;
	mSize.y = Height;
	mHasStencilBuffer = StencilBuffer;
	mHasDepthBuffer = DepthBuffer;
	mChannels = channels;

	unsigned int frameBuffer = 0;

	GLi->genFramebuffers( 1, &frameBuffer );

	mFrameBuffer = static_cast<Int32>( frameBuffer );

	if ( !mFrameBuffer ) {
		eePRINT("FrameBufferFBO::create: Failed to created FrameBuffer Object");
		return false;
	}

	bindFrameBuffer();

	if ( mHasDepthBuffer ) {
		unsigned int depth = 0;

		GLi->genRenderbuffers( 1, &depth );

		mDepthBuffer = static_cast<unsigned int>(depth);

		if ( !mDepthBuffer ) {
			eePRINT("FrameBufferFBO::create: Failed to created Depth Buffer");
			return false;
		}

		bindDepthBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastDB );
	}

	if ( mHasStencilBuffer ) {
		GLuint stencil = 0;
		GLi->genRenderbuffers( 1, &stencil );

		mStencilBuffer = static_cast<Uint32>(stencil);

		if (!mStencilBuffer) {
			eePRINT("FrameBufferFBO::create: Failed to created Stencil Buffer");
			return false;
		}

		bindStencilBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, mStencilBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastSB );
	}

	if ( NULL == mTexture ) {
		Uint32 TexId = TextureFactory::instance()->createEmptyTexture( Width, Height, channels, Color::Transparent );

		if ( TextureFactory::instance()->existsId( TexId ) ) {
			mTexture = 	TextureFactory::instance()->getTexture( TexId );
		} else {
			eePRINTL( "FrameBufferFBO::create: failed to create texture" );
			return false;
		}
	}

	GLi->framebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture->getHandle(), 0 );

	Uint32 status = GLi->checkFramebufferStatus( GL_FRAMEBUFFER );
	if ( status != GL_FRAMEBUFFER_COMPLETE ) {
		eePRINTL("FrameBufferFBO::create: Failed to attach Frame Buffer. Status: %04X", status );
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
	create( mSize.getWidth(), mSize.getHeight(), mHasStencilBuffer, mHasDepthBuffer, mChannels );
}

void FrameBufferFBO::resize( const Uint32& Width, const Uint32& Height ) {
	mSize.x = Width;
	mSize.y = Height;

	bindFrameBuffer();

	if ( mHasDepthBuffer ) {
		bindDepthBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastDB );
	}

	if ( mHasStencilBuffer ) {
		bindStencilBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, mStencilBuffer );

		GLi->bindRenderbuffer( GL_RENDERBUFFER, mLastSB );
	}

	if ( NULL != mTexture ) {
		Image newImage( Width, Height, mChannels );
		mTexture->replace( &newImage );
	}

	GLi->bindFramebuffer( GL_FRAMEBUFFER, mLastFB );
}

const Int32 &FrameBufferFBO::getFrameBufferId() const {
	return mFrameBuffer;
}

void FrameBufferFBO::bindFrameBuffer() {
	int curFB;

	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	mLastFB = (Int32)curFB;

	GLi->bindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );

	if ( !mDepthBuffer && !mStencilBuffer ) {
		const GLenum discards[]  = {GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT};
		GLi->discardFramebuffer( GL_FRAMEBUFFER, 2, discards );
	} else if ( !mDepthBuffer ) {
		const GLenum discards[]  = {GL_DEPTH_ATTACHMENT};
		GLi->discardFramebuffer( GL_FRAMEBUFFER, 1, discards );
	} else if ( !mStencilBuffer ) {
		const GLenum discards[]  = {GL_STENCIL_ATTACHMENT};
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

}}
