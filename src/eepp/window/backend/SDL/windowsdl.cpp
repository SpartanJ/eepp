#include <eepp/window/backend/SDL/windowsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

#if !defined( EE_COMPILER_MSVC )
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	#if !defined( EE_COMPILER_MSVC )
		#include <SDL/SDL_syswm.h>
	#else
		#include <SDL_syswm.h>
	#endif
#endif

#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>

#include <eepp/window/backend/SDL/clipboardsdl.hpp>
#include <eepp/window/backend/SDL/inputsdl.hpp>
#include <eepp/window/backend/SDL/cursormanagersdl.hpp>
#include <eepp/window/platform/platformimpl.hpp>

#include <eepp/graphics/renderer/gl.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/texturefactory.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL {

WindowSDL::WindowSDL( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardSDL, ( this ) ), eeNew( InputSDL, ( this ) ), eeNew( CursorManagerSDL, ( this ) ) ),
	mSurface( NULL )
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	,
	mWMinfo( eeNew( SDL_SysWMinfo, () ) )
#endif
{
	Create( Settings, Context );
}

WindowSDL::~WindowSDL() {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	eeSAFE_DELETE( mWMinfo );
#endif
}

bool WindowSDL::Create( WindowSettings Settings, ContextSettings Context ) {
	if ( mWindow.Created )
		return false;

	mWindow.WindowConfig	= Settings;
	mWindow.ContextConfig	= Context;

	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
		eePRINTL( "Unable to initialize SDL: %s", SDL_GetError() );

		LogFailureInit( "WindowSDL", GetVersion() );

		return false;
	}

	if ( "" != mWindow.WindowConfig.Icon ) {
		mWindow.Created = true;
		Icon( mWindow.WindowConfig.Icon );
		mWindow.Created = false;
	}

	const SDL_VideoInfo * videoInfo = SDL_GetVideoInfo();

	mWindow.DesktopResolution = Sizei( videoInfo->current_w, videoInfo->current_h );

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
		eePRINTL( "Video Mode Unsopported for this videocard: " );

		LogFailureInit( "WindowSDL", GetVersion() );

		return false;
	}

	mWindow.WindowSize = Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );

	if ( NULL == mSurface ) {
		eePRINTL( "Unable to set video mode: %s", SDL_GetError() );

		LogFailureInit( "WindowSDL", GetVersion() );

		return false;
	}

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

	if ( NULL == cGL::ExistsSingleton() ) {
		cGL::CreateSingleton( mWindow.ContextConfig.Version );
		cGL::instance()->Init();
	}

	CreatePlatform();

	GetMainContext();

	Caption( mWindow.WindowConfig.Caption );

	CreateView();

	Setup2D();

	mWindow.Created = true;

	LogSuccessfulInit( GetVersion() );

	/// Init the clipboard after the window creation
	reinterpret_cast<ClipboardSDL*> ( mClipboard )->Init();

	/// Init the input after the window creation
	reinterpret_cast<InputSDL*> ( mInput )->Init();

	mCursorManager->Set( SYS_CURSOR_ARROW );

	return true;
}

std::string WindowSDL::GetVersion() {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	SDL_version ver = mWMinfo->version;

	return String::StrFormated( "SDL %d.%d.%d", ver.major, ver.minor, ver.patch );
#else
	return String::StrFormated( "SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
#endif
}

void WindowSDL::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	SDL_VERSION( &mWMinfo->version );

	SDL_GetWMInfo ( mWMinfo );
#endif

#if defined( EE_X11_PLATFORM )
	mPlatform = eeNew( Platform::X11Impl, ( this, mWMinfo->info.x11.display, mWMinfo->info.x11.wmwindow, mWMinfo->info.x11.window, mWMinfo->info.x11.lock_func, mWMinfo->info.x11.unlock_func ) );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::WinImpl, ( this, GetWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::OSXImpl, ( this ) );
#else
	Window::CreatePlatform();
#endif
}

void WindowSDL::SetGLConfig() {
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE	, mWindow.ContextConfig.DepthBufferSize );				// Depth
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, ( mWindow.ContextConfig.DoubleBuffering ? 1 : 0 ) );	// Double Buffering
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, mWindow.ContextConfig.StencilBufferSize );

	#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, ( mWindow.ContextConfig.VSync ? 1 : 0 )  );			// VSync
	#endif
}

void WindowSDL::ToggleFullscreen() {
	bool WasMaximized = mWindow.Maximized;

	if ( Windowed() ) {
		Size( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, !Windowed() );
	} else {
		Size( mWindow.WindowSize.Width(), mWindow.WindowSize.Height(), !Windowed() );
	}

	if ( WasMaximized ) {
		Maximize();
	}

	GetCursorManager()->Reload();
}

