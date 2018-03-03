#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <SOIL2/src/SOIL2/stb_image.h>
#include <eepp/window/engine.hpp>
#include <eepp/window/platform/platformimpl.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>
#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/SDL2/inputsdl2.hpp>
#include <eepp/window/backend/SDL2/cursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/wminfo.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/renderer/renderer.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS || EE_PLATFORM == EE_PLATFORM_ANDROID
#define SDL2_THREADED_GLCONTEXT
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

WindowSDL::WindowSDL( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardSDL, ( this ) ), eeNew( InputSDL, ( this ) ), eeNew( CursorManagerSDL, ( this ) ) ),
	mSDLWindow( NULL ),
	mGLContext( NULL ),
	mGLContextThread( NULL )
#ifdef EE_USE_WMINFO
	,
	mWMinfo( NULL )
#endif
{
	create( Settings, Context );
}

WindowSDL::~WindowSDL() {
	if ( NULL != mGLContext ) {
		SDL_GL_DeleteContext( mGLContext );
	}

	if ( NULL != mGLContextThread ) {
		SDL_GL_DeleteContext( mGLContextThread );
	}

	if ( NULL != mSDLWindow ) {
		SDL_DestroyWindow( mSDLWindow );
	}

#ifdef EE_USE_WMINFO
	eeSAFE_DELETE( mWMinfo );
#endif
}

