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

FrameBufferFBO::FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool DepthBuffer, EE::Window::Window * window ) :
	FrameBuffer( window ),
	mFrameBuffer(0),
	mDepthBuffer(0),
	mStencilBuffer(0),
	mLastFB(0),
	mLastDB(0),
	mLastSB(0)
{
	create( Width, Height, StencilBuffer, DepthBuffer );
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
	return create( Width, Height, true, false );
}

bool FrameBufferFBO::create(const Uint32& Width, const Uint32& Height, bool StencilBuffer, bool DepthBuffer ) {
	if ( !isSupported() )
		return false;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mWidth 			= Width;
	mHeight 		= Height;
	mHasStencilBuffer = StencilBuffer;
	mHasDepthBuffer = DepthBuffer;

	unsigned int frameBuffer = 0;

	GLi->genFramebuffers( 1, &frameBuffer );

	mFrameBuffer = static_cast<Int32>( frameBuffer );

	if ( !mFrameBuffer) {
		eePRINT("FrameBufferFBO::create: Failed to created FrameBuffer Object");

		return false;
	}

	bindFrameBuffer();

	if ( DepthBuffer ) {
		unsigned int depth = 0;

		GLi->genRenderbuffers( 1, &depth );

		mDepthBuffer = static_cast<unsigned int>(depth);

		if ( !mDepthBuffer )
			return false;

		bindDepthBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer );

		GLi->bindFramebuffer( GL_RENDERBUFFER, mLastDB );
	}

	if ( StencilBuffer ) {
		GLuint stencil = 0;
		GLi->genRenderbuffers( 1, &stencil );

		mStencilBuffer = static_cast<Uint32>(stencil);

		if (!mStencilBuffer) {
			eePRINT("FrameBufferFBO::create: Failed to created Stencil Buffer");

			return false;
		}

		bindStencilBuffer();

		GLi->renderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX, Width, Height );

		GLi->framebufferRenderbuffer( GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER_EXT, mStencilBuffer );

		GLi->bindFramebuffer( GL_RENDERBUFFER, mLastSB );
	}

	if ( NULL == mTexture ) {
		Uint32 TexId = TextureFactory::instance()->createEmptyTexture( Width, Height, 4, Color::Transparent );

		if ( TextureFactory::instance()->existsId( TexId ) ) {
			mTexture = 	TextureFactory::instance()->getTexture( TexId );
		} else {
			return false;
		}
	}

	GLi->framebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture->getHandle(), 0 );

	if ( GLi->checkFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
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
	create( mWidth, mHeight, mHasStencilBuffer, mHasDepthBuffer );
}

void FrameBufferFBO::bindFrameBuffer() {
	int curFB;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	mLastFB = (Int32)curFB;

	GLi->bindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );
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