void WindowSDL::Caption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	SDL_WM_SetCaption( Caption.c_str(), NULL );
}

bool WindowSDL::Icon( const std::string& Path ) {
	int x, y, c;

	if ( !mWindow.Created ) {
		if ( stbi_info( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon 	= Path;

			return true;
		}

		return false;
	}

	Image Img( Path );

	if ( NULL != Img.GetPixelsPtr() ) {
		const Uint8 * Ptr = Img.GetPixelsPtr();
		x = Img.Width();
		y = Img.Height();
		c = Img.Channels();

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

			SDL_WM_SetIcon( TempGlyphSheet, NULL );

			SDL_FreeSurface( TempGlyphSheet );

			return true;
		}
	}

	return false;
}

bool WindowSDL::Active() {
	return 0 != ( SDL_GetAppState() & SDL_APPINPUTFOCUS );
}

bool WindowSDL::Visible() {
	return 0 != ( SDL_GetAppState() & SDL_APPACTIVE );
}

void WindowSDL::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.Width();
		Height	= mWindow.DesktopResolution.Height();
	}

	if ( this->Windowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	eePRINTL( "Switching from %s to %s. Width: %d Height %d.", this->Windowed() ? "windowed" : "fullscreen", Windowed ? "windowed" : "fullscreen", Width, Height );

	#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
	#if EE_PLATFORM == EE_PLATFORM_WIN
	bool Reload = this->Windowed() != Windowed;
	#else
	bool Reload = true;
	#endif

	if ( Reload )
		Graphics::TextureFactory::instance()->GrabTextures();
	#endif

	Uint32 oldWidth		= mWindow.WindowConfig.Width;
	Uint32 oldHeight	= mWindow.WindowConfig.Height;

	mWindow.WindowConfig.Width	= Width;
	mWindow.WindowConfig.Height	= Height;

	if ( Windowed ) {
		mWindow.WindowSize = Sizei( Width, Height );
	} else {
		mWindow.WindowSize = Sizei( oldWidth, oldHeight );
	}

	if ( this->Windowed() && !Windowed ) {
		mWinPos = Position();
	}

	SetGLConfig();

	if ( Windowed ) {
		mSurface = SDL_SetVideoMode( Width, Height, mWindow.WindowConfig.BitsPerPixel, mWindow.Flags );
	} else {
		mSurface = SDL_SetVideoMode( Width, Height, mWindow.WindowConfig.BitsPerPixel, mWindow.Flags | SDL_FULLSCREEN );
	}

	#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX
	if ( Reload ) {
		cGL::instance()->Init();

		Graphics::TextureFactory::instance()->UngrabTextures();		// Reload all textures
		Graphics::ShaderProgramManager::instance()->Reload();			// Reload all shaders
		Graphics::Private::FrameBufferManager::instance()->Reload(); 	// Reload all frame buffers
		Graphics::Private::VertexBufferManager::instance()->Reload(); 	// Reload all vertex buffers
		GetMainContext();												// Recover the context
		CreatePlatform();
	}
	#endif

	if ( !this->Windowed() && Windowed ) {
		Position( mWinPos.x, mWinPos.y );
	}

	BitOp::SetBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

	mDefaultView.SetView( 0, 0, Width, Height );

	Setup2D();

	mCursorManager->Reload();

	SendVideoResizeCb();

	if ( NULL == mSurface ) {
		mWindow.Created = false;
	}
}

void WindowSDL::SwapBuffers() {
	SDL_GL_SwapBuffers();
}

std::vector<DisplayMode> WindowSDL::GetDisplayModes() const {
	SDL_Rect **modes = SDL_ListModes( NULL, SDL_OPENGL | SDL_HWPALETTE | SDL_HWACCEL | SDL_FULLSCREEN );

	if(modes == (SDL_Rect **)0)
		eePRINTL("No VideoMode Found");

	std::vector<DisplayMode> result;
	if( modes != (SDL_Rect **)-1 )
		for(unsigned int i = 0; modes[i]; ++i)
			result.push_back( DisplayMode(modes[i]->w, modes[i]->h,60,0) );

	return result;
}

void WindowSDL::SetGamma( Float Red, Float Green, Float Blue ) {
	eeclamp<Float>( &Red		, 0.1f, 10.0f );
	eeclamp<Float>( &Green	, 0.1f, 10.0f );
	eeclamp<Float>( &Blue		, 0.1f, 10.0f );
	SDL_SetGamma( Red, Green, Blue );
}

eeWindowHandle	WindowSDL::GetWindowHandler() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return mWMinfo->window;
#elif defined( EE_X11_PLATFORM )
	return mWMinfo->info.x11.display;
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	return mWMinfo->info.x11.display;
#else
	return 0;
#endif
}

}}}}

#endif