bool WindowSDL::create( WindowSettings Settings, ContextSettings Context ) {
	if ( mWindow.Created )
		return false;

	mWindow.WindowConfig	= Settings;
	mWindow.ContextConfig	= Context;

	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
		eePRINTL( "Unable to initialize SDL: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	SDL_DisplayMode dpm;
	SDL_GetDesktopDisplayMode( 0, &dpm );

	mWindow.DesktopResolution = Sizei( dpm.w, dpm.h );

	#if EE_PLATFORM == EE_PLATFORM_ANDROID
		mWindow.WindowConfig.Style = WindowStyle::Fullscreen | WindowStyle::UseDesktopResolution;
	#endif

	if ( mWindow.WindowConfig.Style & WindowStyle::UseDesktopResolution ) {
		mWindow.WindowConfig.Width	= mWindow.DesktopResolution.getWidth();
		mWindow.WindowConfig.Height	= mWindow.DesktopResolution.getHeight();
	}

	mWindow.Flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	#if SDL_VERSION_ATLEAST(2,0,1)
	mWindow.Flags |= SDL_WINDOW_ALLOW_HIGHDPI;
	#endif
	#endif

	if ( mWindow.WindowConfig.Style & WindowStyle::Resize ) {
		mWindow.Flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( mWindow.WindowConfig.Style & WindowStyle::Borderless ) {
		mWindow.Flags |= SDL_WINDOW_BORDERLESS;
	}

	setGLConfig();

	Uint32 mTmpFlags = mWindow.Flags;

	if ( mWindow.WindowConfig.Style & WindowStyle::Fullscreen ) {
		mTmpFlags |= SDL_WINDOW_FULLSCREEN;
	}

	if ( mWindow.ContextConfig.Multisamples > 0 ) {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, mWindow.ContextConfig.Multisamples);
	}

	mSDLWindow = SDL_CreateWindow( mWindow.WindowConfig.Caption.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, mTmpFlags );

	if ( NULL == mSDLWindow ) {
		eePRINTL( "Unable to create window: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	/// In some platforms it will not create the desired window size, so we query the real window size created
	#if SDL_VERSION_ATLEAST(2,0,1)
	int w, h;
	SDL_GL_GetDrawableSize( mSDLWindow, &w, &h );

	mWindow.WindowConfig.Width	= w;
	mWindow.WindowConfig.Height	= h;
	mWindow.WindowSize			= Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
	#endif

	#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS
		eePRINTL( "Choosing GL Version from: %d", Context.Version );

		if ( GLv_default != Context.Version ) {
			if ( GLv_ES1 == Context.Version || GLv_2 == Context.Version ) {
				if ( GLv_2 == Context.Version )
					mWindow.ContextConfig.Version = GLv_ES1;

				eePRINTL( "Starting GLES1" );

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			} else {
				eePRINTL( "Starting GLES2" );

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			}
		} else {
			#if defined( EE_GLES2 )
				eePRINTL( "Starting GLES2 default" );

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
			#else
				eePRINTL( "Starting GLES1 default" );

				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
			#endif
		}
	#else
		if ( GLv_3CP == Context.Version ) {
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
		}
	#endif

	#ifdef SDL2_THREADED_GLCONTEXT
		if ( mWindow.ContextConfig.SharedGLContext ) {
			SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

			mGLContext			= SDL_GL_CreateContext( mSDLWindow );
			mGLContextThread	= SDL_GL_CreateContext( mSDLWindow );
		} else {
			mGLContext			= SDL_GL_CreateContext( mSDLWindow );
		}
	#else
		mGLContext			= SDL_GL_CreateContext( mSDLWindow );
	#endif

	if ( NULL == mGLContext
	#ifdef SDL2_THREADED_GLCONTEXT
		 || ( mWindow.ContextConfig.SharedGLContext && NULL == mGLContextThread )
	#endif
	)
	{
		eePRINTL( "Unable to create context: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	SDL_GL_SetSwapInterval( ( mWindow.ContextConfig.VSync ? 1 : 0 ) );								// VSync

	SDL_GL_MakeCurrent( mSDLWindow, mGLContext );

	if ( NULL == Renderer::existsSingleton() ) {
		Renderer::createSingleton( mWindow.ContextConfig.Version );
		Renderer::instance()->init();
	}

	createPlatform();

	getMainContext();

	setCaption( mWindow.WindowConfig.Caption );

	createView();

	setup2D( false );

	mWindow.Created = true;

	if ( "" != mWindow.WindowConfig.Icon ) {
		setIcon( mWindow.WindowConfig.Icon );
	}

	/// Init the clipboard after the window creation
	reinterpret_cast<ClipboardSDL*> ( mClipboard )->init();

	/// Init the input after the window creation
	reinterpret_cast<InputSDL*> ( mInput )->init();

	mCursorManager->set( SYS_CURSOR_ARROW );

	logSuccessfulInit( getVersion() );

	return true;
}

bool WindowSDL::isThreadedGLContext() {
#ifdef SDL2_THREADED_GLCONTEXT
	return mWindow.ContextConfig.SharedGLContext;
#else
	return false;
#endif
}

void WindowSDL::setGLContextThread() {
	SDL_GL_MakeCurrent( mSDLWindow, mGLContextThread );
}

void WindowSDL::unsetGLContextThread() {
	SDL_GL_MakeCurrent( mSDLWindow, NULL );
}

int WindowSDL::getCurrentDisplayIndex() {
	return SDL_GetWindowDisplayIndex( mSDLWindow );
}

std::string WindowSDL::getVersion() {
	SDL_version ver;

	SDL_GetVersion( &ver );

	return String::strFormated( "SDL %d.%d.%d", ver.major, ver.minor, ver.patch );
}

void WindowSDL::createPlatform() {
	eeSAFE_DELETE( mPlatform );

#ifdef EE_USE_WMINFO
	mWMinfo = eeNew( WMInfo, ( mSDLWindow ) );
#endif

#if defined( EE_X11_PLATFORM )
	mPlatform = eeNew( Platform::X11Impl, ( this, mWMinfo->getWindowHandler(), mWMinfo->getWindow(), mWMinfo->getWindow(), NULL, NULL ) );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::WinImpl, ( this, getWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::OSXImpl, ( this ) );
#else
	Window::createPlatform();
#endif
}

void WindowSDL::setGLConfig() {
	if ( mWindow.ContextConfig.DepthBufferSize ) SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE	, mWindow.ContextConfig.DepthBufferSize );				// Depth
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, ( mWindow.ContextConfig.DoubleBuffering ? 1 : 0 ) );	// Double Buffering
	if ( mWindow.ContextConfig.StencilBufferSize ) SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, mWindow.ContextConfig.StencilBufferSize );

	if ( mWindow.WindowConfig.BitsPerPixel == 16 ) {
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE	, 4 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE	, 4 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE	, 4 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE	, 4 );
	} else {
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE	, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE	, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE	, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE	, 8 );
	}
}

void WindowSDL::toggleFullscreen() {
	eePRINTL( "toggleFullscreen called: %s", isWindowed() ? "is windowed" : "is fullscreen" );

	if ( isWindowed() ) {
		mWinPos = getPosition();
		mWindow.Maximized = isMaximized();
	}

	SDL_SetWindowFullscreen( mSDLWindow, isWindowed() ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 );

	BitOp::setBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, isWindowed() ? 1 : 0 );

	getCursorManager()->reload();

	if ( isWindowed() ) {
		setPosition( mWinPos.x, mWinPos.y );

		if ( mWindow.Maximized )
			maximize();
	}
}

void WindowSDL::setCaption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	SDL_SetWindowTitle( mSDLWindow, Caption.c_str() );
}

