#include <eepp/window/backend/SDL/cwindowsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <SDL/SDL.h>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
#include <SDL/SDL_syswm.h>
#endif

#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>

#include <eepp/window/backend/SDL/cclipboardsdl.hpp>
#include <eepp/window/backend/SDL/cinputsdl.hpp>
#include <eepp/window/backend/SDL/ccursormanagersdl.hpp>
#include <eepp/window/platform/platformimpl.hpp>

#include <eepp/graphics/renderer/cgl.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/cshaderprogrammanager.hpp>
#include <eepp/graphics/cvertexbuffermanager.hpp>
#include <eepp/graphics/cframebuffermanager.hpp>
#include <eepp/graphics/ctexturefactory.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL {

cWindowSDL::cWindowSDL( WindowSettings Settings, ContextSettings Context ) :
	cWindow( Settings, Context, eeNew( cClipboardSDL, ( this ) ), eeNew( cInputSDL, ( this ) ), eeNew( cCursorManagerSDL, ( this ) ) ),
	mSurface( NULL ),
	mWMinfo( eeNew( SDL_SysWMinfo, () ) )
{
	Create( Settings, Context );
}

cWindowSDL::~cWindowSDL() {
	eeSAFE_DELETE( mWMinfo );
}

bool cWindowSDL::Create( WindowSettings Settings, ContextSettings Context ) {
	if ( mWindow.Created )
		return false;

	mWindow.WindowConfig	= Settings;
	mWindow.ContextConfig	= Context;

	if ( SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
		cLog::instance()->Write( "Unable to initialize SDL: " + std::string( SDL_GetError() ) );

		LogFailureInit( "cWindowSDL", GetVersion() );

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

		LogFailureInit( "cWindowSDL", GetVersion() );

		return false;
	}

	mWindow.WindowSize = eeSize( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );

	if ( NULL == mSurface ) {
		cLog::instance()->Write( "Unable to set video mode: " + std::string( SDL_GetError() ) );

		LogFailureInit( "cWindowSDL", GetVersion() );

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
	reinterpret_cast<cClipboardSDL*> ( mClipboard )->Init();

	/// Init the input after the window creation
	reinterpret_cast<cInputSDL*> ( mInput )->Init();

	mCursorManager->Set( Cursor::SYS_CURSOR_DEFAULT );

	return true;
}

std::string cWindowSDL::GetVersion() {
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	SDL_version ver = mWMinfo->version;

	return String::StrFormated( "SDL %d.%d.%d", ver.major, ver.minor, ver.patch );
#else
	return String::StrFormated( "SDL %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
#endif
}

void cWindowSDL::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );
#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM )
	SDL_VERSION( &mWMinfo->version );

	SDL_GetWMInfo ( mWMinfo );
#endif

#if defined( EE_X11_PLATFORM )
	mPlatform = eeNew( Platform::cX11Impl, ( this, mWMinfo->info.x11.display, mWMinfo->info.x11.wmwindow, mWMinfo->info.x11.window, mWMinfo->info.x11.lock_func, mWMinfo->info.x11.unlock_func ) );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::cWinImpl, ( this, GetWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::cOSXImpl, ( this ) );
#else
	cWindow::CreatePlatform();
#endif
}

void cWindowSDL::SetGLConfig() {
	SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE	, mWindow.ContextConfig.DepthBufferSize );				// Depth
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, ( mWindow.ContextConfig.DoubleBuffering ? 1 : 0 ) );	// Double Buffering
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, mWindow.ContextConfig.StencilBufferSize );
	SDL_GL_SetAttribute( SDL_GL_SWAP_CONTROL, ( mWindow.ContextConfig.VSync ? 1 : 0 )  );			// VSync
}

void cWindowSDL::ToggleFullscreen() {
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

void cWindowSDL::Caption( const std::string& Caption ) {
	mWindow.WindowConfig.Caption = Caption;

	SDL_WM_SetCaption( Caption.c_str(), NULL );
}

bool cWindowSDL::Icon( const std::string& Path ) {
	int x, y, c;

	if ( !mWindow.Created ) {
		if ( stbi_info( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon 	= Path;

			return true;
		}

		return false;
	}

	cImage * Img = eeNew( cImage, ( Path ) );

	if ( NULL == Img->GetPixelsPtr() ) {
		eeSAFE_DELETE( Img );
		Img = eeNew( cImage, ( Sys::GetProcessPath() + Path ) );
	}

	if ( NULL != Img->GetPixelsPtr() ) {
		const Uint8 * Ptr = Img->GetPixelsPtr();
		x = Img->Width();
		y = Img->Height();
		c = Img->Channels();

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

bool cWindowSDL::Active() {
	return 0 != ( SDL_GetAppState() & SDL_APPINPUTFOCUS );
}

bool cWindowSDL::Visible() {
	return 0 != ( SDL_GetAppState() & SDL_APPACTIVE );
}

void cWindowSDL::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.Width();
		Height	= mWindow.DesktopResolution.Height();
	}

	if ( this->Windowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	#ifdef EE_SUPPORT_EXCEPTIONS
	try {
	#endif
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

		Uint32 oldWidth		= mWindow.WindowConfig.Width;
		Uint32 oldHeight	= mWindow.WindowConfig.Height;

		mWindow.WindowConfig.Width    = Width;
		mWindow.WindowConfig.Height   = Height;

		if ( Windowed ) {
			mWindow.WindowSize = eeSize( Width, Height );
		} else {
			mWindow.WindowSize = eeSize( oldWidth, oldHeight );
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

			Graphics::cTextureFactory::instance()->UngrabTextures();		// Reload all textures
			Graphics::cShaderProgramManager::instance()->Reload();			// Reload all shaders
			Graphics::Private::cFrameBufferManager::instance()->Reload(); 	// Reload all frame buffers
			Graphics::Private::cVertexBufferManager::instance()->Reload(); 	// Reload all vertex buffers
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
	#ifdef EE_SUPPORT_EXCEPTIONS
	} catch (...) {
		cLog::instance()->Write( "Unable to change resolution: " + std::string( SDL_GetError() ) );
		cLog::instance()->Save();
		mWindow.Created = false;
	}
	#endif
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
	eeclamp<eeFloat>( &Red		, 0.1f, 10.0f );
	eeclamp<eeFloat>( &Green	, 0.1f, 10.0f );
	eeclamp<eeFloat>( &Blue		, 0.1f, 10.0f );
	SDL_SetGamma( Red, Green, Blue );
}

eeWindowHandle	cWindowSDL::GetWindowHandler() {
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
