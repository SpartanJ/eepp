#include "cwindowsdl.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "cclipboardsdl.hpp"
#include "cinputsdl.hpp"
#include "../../../graphics/cglobalbatchrenderer.hpp"
#include "../../../graphics/cshaderprogrammanager.hpp"
#include "../../../graphics/cvertexbuffermanager.hpp"
#include "../../../graphics/cframebuffermanager.hpp"
#include "../../../graphics/ctexturefactory.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL {

cWindowSDL::cWindowSDL( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardSDL, ( this ) ), eeNew( cInputSDL, ( this ) ) )
{
	Create( Settings, Context );
}

cWindowSDL::~cWindowSDL() {
}

bool cWindowSDL::Create( WindowSettings Settings, ContextSettings Context ) {
	try {
		if ( mWindow.Created )
			return false;

		mWindow.WindowConfig	= Settings;
		mWindow.ContextConfig	= Context;

		if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
			cLog::instance()->Write( "Unable to initialize SDL: " + std::string( SDL_GetError() ) );
			return false;
                }

		if ( "" != mWindow.WindowConfig.Icon ) {
			mWindow.Created = true;
			Icon( mWindow.WindowConfig.Icon );
			mWindow.Created = false;
		}

		const SDL_VideoInfo * videoInfo = SDL_GetVideoInfo();

		mWindow.DesktopResolution = eeSize( videoInfo->current_w, videoInfo->current_h );

		if ( mWindow.WindowConfig.Style & WindowStyle::UseDesktopResolution ) {
			mWindow.WindowConfig.Width	= mWindow.DesktopResolution.Width();
			mWindow.WindowConfig.Height	= mWindow.DesktopResolution.Height();
		}

		mWindow.Flags = SDL_OPENGL | SDL_HWPALETTE;

		if ( mWindow.WindowConfig.Style & WindowStyle::Resize ) {
			mWindow.Flags |= SDL_RESIZABLE;
		}

		if ( mWindow.WindowConfig.Style & WindowStyle::NoBorder ) {
			mWindow.Flags |= SDL_NOFRAME;
		}

		SetGLConfig();

		Uint32 mTmpFlags = mWindow.Flags;

		if ( mWindow.WindowConfig.Style & WindowStyle::Fullscreen ) {
    		mTmpFlags |= SDL_FULLSCREEN;
		}

		if ( SDL_VideoModeOK( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, mWindow.WindowConfig.BitsPerPixel, mTmpFlags ) ) {
			mSurface = SDL_SetVideoMode( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, mWindow.WindowConfig.BitsPerPixel, mTmpFlags );
		} else {
			cLog::instance()->Write( "Video Mode Unsopported for this videocard: " );
			return false;
		}

		mWindow.WindowSize = eeSize( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );

		if ( NULL == mSurface ) {
			cLog::instance()->Write( "Unable to set video mode: " + std::string( SDL_GetError() ) );
			return false;
		}

		SDL_VERSION( &mWMinfo.version );
		SDL_GetWMInfo ( &mWMinfo );

		if ( mWindow.WindowConfig.BitsPerPixel == 16 ) {
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE	, 4 );
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE	, 4 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE	, 4 );
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE	, 4 );
		} else {
			SDL_GL_SetAttribute( SDL_GL_RED_SIZE	, 8);
			SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE	, 8 );
			SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE	, 8 );
			SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE	, 8 );
		}

		Caption( mWindow.WindowConfig.Caption );

		if ( NULL == cGL::ExistsSingleton() ) {
			cGL::CreateSingleton( mWindow.ContextConfig.Version );
			cGL::instance()->Init();
		}

		GetMainContext();

		CreateView();

		Setup2D();

		mWindow.Created = true;

		LogSuccessfulInit( "SDL" );

		/// Init the clipboard after the window creation
		reinterpret_cast<cClipboardSDL*> ( mClipboard )->Init();

		/// Init the input after the window creation
		reinterpret_cast<cInputSDL*> ( mInput )->Init();

		return true;
	} catch (...) {
		LogFailureInit( "cWindowSDL", "SDL" );
		return false;
	}
}

void cWindowSDL::SetGLConfig() {
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE	, mWindow.ContextConfig.DepthBufferSize );				// Depth
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, ( mWindow.ContextConfig.DoubleBuffering ? 1 : 0 ) );	// Double Buffering
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, mWindow.ContextConfig.StencilBufferSize );
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, ( mWindow.ContextConfig.VSync ? 1 : 0 )  );			// VSync
}

void cWindowSDL::ToggleFullscreen() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
		bool WasMaximized = mWindow.Maximized;

		if ( Windowed() ) {
			Size( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, !Windowed() );
		} else {
			Size( mWindow.WindowSize.Width(), mWindow.WindowSize.Height(), !Windowed() );
		}

		if ( WasMaximized ) {
			Maximize();
		}
	#else
		SetFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !( mWindow.WindowConfig.Style & WindowStyle::Fullscreen ) );

		SDL_WM_ToggleFullScreen( mSurface );
	#endif

	ShowCursor( mWindow.CursorVisible );
}

