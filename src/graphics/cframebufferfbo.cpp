#include "cframebufferfbo.hpp"
#include "ctexturefactory.hpp"
#include "../window/cengine.hpp"
#include "glhelper.hpp"

using namespace EE::Graphics::Private;

namespace EE { namespace Graphics {

bool cFrameBufferFBO::IsSupported() {
	return 0 != cGL::instance()->IsExtension( EEGL_EXT_framebuffer_object );
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
	glGetIntegerv( GL_FRAMEBUFFER_BINDING_EXT, &curFB );

	if ( curFB == mFrameBuffer )
		Unbind();

    if ( mDepthBuffer ) {
        GLuint depthBuffer = static_cast<GLuint>( mDepthBuffer );
        glDeleteFramebuffersEXT( 1, &depthBuffer );
    }

    if ( mFrameBuffer ) {
        GLuint frameBuffer = static_cast<GLuint>( mFrameBuffer );
        glDeleteFramebuffersEXT( 1, &frameBuffer );
    }
}

bool cFrameBufferFBO::Create( const Uint32& Width, const Uint32& Height ) {
	return Create( Width, Height, false );
}

bool cFrameBufferFBO::Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) {
	if ( !IsSupported() )
		return false;

	GLuint frameBuffer = 0;

	glGenFramebuffersEXT( 1, &frameBuffer );

	mFrameBuffer = static_cast<unsigned int>( frameBuffer );

	if ( !mFrameBuffer)
		return false;

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, mFrameBuffer );

	if ( DepthBuffer ) {
		GLuint depth = 0;

		glGenRenderbuffersEXT( 1, &depth );

		mDepthBuffer = static_cast<unsigned int>(depth);

		if ( !mDepthBuffer )
			return false;

		glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, mDepthBuffer );

		glRenderbufferStorageEXT( GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT, Width, Height );

		glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, mDepthBuffer );
	}

	Uint32 TexId = cTextureFactory::instance()->CreateEmptyTexture( Width, Height, eeColorA(0,0,0,0) );

	if ( cTextureFactory::instance()->TextureIdExists( TexId ) ) {
		mTexture = 	cTextureFactory::instance()->GetTexture( TexId );
	} else {
		return false;
	}

	glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, mTexture->Handle(), 0 );

	if ( glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) != GL_FRAMEBUFFER_COMPLETE_EXT ) {
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

		return false;
	}

	glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );

	mWidth 	= Width;
	mHeight = Height;

	return true;
}

void cFrameBufferFBO::Bind() {
	if ( mFrameBuffer ) {
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, mFrameBuffer );
		SetBufferView();
	}
}

void cFrameBufferFBO::Unbind() {
	if ( mFrameBuffer ) {
		glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		RecoverView();
	}
}

}}
