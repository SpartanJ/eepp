#include <eepp/graphics/cframebufferpbuffer.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/window/cengine.hpp>

#ifdef EE_GLEW_AVAILABLE

#include <eepp/graphics/glextensions.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/helper/glew/wglew.h>
#elif defined( EE_X11_PLATFORM )
#include <eepp/helper/glew/glxew.h>
#include <X11/Xlib.h>
#else
#warning No PBuffer implemented on this platform
#endif

#endif

#include <eepp/graphics/renderer/crenderergl3.hpp>
#include <eepp/graphics/renderer/crenderergles2.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>

namespace EE { namespace Graphics {

cFrameBufferPBuffer::cFrameBufferPBuffer( Window::cWindow * window )
#if EE_PLATFORM == EE_PLATFORM_WIN
	: cFrameBuffer( window ),
	mDeviceContext( NULL ),
	mPBuffer( NULL ),
	mContext( NULL )
#elif defined( EE_X11_PLATFORM )
	: cFrameBuffer( window ),
	mDisplay( NULL ),
	mPBuffer( 0 ),
	mContext( NULL )
#else
	: cFrameBuffer( window )
#endif
{
#if defined( EE_GLEW_AVAILABLE ) && defined( EE_X11_PLATFORM )
	mDisplay = XOpenDisplay(NULL);
#endif
}

cFrameBufferPBuffer::cFrameBufferPBuffer( const Uint32& Width, const Uint32& Height, bool DepthBuffer, Window::cWindow * window )
#if EE_PLATFORM == EE_PLATFORM_WIN
	: cFrameBuffer( window ),
	mDeviceContext( NULL ),
	mPBuffer( NULL ),
	mContext( NULL )
#elif defined( EE_X11_PLATFORM )
	: cFrameBuffer( window ),
	mDisplay( NULL ),
	mPBuffer( 0 ),
	mContext( NULL )
#else
	: cFrameBuffer( window )
#endif
{
#if defined( EE_GLEW_AVAILABLE ) && defined( EE_X11_PLATFORM )
	mDisplay = XOpenDisplay(NULL);
#endif
	Create( Width, Height, DepthBuffer );
}

cFrameBufferPBuffer::~cFrameBufferPBuffer() {
#ifdef EE_GLEW_AVAILABLE

#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( mContext )
		wglDeleteContext( (HGLRC)mContext );

	if ( mPBuffer && mDeviceContext ) {
		wglReleasePbufferDCARB( (HPBUFFERARB)mPBuffer, (HDC)mDeviceContext );
		wglDestroyPbufferARB( (HPBUFFERARB)mPBuffer );
	}
#elif defined( EE_X11_PLATFORM )
    if ( mContext )
		glXDestroyContext( (Display*)mDisplay, (GLXContext)mContext );

    if ( mPBuffer )
		glXDestroyGLXPbufferSGIX( (Display*)mDisplay, mPBuffer );

	if ( mDisplay )
		XCloseDisplay( (Display*)mDisplay );
#endif

#endif
	if ( Window::cEngine::instance()->ExistsWindow( mWindow ) ) {
		mWindow->SetDefaultContext();
	}
}

bool cFrameBufferPBuffer::IsSupported() {
#ifdef EE_GLEW_AVAILABLE

#if EE_PLATFORM == EE_PLATFORM_WIN
	return WGLEW_ARB_pbuffer && WGLEW_ARB_pixel_format;
#elif defined( EE_X11_PLATFORM )
	return glxewIsSupported("GLX_SGIX_pbuffer");
#else
	return false;
#endif

#else
	return false;
#endif
}

bool cFrameBufferPBuffer::Create( const Uint32& Width, const Uint32& Height ) {
	return Create( Width, Height, false );
}

bool cFrameBufferPBuffer::Create( const Uint32& Width, const Uint32& Height, bool DepthBuffer ) {
	if ( !IsSupported() )
		return false;

	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	mWidth 			= Width;
	mHeight 		= Height;
	mHasDepthBuffer = DepthBuffer;

#ifdef EE_GLEW_AVAILABLE

#if EE_PLATFORM == EE_PLATFORM_WIN
	HDC currentDC = wglGetCurrentDC();

	int attributes[] =
	{
		WGL_SUPPORT_OPENGL_ARB,  GL_TRUE,
		WGL_DRAW_TO_PBUFFER_ARB, GL_TRUE,
		WGL_RED_BITS_ARB,        8,
		WGL_GREEN_BITS_ARB,      8,
		WGL_BLUE_BITS_ARB,       8,
		WGL_ALPHA_BITS_ARB,      8,
		WGL_DEPTH_BITS_ARB,      ( DepthBuffer ? 24 : 0 ),
		WGL_DOUBLE_BUFFER_ARB,   GL_FALSE,
		0
	};

	unsigned int nbFormats = 0;
	int pixelFormat = -1;
	wglChoosePixelFormatARB( currentDC, attributes, NULL, 1, &pixelFormat, &nbFormats );

	if ( nbFormats == 0 )
		return false;

	mPBuffer       = wglCreatePbufferARB( currentDC, pixelFormat, Width, Height, NULL );
	mDeviceContext = wglGetPbufferDCARB( (HPBUFFERARB)mPBuffer );
	mContext       = wglCreateContext( (HDC)mDeviceContext );

	if ( !mPBuffer || !mDeviceContext || !mContext )
		return false;

	int actualWidth, actualHeight;
	wglQueryPbufferARB( (HPBUFFERARB)mPBuffer, WGL_PBUFFER_WIDTH_ARB, &actualWidth );
	wglQueryPbufferARB( (HPBUFFERARB)mPBuffer, WGL_PBUFFER_HEIGHT_ARB, &actualHeight );

	if ( ( actualWidth != static_cast<int>(Width) ) || ( actualHeight != static_cast<int>(Height) ) )
		return false;

	HGLRC currentContext = wglGetCurrentContext();
	if (currentContext) {
		wglMakeCurrent( NULL, NULL );
		wglShareLists( currentContext, (HGLRC)mContext );
		wglMakeCurrent( currentDC, currentContext );
	}
#elif defined( EE_X11_PLATFORM )
	int visualAttributes[] =
	{
		GLX_RENDER_TYPE,   GLX_RGBA_BIT,
		GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
		GLX_RED_SIZE,      8,
		GLX_GREEN_SIZE,    8,
		GLX_BLUE_SIZE,     8,
		GLX_ALPHA_SIZE,    8,
		GLX_DEPTH_SIZE,    ( DepthBuffer ? 24 : 0 ),
		0
	};

	int PBufferAttributes[] =
	{
		GLX_PBUFFER_WIDTH, 	(int)Width,
		GLX_PBUFFER_HEIGHT, (int)Height,
		0
	};

	int nbConfigs = 0;
	GLXFBConfig* configs = glXChooseFBConfigSGIX( (Display*)mDisplay, DefaultScreen( (Display*)mDisplay ), visualAttributes, &nbConfigs );

	if (!configs || !nbConfigs)
		return false;

	mPBuffer = glXCreateGLXPbufferSGIX( (Display*)mDisplay, configs[0], Width, Height, PBufferAttributes );

	if ( !mPBuffer ) {
		XFree(configs);
		return false;
	}

	unsigned int actualWidth, actualHeight;
	glXQueryGLXPbufferSGIX( (Display*)mDisplay, mPBuffer, GLX_WIDTH_SGIX, &actualWidth);
	glXQueryGLXPbufferSGIX( (Display*)mDisplay, mPBuffer, GLX_HEIGHT_SGIX, &actualHeight);

	if ( ( actualWidth != Width ) || ( actualHeight != Height ) ) {
		XFree(configs);
		return false;
	}

	GLXDrawable currentDrawable = glXGetCurrentDrawable();
	GLXContext currentContext = glXGetCurrentContext();

	if ( currentContext )
		glXMakeCurrent( (Display*)mDisplay, 0, NULL );

	XVisualInfo* visual = glXGetVisualFromFBConfig( (Display*)mDisplay, configs[0] );
	mContext = glXCreateContext( (Display*)mDisplay, visual, currentContext, true );

	if ( !mContext ) {
		XFree(configs);
		XFree(visual);
		return false;
	}

	if ( currentContext )
		glXMakeCurrent( (Display*)mDisplay, currentDrawable, currentContext );

	XFree(configs);
	XFree(visual);
#endif

#endif

	if ( NULL == mTexture ) {
		Uint32 TexId = cTextureFactory::instance()->CreateEmptyTexture( Width, Height, 4, eeColorA(0,0,0,0) );

		if ( cTextureFactory::instance()->TextureIdExists( TexId ) ) {
			mTexture = 	cTextureFactory::instance()->GetTexture( TexId );
		} else {
			return false;
		}
	}

	return true;
}

void cFrameBufferPBuffer::Bind() {
	bool ChangeContext = false;

	cGlobalBatchRenderer::instance()->Draw();

#ifdef EE_GLEW_AVAILABLE

	#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( mDeviceContext && mContext ) {
		if ( wglGetCurrentContext() != mContext ) {
			ChangeContext = true;
			wglMakeCurrent( (HDC)mDeviceContext, (HGLRC)mContext );
		}
	}
	#elif defined( EE_X11_PLATFORM )
	if ( mPBuffer && mContext ) {
		if ( glXGetCurrentContext() != mContext ) {
			ChangeContext = true;
			glXMakeCurrent( (Display*)mDisplay, mPBuffer, (GLXContext)mContext );
		}
	}
	#endif

#endif

	if ( ChangeContext ) {
		#ifdef EE_GL3_ENABLED
		if ( GLv_3 == GLi->Version() ) {
			GLi->GetRendererGL3()->ReloadCurrentShader();
		}

		if ( GLv_ES2 == GLi->Version() ) {
			GLi->GetRendererGLES2()->ReloadCurrentShader();
		}
		#endif

		mWindow->Setup2D( true );

		SetBufferView();
	}
}

void cFrameBufferPBuffer::Unbind() {
	RecoverView();

	int previousTexture;
	glGetIntegerv( GL_TEXTURE_BINDING_2D, &previousTexture );

	GLi->BindTexture( GL_TEXTURE_2D, (int)mTexture->Handle() );

	glCopyTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, 0, 0, mWidth, mHeight );

	GLi->BindTexture( GL_TEXTURE_2D, previousTexture );

	mWindow->SetDefaultContext();
}


void cFrameBufferPBuffer::Reload() {
	Create( mWidth, mHeight, mHasDepthBuffer );
}

}}