void cWindowSDL::Caption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	SDL_WM_SetCaption( Caption.c_str(), NULL );
}

bool cWindowSDL::Icon( const std::string& Path ) {
	int x, y, c;

	if ( !FileExists( Path  ) ) {
		return false;
	}

	if ( !mWindow.Created ) {
		if ( stbi_info( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon 	= Path;

			return true;
		}

		return false;
	}

	unsigned char * Ptr = SOIL_load_image( Path.c_str(), &x, &y, &c, SOIL_LOAD_AUTO );

	if ( NULL != Ptr ) {
		Int32 W = x;
		Int32 H = y;

		if ( ( W  % 8 ) == 0 && ( H % 8 ) == 0 ) {
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
			SDL_Surface * TempGlyphSheet = SDL_CreateRGBSurface(SDL_SWSURFACE, W, H, c * 8, rmask, gmask, bmask, amask);

			SDL_LockSurface( TempGlyphSheet );

			Uint32 ssize = TempGlyphSheet->w * TempGlyphSheet->h * c;

			for ( Uint32 i=0; i < ssize; i++ ) {
				( static_cast<Uint8*>( TempGlyphSheet->pixels ) )[i+0] = (Ptr)[i];
			}

			SDL_UnlockSurface( TempGlyphSheet );

			SDL_WM_SetIcon( TempGlyphSheet, NULL );

			SDL_FreeSurface( TempGlyphSheet );

			free( Ptr );

			return true;
		}

		free( Ptr );
	}

	return false;
}

void cWindowSDL::Minimize() {
	SDL_WM_IconifyWindow();
}

void cWindowSDL::Maximize() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
		WIN_ShowWindow(mWMinfo.window, SW_MAXIMIZE);
	#elif EE_PLATFORM == EE_PLATFORM_LINUX
		// coded by Rafał Maj, idea from Måns Rullgård http://tinyurl.com/68mvk3
		mWMinfo.info.x11.lock_func();

		XEvent xev;
		Atom wm_state =  XInternAtom( mWMinfo.info.x11.display, "_NET_WM_STATE", False);
		Atom maximizeV = XInternAtom( mWMinfo.info.x11.display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
		Atom maximizeH = XInternAtom( mWMinfo.info.x11.display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

		memset( &xev, 0, sizeof(xev) );
		xev.type = ClientMessage;
		xev.xclient.window = mWMinfo.info.x11.wmwindow;
		xev.xclient.message_type = wm_state;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = 1;
		xev.xclient.data.l[1] = maximizeV;
		xev.xclient.data.l[2] = maximizeH;
		xev.xclient.data.l[3] = 0;
		XSendEvent( mWMinfo.info.x11.display, DefaultRootWindow(mWMinfo.info.x11.display), 0, SubstructureNotifyMask|SubstructureRedirectMask, &xev);

		XFlush(mWMinfo.info.x11.display);

		mWMinfo.info.x11.unlock_func();
	#else
		#warning cWindowSDL::MaximizeWindow() not implemented on this platform.
	#endif
}

void cWindowSDL::Hide() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	mWMinfo.info.x11.lock_func();
    XUnmapWindow( mWMinfo.info.x11.display, mWMinfo.info.x11.wmwindow );
	mWMinfo.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN
    WIN_ShowWindow( mWMinfo.window, SW_HIDE );
#else
	#warning cWindowSDL::HideWindow() not implemented on this platform.
#endif
}

void cWindowSDL::Raise() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	mWMinfo.info.x11.lock_func();
    XRaiseWindow( mWMinfo.info.x11.display, mWMinfo.info.x11.wmwindow );
	mWMinfo.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN
    HWND top;

	if ( !Windowed() )
        top = HWND_TOPMOST;
    else
        top = HWND_NOTOPMOST;

    SetWindowPos( mWMinfo.window, top, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE) );
#else
	#warning cWindowSDL::RaiseWindow() not implemented on this platform.
#endif
}

void cWindowSDL::Show() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	mWMinfo.info.x11.lock_func();
	XMapRaised( mWMinfo.info.x11.display, mWMinfo.info.x11.wmwindow );
	mWMinfo.info.x11.unlock_func();
#elif EE_PLATFORM == EE_PLATFORM_WIN
	WIN_ShowWindow( mWMinfo.window, SW_SHOW );
#else
	#warning cWindowSDL::RaiseWindow() not implemented on this platform.
#endif
}

void cWindowSDL::Position( Int16 Left, Int16 Top ) {
#if EE_PLATFORM == EE_PLATFORM_LINUX
    XMoveWindow( mWMinfo.info.x11.display, mWMinfo.info.x11.wmwindow, Left, Top);
    XFlush( mWMinfo.info.x11.display );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	SetWindowPos( mWMinfo.window, NULL, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
#else
	#warning cWindowSDL::SetWindowPosition() not implemented on this platform.
#endif
}

bool cWindowSDL::Active() {
	return 0 != ( SDL_GetAppState() & SDL_APPINPUTFOCUS );
}

bool cWindowSDL::Visible() {
	return 0 != ( SDL_GetAppState() & SDL_APPACTIVE );
}

eeVector2i cWindowSDL::Position() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	XWindowAttributes Attrs;
	XGetWindowAttributes( mWMinfo.info.x11.display, mWMinfo.info.x11.wmwindow, &Attrs );

	return eeVector2i( Attrs.x, Attrs.y );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	RECT r;
	GetWindowRect( mWMinfo.window, &r );
	return eeVector2i( r.left, r.top );
#else
	#warning cWindowSDL::GetWindowPos() not implemented on this platform.
	return eeVector2i( 0, 0 );
#endif
}

