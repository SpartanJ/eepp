#include <eepp/graphics/framebufferfbo.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>

namespace EE { namespace Graphics {

bool FrameBufferFBO::IsSupported() {
	return 0 != GLi->IsExtension( EEGL_EXT_framebuffer_object );
}

FrameBufferFBO::FrameBufferFBO( EE::Window::Window * window ) :
	FrameBuffer( window ),
	mFrameBuffer(0),
	mDepthBuffer(0),
	mLastFB(0),
	mLastRB(0)
{}

FrameBufferFBO::FrameBufferFBO( const Uint32& Width, const Uint32& Height, bool DepthBuffer, EE::Window::Window * window ) :
	FrameBuffer( window ),
	mFrameBuffer(0),
	mDepthBuffer(0),
	mLastFB(0),
	mLastRB(0)
{
	Create( Width, Height, DepthBuffer );
}

FrameBufferFBO::~FrameBufferFBO() {
	if ( !IsSupported() )
		return;

	int curFB;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	if ( curFB == mFrameBuffer )
		Unbind();

	if ( mDepthBuffer ) {
		unsigned int depthBuffer = static_cast<unsigned int>( mDepthBuffer );
		glDeleteFramebuffersEXT( 1, &depthBuffer );
	}

	if ( mFrameBuffer ) {
		unsigned int frameBuffer = static_cast<unsigned int>( mFrameBuffer );
		glDeleteFramebuffersEXT( 1, &frameBuffer );
	}
}

bool FrameBufferFBO::Create( const Uint32& Width, const Uint32& Height ) {
	return Create( Width, Height, false );
}

bool FrameBufferFBO::Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) {
	if ( !IsSupported() )
		return false;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mWidth 			= Width;
	mHeight 		= Height;
	mHasDepthBuffer = DepthBuffer;

	unsigned int frameBuffer = 0;

	glGenFramebuffersEXT( 1, &frameBuffer );

	mFrameBuffer = static_cast<Int32>( frameBuffer );

	if ( !mFrameBuffer)
		return false;

	BindFrameBuffer();

	if ( DepthBuffer ) {
		unsigned int depth = 0;

		glGenRenderbuffersEXT( 1, &depth );

		mDepthBuffer = static_cast<unsigned int>(depth);

		if ( !mDepthBuffer )
			return false;

		BindRenderBuffer();

		glRenderbufferStorageEXT( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer );
	}

	if ( NULL == mTexture ) {
		Uint32 TexId = TextureFactory::instance()->CreateEmptyTexture( Width, Height, 4, ColorA(0,0,0,0) );

		if ( TextureFactory::instance()->TextureIdExists( TexId ) ) {
			mTexture = 	TextureFactory::instance()->GetTexture( TexId );
		} else {
			return false;
		}
	}

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture->Handle(), 0 );

	if ( glCheckFramebufferStatusEXT( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
		glBindFramebufferEXT( GL_FRAMEBUFFER, mLastFB );

		return false;
	}

	glBindFramebufferEXT( GL_FRAMEBUFFER, mLastFB );

	return true;
}

void FrameBufferFBO::Bind() {
	if ( mFrameBuffer ) {
		GlobalBatchRenderer::instance()->Draw();

		BindFrameBuffer();
		BindRenderBuffer();

		SetBufferView();
	}
}

void FrameBufferFBO::Unbind() {
	if ( mFrameBuffer ) {
		RecoverView();

		if ( mDepthBuffer ) {
			glBindFramebufferEXT( GL_FRAMEBUFFER, mLastRB );
		}

		glBindFramebufferEXT( GL_FRAMEBUFFER, mLastFB );
	}
}

void FrameBufferFBO::Reload() {
	Create( mWidth, mHeight, mHasDepthBuffer );
}

void FrameBufferFBO::BindFrameBuffer() {
	int curFB;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	mLastFB = (Int32)curFB;

	glBindFramebufferEXT( GL_FRAMEBUFFER, mFrameBuffer );
}

void FrameBufferFBO::BindRenderBuffer() {
	if ( mDepthBuffer ) {
		int curRB;
		glGetIntegerv( GL_RENDERBUFFER_BINDING, &curRB );

		mLastRB = (Int32)curRB;

		glBindRenderbufferEXT( GL_RENDERBUFFER, mDepthBuffer );
	}
}

}}
