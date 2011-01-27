#include "cframebufferfbo.hpp"
#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"
#include "glhelper.hpp"

namespace EE { namespace Graphics {

bool cFrameBufferFBO::IsSupported() {
	return 0 != GLi->IsExtension( EEGL_EXT_framebuffer_object );
}

cFrameBufferFBO::cFrameBufferFBO() :
	cFrameBuffer(),
	mFrameBuffer(0),
	mDepthBuffer(0)
{}

cFrameBufferFBO::cFrameBufferFBO( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) :
	cFrameBuffer(),
	mFrameBuffer(0),
	mDepthBuffer(0)
{
	Create( Width, Height, DepthBuffer );
}

cFrameBufferFBO::~cFrameBufferFBO() {
	if ( !IsSupported() )
		return;

	GLint curFB;
	glGetIntegerv( GL_FRAMEBUFFER_BINDING, &curFB );

	if ( curFB == mFrameBuffer )
		Unbind();

    if ( mDepthBuffer ) {
        GLuint depthBuffer = static_cast<GLuint>( mDepthBuffer );
		glDeleteFramebuffers( 1, &depthBuffer );
    }

    if ( mFrameBuffer ) {
        GLuint frameBuffer = static_cast<GLuint>( mFrameBuffer );
		glDeleteFramebuffers( 1, &frameBuffer );
    }
}

bool cFrameBufferFBO::Create( const Uint32& Width, const Uint32& Height ) {
	return Create( Width, Height, false );
}

bool cFrameBufferFBO::Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) {
	if ( !IsSupported() )
		return false;

	mWidth 			= Width;
	mHeight 		= Height;
	mHasDepthBuffer = DepthBuffer;

	GLuint frameBuffer = 0;

	glGenFramebuffers( 1, &frameBuffer );

	mFrameBuffer = static_cast<unsigned int>( frameBuffer );

	if ( !mFrameBuffer)
		return false;

	glBindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );

	if ( DepthBuffer ) {
		GLuint depth = 0;

		glGenRenderbuffers( 1, &depth );

		mDepthBuffer = static_cast<unsigned int>(depth);

		if ( !mDepthBuffer )
			return false;

		glBindRenderbuffer( GL_RENDERBUFFER, mDepthBuffer );

		glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Width, Height );

		glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mDepthBuffer );
	}

	if ( NULL == mTexture ) {
		Uint32 TexId = cTextureFactory::instance()->CreateEmptyTexture( Width, Height, eeColorA(0,0,0,0) );

		if ( cTextureFactory::instance()->TextureIdExists( TexId ) ) {
			mTexture = 	cTextureFactory::instance()->GetTexture( TexId );
		} else {
			return false;
		}
	}

	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTexture->Handle(), 0 );

	if ( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE ) {
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );

		return false;
	}

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );

	return true;
}

void cFrameBufferFBO::Bind() {
	if ( mFrameBuffer ) {
		glBindFramebuffer( GL_FRAMEBUFFER, mFrameBuffer );
		SetBufferView();
	}
}

void cFrameBufferFBO::Unbind() {
	if ( mFrameBuffer ) {
		RecoverView();
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	}
}

void cFrameBufferFBO::Reload() {
	Create( mWidth, mHeight, mHasDepthBuffer );
}

}}