void cWindowSDL::Size( const Uint32& Width, const Uint32& Height ) {
	if ( Windowed() ) {
		Size( Width, Height, true );
	}
}

void cWindowSDL::Size( const Uint16& Width, const Uint16& Height, const bool& Windowed ) {
	try {
		cLog::instance()->Writef( "Switching from %s to %s. Width: %d Height %d.", this->Windowed() ? "windowed" : "fullscreen", Windowed ? "windowed" : "fullscreen", Width, Height );

		#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
		#if EE_PLATFORM == EE_PLATFORM_WIN
		bool Reload = this->Windowed() != Windowed;
		#else
		bool Reload = true;
		#endif

		if ( Reload )
			Graphics::cTextureFactory::instance()->GrabTextures();
		#endif

		SetFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

		mWindow.WindowConfig.Width    = Width;
		mWindow.WindowConfig.Height   = Height;

		if ( Windowed ) {
			mWindow.WindowSize = eeSize( Width, Height );
		}

		mDefaultView.SetView( 0, 0, Width, Height );

		SetGLConfig();

		if ( Windowed ) {
			mSurface = SDL_SetVideoMode( Width, Height, mWindow.WindowConfig.BitsPerPixel, mWindow.Flags );
		} else {
			mSurface = SDL_SetVideoMode( Width, Height, mWindow.WindowConfig.BitsPerPixel, mWindow.Flags | SDL_FULLSCREEN );
		}

		Setup2D();

		#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
		if ( Reload ) {
			Graphics::cTextureFactory::instance()->UngrabTextures();		// Reload all textures
			Graphics::cShaderProgramManager::instance()->Reload();			// Reload all shaders
			Graphics::Private::cFrameBufferManager::instance()->Reload(); 	// Reload all frame buffers
			Graphics::Private::cVertexBufferManager::instance()->Reload(); 	// Reload all vertex buffers
			GetMainContext();												// Recover the context
		}
		#endif

		SendVideoResizeCb();

		if ( NULL == mSurface ) {
			mWindow.Created = false;
		}
	} catch (...) {
		cLog::instance()->Write( "Unable to change resolution: " + std::string( SDL_GetError() ) );
		cLog::instance()->Save();
		mWindow.Created = false;
	}
}

void cWindowSDL::ShowCursor( const bool& showcursor ) {
	mWindow.CursorVisible = showcursor;

	SDL_ShowCursor( mWindow.CursorVisible ? SDL_ENABLE : SDL_DISABLE );
}

void cWindowSDL::SwapBuffers() {
	SDL_GL_SwapBuffers();
}

std::vector< std::pair<unsigned int, unsigned int> > cWindowSDL::GetPossibleResolutions() const {
	SDL_Rect **modes = SDL_ListModes( NULL, SDL_OPENGL | SDL_HWPALETTE | SDL_HWACCEL | SDL_FULLSCREEN );

	if(modes == (SDL_Rect **)0)
		cLog::instance()->Write("No VideoMode Found");

	std::vector< std::pair<unsigned int, unsigned int> > result;
	if( modes != (SDL_Rect **)-1 )
		for(unsigned int i = 0; modes[i]; ++i)
			result.push_back( std::pair<unsigned int, unsigned int>(modes[i]->w, modes[i]->h) );

	return result;
}

void cWindowSDL::SetGamma( eeFloat Red, eeFloat Green, eeFloat Blue ) {
	eeclamp( &Red	, 0.1f, 10.0f );
	eeclamp( &Green	, 0.1f, 10.0f );
	eeclamp( &Blue	, 0.1f, 10.0f );
	SDL_SetGamma( Red, Green, Blue );
}

eeWindowHandler	cWindowSDL::GetWindowHandler() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return mWMinfo.window;
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	return mWMinfo.info.x11.display;
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	return mWMinfo.cocoa.window;
#else
	return 0;
#endif
}

void cWindowSDL::SetCurrentContext( eeWindowContex Context ) {
	if ( mWindow.Created ) {
		#ifdef EE_GLEW_AVAILABLE

		#if EE_PLATFORM == EE_PLATFORM_WIN
			wglMakeCurrent( GetDC( mWMinfo.window ), Context );
		#elif EE_PLATFORM == EE_PLATFORM_LINUX
			mWMinfo.info.x11.lock_func();
			glXMakeCurrent( mWMinfo.info.x11.display, mWMinfo.info.x11.window, Context );
			mWMinfo.info.x11.unlock_func();
		#elif EE_PLATFORM == EE_PLATFORM_MACOSX
			aglSetCurrentContext( Context );
		#else
			#warning No context supported on this platform
		#endif

		#endif
	}
}

}}}}

#endif