bool WindowSDL::isActive() {
	Uint32 flags = 0;

	flags = SDL_GetWindowFlags( mSDLWindow );

	return 0 != ( ( flags & SDL_WINDOW_INPUT_FOCUS ) && ( flags & SDL_WINDOW_MOUSE_FOCUS ) );
}

bool WindowSDL::isVisible() {
	Uint32 flags = 0;

	flags = SDL_GetWindowFlags( mSDLWindow );

	return 0 != ( ( flags & SDL_WINDOW_SHOWN ) && !( flags & SDL_WINDOW_MINIMIZED ) );
}

void WindowSDL::onWindowResize( Uint32 Width, Uint32 Height ) {
	if ( Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	eePRINTL( "onWindowResize: %d Height %d.", Width, Height );

	mWindow.WindowConfig.Width	= Width;
	mWindow.WindowConfig.Height	= Height;
	mWindow.WindowSize = Sizei( Width, Height );

	mDefaultView.reset( Rectf( 0, 0, Width, Height ) );

	setup2D( false );

	SDL_PumpEvents();

	SDL_FlushEvent( SDL_WINDOWEVENT );

	mCursorManager->reload();

	sendVideoResizeCb();
}

void WindowSDL::setSize( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.getWidth();
		Height	= mWindow.DesktopResolution.getHeight();
	}

	if ( this->isWindowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	eePRINTL( "Switching from %s to %s. Width: %d Height %d.", this->isWindowed() ? "windowed" : "fullscreen", Windowed ? "windowed" : "fullscreen", Width, Height );

	Uint32 oldWidth		= mWindow.WindowConfig.Width;
	Uint32 oldHeight	= mWindow.WindowConfig.Height;

	mWindow.WindowConfig.Width	= Width;
	mWindow.WindowConfig.Height	= Height;

	if ( Windowed ) {
		mWindow.WindowSize = Sizei( Width, Height );
	} else {
		mWindow.WindowSize = Sizei( oldWidth, oldHeight );
	}

	if ( isWindowed() && !Windowed ) {
		mWinPos = getPosition();
	} else {
		SDL_SetWindowFullscreen( mSDLWindow, Windowed ? 0 : SDL_WINDOW_FULLSCREEN );
	}

	SDL_SetWindowSize( mSDLWindow, Width, Height );

	if ( isWindowed() && !Windowed ) {
		mWinPos = getPosition();

		setGLConfig();

		SDL_SetWindowFullscreen( mSDLWindow, Windowed ? 0 : SDL_WINDOW_FULLSCREEN );
	}

	if ( isWindowed() && Windowed ) {
		setPosition( mWinPos.x, mWinPos.y );
	}

	BitOp::setBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

	mDefaultView.reset( Rectf( 0, 0, Width, Height ) );

	setup2D( false );

	SDL_PumpEvents();

	SDL_FlushEvent( SDL_WINDOWEVENT );

	mCursorManager->reload();

	sendVideoResizeCb();
}

void WindowSDL::swapBuffers() {	
	SDL_GL_SwapWindow( mSDLWindow );
}

std::vector<DisplayMode> WindowSDL::getDisplayModes() const {
	std::vector<DisplayMode> result;

	int displays = SDL_GetNumVideoDisplays();

	for ( int x = 0; x < displays; x++ ) {
		int displayModes = SDL_GetNumDisplayModes(x);

		for ( int i = 0; i < displayModes; i++ ) {
			SDL_DisplayMode mode;
			SDL_GetDisplayMode( x, i, &mode );

			result.push_back( DisplayMode( mode.w, mode.h, mode.refresh_rate, x ) );
		}
	}

	return result;
}

void WindowSDL::setGamma( Float Red, Float Green, Float Blue ) {
	eeclamp( &Red	, (Float)0.1f, (Float)10.0f );
	eeclamp( &Green	, (Float)0.1f, (Float)10.0f );
	eeclamp( &Blue	, (Float)0.1f, (Float)10.0f );

	Uint16 red_ramp[256];
	Uint16 green_ramp[256];
	Uint16 blue_ramp[256];

	SDL_CalculateGammaRamp( Red, red_ramp );

	if ( Green == Red ) {
		SDL_memcpy(green_ramp, red_ramp, sizeof(red_ramp));
	} else {
		SDL_CalculateGammaRamp( Green, green_ramp );
	}

	if ( Blue == Red ) {
		SDL_memcpy( blue_ramp, red_ramp, sizeof(red_ramp) );
	} else if ( Blue == Green ) {
		SDL_memcpy( blue_ramp, green_ramp, sizeof(green_ramp) );
	} else {
		SDL_CalculateGammaRamp( Blue, blue_ramp );
	}

	SDL_SetWindowGammaRamp( mSDLWindow, red_ramp, green_ramp, blue_ramp );
}

eeWindowHandle	WindowSDL::getWindowHandler() {
#ifdef EE_USE_WMINFO
	if ( NULL != mWMinfo ) {
		return mWMinfo->getWindowHandler();
	}
#endif
	return 0;
}

bool WindowSDL::setIcon( const std::string& Path ) {
	int x, y, c;

	if ( !mWindow.Created ) {
		if ( stbi_info( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon 	= Path;

			return true;
		}

		return false;
	}

	Image Img( Path );

	if ( NULL != Img.getPixelsPtr() ) {
		const Uint8 * Ptr = Img.getPixelsPtr();
		x = Img.getWidth();
		y = Img.getHeight();
		c = Img.getChannels();

		if ( ( x  % 8 ) == 0 && ( y % 8 ) == 0 ) {
			Uint32 rmask, gmask, bmask, amask;
			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				rmask = 0xff000000;
				gmask = 0x00ff0000;
				bmask = 0x0000ff00;
				amask = 0x000000ff;
			#else
				rmask = 0x000000ff;
				gmask = 0x0000ff00;
				bmask = 0x00ff0000;
				amask = 0xff000000;
			#endif
			SDL_Surface * TempGlyphSheet = SDL_CreateRGBSurface( SDL_SWSURFACE, x, y, c * 8, rmask, gmask, bmask, amask );

			SDL_LockSurface( TempGlyphSheet );

			Uint32 ssize = TempGlyphSheet->w * TempGlyphSheet->h * c;

			for ( Uint32 i=0; i < ssize; i++ ) {
				( static_cast<Uint8*>( TempGlyphSheet->pixels ) )[i+0] = (Ptr)[i];
			}

			SDL_UnlockSurface( TempGlyphSheet );

			SDL_SetWindowIcon( mSDLWindow, TempGlyphSheet );

			SDL_FreeSurface( TempGlyphSheet );

			return true;
		}
	}

	return false;
}

void WindowSDL::minimize() {
	SDL_MinimizeWindow( mSDLWindow );
}

void WindowSDL::maximize() {
	SDL_MaximizeWindow( mSDLWindow );
}

void WindowSDL::hide() {
	SDL_HideWindow( mSDLWindow );
}

void WindowSDL::raise() {
	SDL_RaiseWindow( mSDLWindow );
}

void WindowSDL::show() {
	SDL_ShowWindow( mSDLWindow );
}

void WindowSDL::setPosition( Int16 Left, Int16 Top ) {
	SDL_SetWindowPosition( mSDLWindow, Left, Top );
}

Vector2i WindowSDL::getPosition() {
	Vector2i p;

	SDL_GetWindowPosition( mSDLWindow, &p.x, &p.y );

	return p;
}

void WindowSDL::updateDesktopResolution() {
	SDL_DisplayMode dpm;
	SDL_GetDesktopDisplayMode( SDL_GetWindowDisplayIndex( mSDLWindow ), &dpm );

	mWindow.DesktopResolution = Sizei( dpm.w, dpm.h );
}

const Sizei& WindowSDL::getDesktopResolution() {
	updateDesktopResolution();
	return Window::getDesktopResolution();
}

SDL_Window * WindowSDL::GetSDLWindow() const {
	return mSDLWindow;
}

void WindowSDL::startTextInput() {
	SDL_StartTextInput();
}

bool WindowSDL::isTextInputActive() {
	return SDL_TRUE == SDL_IsTextInputActive();
}

void WindowSDL::stopTextInput() {
	SDL_StopTextInput();
}

void WindowSDL::setTextInputRect( Rect& rect ) {
	SDL_Rect r;

	r.x = rect.Left;
	r.y = rect.Top;
	r.w = rect.getSize().getWidth();
	r.h = rect.getSize().getHeight();

	SDL_SetTextInputRect( &r );

	rect.Left	= r.x;
	rect.Top	= r.y;
	rect.Right	= rect.Left + r.w;
	rect.Bottom	= rect.Top + r.h;
}

bool WindowSDL::hasScreenKeyboardSupport() {
	return SDL_TRUE == SDL_HasScreenKeyboardSupport();
}

bool WindowSDL::isScreenKeyboardShown() {
	return SDL_TRUE == SDL_IsScreenKeyboardShown( mSDLWindow );
}

}}}}

#endif
