#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/SDL2/cursormanagersdl2.hpp>
#include <eepp/window/backend/SDL2/inputsdl2.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>
#include <eepp/window/backend/SDL2/wminfo.hpp>
#include <eepp/window/engine.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/window/backend/SDL2/displaymanagersdl2.hpp>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS || \
	defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS ||       \
	EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#define SDL2_THREADED_GLCONTEXT
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <cstdint>
#include <initguid.h>
#include <objbase.h>
#include <shellapi.h>
#include <tlhelp32.h>

bool WindowsIsProcessRunning( const char* processName, bool killProcess = false ) {
	bool exists = false;
	PROCESSENTRY32 entry = {};
	entry.dwSize = sizeof( PROCESSENTRY32 );
	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( Process32First( snapshot, &entry ) ) {
		while ( Process32Next( snapshot, &entry ) ) {
#ifdef UNICODE
			if ( EE::String( entry.szExeFile ).toUtf8() == std::string( processName ) ) {
#else
			if ( !stricmp( entry.szExeFile, processName ) ) {
#endif
				exists = true;
				if ( killProcess ) {
					HANDLE aProc = OpenProcess( PROCESS_TERMINATE, 0, entry.th32ProcessID );
					if ( aProc ) {
						TerminateProcess( aProc, 9 );
						CloseHandle( aProc );
					}
				}
				break;
			}
		}
	}
	CloseHandle( snapshot );
	return exists;
}

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED ( 740 )
#endif // ERROR_ELEVATION_REQUIRED

bool WindowsProcessLaunch( std::string command, HWND windowHwnd ) {
#ifdef UNICODE
	wchar_t expandedCmd[1024] = {};
#else
	char expandedCmd[1024] = {};
#endif
	static PROCESS_INFORMATION pi = {};
	static STARTUPINFO si = {};
	si.cb = sizeof( si );
#if UNICODE
	ExpandEnvironmentStrings( EE::String::fromUtf8( command ).toWideString().c_str(), expandedCmd,
							  1024 );
#else
	ExpandEnvironmentStrings( command.c_str(), expandedCmd, 1024 );
#endif
	if ( CreateProcess( NULL, expandedCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ) {
		// Wait until child process exits.
		WaitForSingleObject( pi.hProcess, 10000 );
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return true;
	} else {
		DWORD error = GetLastError();
		if ( error == ERROR_ELEVATION_REQUIRED && 0 != windowHwnd ) {
#ifdef UNICODE
			std::intptr_t res = reinterpret_cast<std::intptr_t>(
				ShellExecute( windowHwnd, L"open", expandedCmd, L"", NULL, SW_SHOWDEFAULT ) );
#else
			std::intptr_t res = reinterpret_cast<std::intptr_t>(
				ShellExecute( windowHwnd, "open", expandedCmd, "", NULL, SW_SHOWDEFAULT ) );
#endif
			if ( res <= 32 ) {
				return false;
			}
		}
	}
	return false;
}

// 4ce576fa-83dc-4F88-951c-9d0782b4e376
DEFINE_GUID( CLSID_UIHostNoLaunch, 0x4CE576FA, 0x83DC, 0x4f88, 0x95, 0x1C, 0x9D, 0x07, 0x82, 0xB4,
			 0xE3, 0x76 );

// 37c994e7_432b_4834_a2f7_dce1f13b834b
DEFINE_GUID( IID_ITipInvocation, 0x37c994e7, 0x432b, 0x4834, 0xa2, 0xf7, 0xdc, 0xe1, 0xf1, 0x3b,
			 0x83, 0x4b );

struct ITipInvocation : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Toggle( HWND wnd ) = 0;
};

static bool WIN_OSK_VISIBLE = false;

int showOSK( HWND windowHwnd ) {
	if ( !WindowsIsProcessRunning( "TabTip.exe" ) ) {
		WindowsIsProcessRunning(
			"WindowsInternal.ComposableShell.Experiences.TextInput.InputApp.EXE", true );

		std::string programFiles( Sys::getOSArchitecture() == "x64" ? "%ProgramW6432%"
																	: "%ProgramFiles(x86)%" );
		WindowsProcessLaunch( programFiles + "\\Common Files\\microsoft shared\\ink\\TabTip.exe",
							  windowHwnd );
	}

	CoInitialize( 0 );

	ITipInvocation* tip;
	CoCreateInstance( CLSID_UIHostNoLaunch, 0, CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
					  IID_ITipInvocation, (void**)&tip );
	if ( tip != NULL ) {
		tip->Toggle( GetDesktopWindow() );
		tip->Release();
		WIN_OSK_VISIBLE = true;
	}

	return 0;
}

int hideOSK() {
	WIN_OSK_VISIBLE = false;
	return PostMessage( GetDesktopWindow(), WM_SYSCOMMAND, (int)SC_CLOSE, 0 );
}

bool isDarkModeEnabled() {
	HKEY hKey;
	DWORD value = 1; // Default to light theme
	DWORD valueSize = sizeof( value );

	if ( RegOpenKeyExA( HKEY_CURRENT_USER,
						"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
						KEY_READ, &hKey ) == ERROR_SUCCESS ) {
		RegQueryValueExA( hKey, "AppsUseLightTheme", nullptr, nullptr,
						  reinterpret_cast<LPBYTE>( &value ), &valueSize );
		RegCloseKey( hKey );
	}

	return value == 0; // 0 means dark theme is enabled
}

typedef HRESULT( WINAPI* DwmSetWindowAttributeFunc )( HWND, DWORD, LPCVOID, DWORD );

constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;

void setUserTheme( HWND hwnd ) {
	HMODULE hDwmapi = LoadLibraryA( "dwmapi.dll" );
	if ( !hDwmapi ) {
		return;
	}

	auto DwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
		GetProcAddress( hDwmapi, "DwmSetWindowAttribute" ) );
	if ( !DwmSetWindowAttribute ) {
		FreeLibrary( hDwmapi );
		return;
	}

	BOOL darkMode = isDarkModeEnabled() ? TRUE : FALSE;
	DwmSetWindowAttribute( hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof( darkMode ) );

	FreeLibrary( hDwmapi );
}
#elif defined( EE_X11_PLATFORM )
#include <signal.h>
#include <unistd.h>

static pid_t ONBOARD_PID = 0;

void showOSK() {
	if ( ONBOARD_PID == 0 ) {
		if ( FileSystem::fileExists( "/usr/bin/onboard" ) ) {
			pid_t pid = fork();

			if ( pid == 0 ) {
				execl( "/usr/bin/onboard", "onboard", NULL );
			} else if ( pid != -1 ) {
				ONBOARD_PID = pid;
			}
		} else {
			EE::System::Log::error(
				"\"onboard\" must be installed to be able to use the On Screen Keyboard" );
		}
	}
}

void hideOSK() {
	if ( ONBOARD_PID != 0 ) {
		kill( ONBOARD_PID, SIGTERM );
		ONBOARD_PID = 0;
	}
}
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

WindowSDL::WindowSDL( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardSDL, ( this ) ), eeNew( InputSDL, ( this ) ),
			eeNew( CursorManagerSDL, ( this ) ) ),
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

#ifdef EE_USE_WMINFO
	eeSAFE_DELETE( mWMinfo );
#endif

	if ( NULL != mSDLWindow ) {
		SDL_DestroyWindow( mSDLWindow );
	}

#if defined( EE_X11_PLATFORM )
	hideOSK();
#endif
}

bool WindowSDL::create( WindowSettings Settings, ContextSettings Context ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	DisplayManagerSDL2::setDPIAwareness();
#endif

	if ( mWindow.Created )
		return false;

	mWindow.WindowConfig = Settings;
	mWindow.ContextConfig = Context;

	if ( !SDL_WasInit( SDL_INIT_VIDEO ) && SDL_Init( SDL_INIT_VIDEO ) != 0 ) {
		Log::error( "Unable to initialize SDL: %s", SDL_GetError() );

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
		mWindow.WindowConfig.Width = mWindow.DesktopResolution.getWidth();
		mWindow.WindowConfig.Height = mWindow.DesktopResolution.getHeight();
	}

	mWindow.Flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

	if ( mWindow.WindowConfig.Style & WindowStyle::Resize ) {
		mWindow.Flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( mWindow.WindowConfig.Style & WindowStyle::Borderless ) {
		mWindow.Flags |= SDL_WINDOW_BORDERLESS;
	}

	setGLConfig();

	Uint32 tmpFlags = mWindow.Flags;

	if ( mWindow.WindowConfig.Style & WindowStyle::Fullscreen ) {
		tmpFlags |= SDL_WINDOW_FULLSCREEN;
	}

	if ( mWindow.ContextConfig.Multisamples > 0 ) {
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLEBUFFERS, 1 );
		SDL_GL_SetAttribute( SDL_GL_MULTISAMPLESAMPLES, mWindow.ContextConfig.Multisamples );
	}

#if EE_PLATFORM != EE_PLATFORM_MACOS && EE_PLATFORM != EE_PLATFORM_IOS && \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	mWindow.WindowConfig.Width *= mWindow.WindowConfig.PixelDensity;
	mWindow.WindowConfig.Height *= mWindow.WindowConfig.PixelDensity;
#endif

	mSDLWindow = SDL_CreateWindow( mWindow.WindowConfig.Title.c_str(), SDL_WINDOWPOS_UNDEFINED,
								   SDL_WINDOWPOS_UNDEFINED, mWindow.WindowConfig.Width,
								   mWindow.WindowConfig.Height, tmpFlags );

	if ( NULL == mSDLWindow ) {
		Log::error( "Unable to create window: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS
	Log::notice( "Choosing GL Version from: %d", Context.Version );

	if ( GLv_default != Context.Version ) {
		if ( GLv_ES1 == Context.Version || GLv_2 == Context.Version ) {
			if ( GLv_2 == Context.Version )
				mWindow.ContextConfig.Version = GLv_ES1;

			Log::notice( "Starting GLES1" );

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
		} else {
			Log::notice( "Starting GLES2" );

			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
			SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
		}
	} else {
#if defined( EE_GLES2 )
		Log::notice( "Starting GLES2 default" );

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 0 );
#else
		Log::notice( "Starting GLES1 default" );

		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 1 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
#endif
	}
#else
	if ( GLv_3CP == Context.Version ) {
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
	}
#endif

#ifdef SDL2_THREADED_GLCONTEXT
	if ( mWindow.ContextConfig.SharedGLContext ) {
		SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

		mGLContextThread = SDL_GL_CreateContext( mSDLWindow );
		mGLContext = SDL_GL_CreateContext( mSDLWindow );
	} else {
		mGLContext = SDL_GL_CreateContext( mSDLWindow );
	}
#else
	mGLContext = SDL_GL_CreateContext( mSDLWindow );
	mWindow.ContextConfig.SharedGLContext = false;
#endif

	if ( NULL == mGLContext
#ifdef SDL2_THREADED_GLCONTEXT
		 || ( mWindow.ContextConfig.SharedGLContext && NULL == mGLContextThread )
#endif
	) {
		Log::error( "Unable to create context: %s", SDL_GetError() );

		logFailureInit( "WindowSDL", getVersion() );

		return false;
	}

	// In some platforms it will not create the desired window size, so we query the real window
	// size created
	int w, h;
	SDL_GL_GetDrawableSize( mSDLWindow, &w, &h );

	if ( w > 0 && h > 0 ) {
		mWindow.WindowConfig.Width = w;
		mWindow.WindowConfig.Height = h;
		mWindow.WindowSize = Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );
		mLastWindowedSize = mWindow.WindowSize;
	} else {
		Log::error( "Window failed to create!" );

		if ( NULL != SDL_GetError() ) {
			Log::error( "SDL Error: %s", SDL_GetError() );
		}

		return false;
	}

	SDL_GL_SetSwapInterval( ( mWindow.ContextConfig.VSync ? 1 : 0 ) ); // VSync

	SDL_GL_MakeCurrent( mSDLWindow, mGLContext );

	mID = SDL_GetWindowID( mSDLWindow );

#ifdef EE_USE_WMINFO
	mWMinfo = eeNew( WMInfo, ( mSDLWindow ) );
#endif

	if ( NULL == Renderer::existsSingleton() ) {
		Renderer::createSingleton( mWindow.ContextConfig.Version );
		Renderer::instance()->init();
		if ( mWindow.ContextConfig.Multisamples > 0 )
			Renderer::instance()->multisample( true );
	}

	getMainContext();

	setTitle( mWindow.WindowConfig.Title );

	createView();

	setup2D( false );

	mWindow.Created = true;

	if ( "" != mWindow.WindowConfig.Icon ) {
		setIcon( mWindow.WindowConfig.Icon );
	}

	/// Init the clipboard after the window creation
	static_cast<ClipboardSDL*>( mClipboard )->init();

	/// Init the input after the window creation
	static_cast<InputSDL*>( mInput )->init();

	mCursorManager->set( Cursor::SysArrow );

#if EE_PLATFORM == EE_PLATFORM_WIN
	setUserTheme( (HWND)getWindowHandler() );
#endif

	logSuccessfulInit( getVersion() );

	return true;
}

Uint32 WindowSDL::getWindowID() const {
	return mID;
}

void WindowSDL::makeCurrent() {
	SDL_GL_MakeCurrent( mSDLWindow, mGLContext );
}

void WindowSDL::close() {
	SDL_DestroyWindow( mSDLWindow );
	mSDLWindow = NULL;
	mGLContext = NULL;
	mGLContextThread = NULL;
	Window::close();
}

void WindowSDL::setCurrent() {
	makeCurrent();
}

bool WindowSDL::isThreadedGLContext() const {
#ifdef SDL2_THREADED_GLCONTEXT
	return mWindow.ContextConfig.SharedGLContext;
#else
	return false;
#endif
}

void WindowSDL::setGLContextThread() {
	mGLContextMutex.lock();
	SDL_GL_MakeCurrent( mSDLWindow, mGLContextThread );
}

void WindowSDL::unsetGLContextThread() {
	SDL_GL_MakeCurrent( mSDLWindow, NULL );
	mGLContextMutex.unlock();
}

int WindowSDL::getCurrentDisplayIndex() const {
	int index = SDL_GetWindowDisplayIndex( mSDLWindow );

	if ( index < 0 ) {
		Log::error( SDL_GetError() );
		return 0;
	}

	return index;
}

std::string WindowSDL::getVersion() {
	SDL_version ver;

	SDL_GetVersion( &ver );

	return String::format( "SDL %d.%d.%d", ver.major, ver.minor, ver.patch );
}

void WindowSDL::setGLConfig() {
	if ( mWindow.ContextConfig.DepthBufferSize )
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, mWindow.ContextConfig.DepthBufferSize ); // Depth
	SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER,
						 ( mWindow.ContextConfig.DoubleBuffering ? 1 : 0 ) ); // Double Buffering
	if ( mWindow.ContextConfig.StencilBufferSize )
		SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, mWindow.ContextConfig.StencilBufferSize );

	if ( mWindow.WindowConfig.BitsPerPixel == 16 ) {
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 4 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 4 );
	} else {
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
	}
}

void WindowSDL::toggleFullscreen() {
	Log::info( "toggleFullscreen called: %s", isWindowed() ? "is windowed" : "is fullscreen" );

	if ( isWindowed() ) {
		mWinPos = getPosition();
		mWindow.Maximized = isMaximized();
	}

	SDL_SetWindowFullscreen( mSDLWindow, isWindowed() ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0 );

	BitOp::setBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen,
							isWindowed() ? 1 : 0 );

	getCursorManager()->reload();

	if ( isWindowed() ) {
		setPosition( mWinPos.x, mWinPos.y );

		if ( mWindow.Maximized )
			maximize();
	}
}

void WindowSDL::setTitle( const std::string& title ) {
	if ( mWindow.WindowConfig.Title != title ) {
		mWindow.WindowConfig.Title = title;

		SDL_SetWindowTitle( mSDLWindow, title.c_str() );
	}
}

bool WindowSDL::isActive() const {
	Uint32 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( ( flags & SDL_WINDOW_INPUT_FOCUS ) && ( flags & SDL_WINDOW_MOUSE_FOCUS ) );
}

bool WindowSDL::isVisible() const {
	Uint32 flags = SDL_GetWindowFlags( mSDLWindow );
	return 0 != ( ( flags & SDL_WINDOW_SHOWN ) && !( flags & SDL_WINDOW_MINIMIZED ) );
}

bool WindowSDL::hasFocus() const {
	return 0 != ( SDL_GetWindowFlags( mSDLWindow ) &
				  ( SDL_WINDOW_INPUT_FOCUS | SDL_WINDOW_MOUSE_FOCUS ) );
}

bool WindowSDL::hasInputFocus() const {
	return 0 != ( SDL_GetWindowFlags( mSDLWindow ) & ( SDL_WINDOW_INPUT_FOCUS ) );
}

bool WindowSDL::hasMouseFocus() const {
	return 0 != ( SDL_GetWindowFlags( mSDLWindow ) & ( SDL_WINDOW_MOUSE_FOCUS ) );
}

void WindowSDL::onWindowResize( Uint32 width, Uint32 height ) {
	if ( width == mWindow.WindowConfig.Width && height == mWindow.WindowConfig.Height )
		return;

	Log::debug( "onWindowResize: Width %d Height %d.", width, height );

	mWindow.WindowConfig.Width = width;
	mWindow.WindowConfig.Height = height;
	mWindow.WindowSize = Sizei( width, height );

	if ( isWindowed() )
		mLastWindowedSize = Sizei( width, height );

	mDefaultView.reset( Rectf( 0, 0, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height ) );

	setup2D( false );

	SDL_PumpEvents();

	SDL_FlushEvent( SDL_WINDOWEVENT );

	mCursorManager->reload();

	sendVideoResizeCb();
}

void WindowSDL::setSize( Uint32 width, Uint32 height, bool windowed ) {
	if ( ( !width || !height ) ) {
		width = mWindow.DesktopResolution.getWidth();
		height = mWindow.DesktopResolution.getHeight();
	}

	if ( this->isWindowed() == windowed && width == mWindow.WindowConfig.Width &&
		 height == mWindow.WindowConfig.Height )
		return;

	Log::debug( "Switching from %s to %s. Width: %d Height %d.",
				this->isWindowed() ? "windowed" : "fullscreen",
				windowed ? "windowed" : "fullscreen", width, height );

	Uint32 oldWidth = mWindow.WindowConfig.Width;
	Uint32 oldHeight = mWindow.WindowConfig.Height;

	mWindow.WindowConfig.Width = width;
	mWindow.WindowConfig.Height = height;

	if ( windowed ) {
		mWindow.WindowSize = Sizei( width, height );
	} else {
		mWindow.WindowSize = Sizei( oldWidth, oldHeight );
	}

	if ( isWindowed() && !windowed ) {
		mWinPos = getPosition();
	} else {
		SDL_SetWindowFullscreen( mSDLWindow, windowed ? 0 : SDL_WINDOW_FULLSCREEN );
	}

	if ( windowed )
		mLastWindowedSize = Sizei( width, height );

	SDL_SetWindowSize( mSDLWindow, width, height );

	if ( isWindowed() && !windowed ) {
		mWinPos = getPosition();

		setGLConfig();

		SDL_SetWindowFullscreen( mSDLWindow, windowed ? 0 : SDL_WINDOW_FULLSCREEN );
	}

	if ( isWindowed() && windowed ) {
		setPosition( mWinPos.x, mWinPos.y );
	}

	BitOp::setBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !windowed );

	mDefaultView.reset( Rectf( 0, 0, width, height ) );

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
		int displayModes = SDL_GetNumDisplayModes( x );

		for ( int i = 0; i < displayModes; i++ ) {
			SDL_DisplayMode mode;
			SDL_GetDisplayMode( x, i, &mode );

			result.push_back( DisplayMode( mode.w, mode.h, mode.refresh_rate, x ) );
		}
	}

	return result;
}

void WindowSDL::setGamma( Float Red, Float Green, Float Blue ) {
	eeclamp( &Red, (Float)0.1f, (Float)10.0f );
	eeclamp( &Green, (Float)0.1f, (Float)10.0f );
	eeclamp( &Blue, (Float)0.1f, (Float)10.0f );

	Uint16 red_ramp[256];
	Uint16 green_ramp[256];
	Uint16 blue_ramp[256];

	SDL_CalculateGammaRamp( Red, red_ramp );

	if ( Green == Red ) {
		SDL_memcpy( green_ramp, red_ramp, sizeof( red_ramp ) );
	} else {
		SDL_CalculateGammaRamp( Green, green_ramp );
	}

	if ( Blue == Red ) {
		SDL_memcpy( blue_ramp, red_ramp, sizeof( red_ramp ) );
	} else if ( Blue == Green ) {
		SDL_memcpy( blue_ramp, green_ramp, sizeof( green_ramp ) );
	} else {
		SDL_CalculateGammaRamp( Blue, blue_ramp );
	}

	SDL_SetWindowGammaRamp( mSDLWindow, red_ramp, green_ramp, blue_ramp );
}

eeWindowHandle WindowSDL::getWindowHandler() const {
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
		if ( Image::getInfo( Path.c_str(), &x, &y, &c ) ) {
			mWindow.WindowConfig.Icon = Path;

			return true;
		}

		return false;
	}

	Image Img( Path );

	if ( NULL != Img.getPixelsPtr() ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( Img.getWidth() > 64 || Img.getHeight() > 64 ) {
			Img.resize( 64, 64 );
		}
#endif
		const Uint8* Ptr = Img.getPixelsPtr();
		x = Img.getWidth();
		y = Img.getHeight();
		c = Img.getChannels();

		if ( ( x % 8 ) == 0 && ( y % 8 ) == 0 ) {
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
			SDL_Surface* TempGlyphSheet =
				SDL_CreateRGBSurface( SDL_SWSURFACE, x, y, c * 8, rmask, gmask, bmask, amask );

			SDL_LockSurface( TempGlyphSheet );

			Uint32 ssize = TempGlyphSheet->w * TempGlyphSheet->h * c;

			for ( Uint32 i = 0; i < ssize; i++ ) {
				( static_cast<Uint8*>( TempGlyphSheet->pixels ) )[i + 0] = ( Ptr )[i];
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

bool WindowSDL::isMaximized() const {
	return SDL_GetWindowFlags( mSDLWindow ) & SDL_WINDOW_MAXIMIZED;
}

bool WindowSDL::isMinimized() const {
	return SDL_GetWindowFlags( mSDLWindow ) & SDL_WINDOW_MINIMIZED;
}

void WindowSDL::hide() {
	SDL_HideWindow( mSDLWindow );
}

void WindowSDL::raise() {
	SDL_RaiseWindow( mSDLWindow );
}

void WindowSDL::restore() {
	SDL_RestoreWindow( mSDLWindow );
}

void WindowSDL::flash( WindowFlashOperation op ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#if SDL_VERSION_ATLEAST( 2, 0, 16 )
	SDL_FlashOperation sdlOp = SDL_FlashOperation::SDL_FLASH_BRIEFLY;
	if ( op == WindowFlashOperation::Cancel )
		sdlOp = SDL_FlashOperation::SDL_FLASH_CANCEL;
	else if ( op == WindowFlashOperation::UntilFocused )
		sdlOp = SDL_FlashOperation::SDL_FLASH_UNTIL_FOCUSED;
	SDL_FlashWindow( mSDLWindow, sdlOp );
#endif
#endif
}

void WindowSDL::show() {
	SDL_ShowWindow( mSDLWindow );
}

void WindowSDL::setPosition( int Left, int Top ) {
	SDL_SetWindowPosition( mSDLWindow, Left, Top );
}

Vector2i WindowSDL::getPosition() const {
	Vector2i p;

	SDL_GetWindowPosition( mSDLWindow, &p.x, &p.y );

	return p;
}

void WindowSDL::updateDesktopResolution() const {
	SDL_DisplayMode dpm;
	SDL_GetDesktopDisplayMode( SDL_GetWindowDisplayIndex( mSDLWindow ), &dpm );

	mWindow.DesktopResolution = Sizei( dpm.w, dpm.h );
}

const Sizei& WindowSDL::getDesktopResolution() const {
	updateDesktopResolution();
	return Window::getDesktopResolution();
}

Rect WindowSDL::getBorderSize() const {
	Rect bordersSize;
	SDL_GetWindowBordersSize( mSDLWindow, &bordersSize.Top, &bordersSize.Left, &bordersSize.Bottom,
							  &bordersSize.Right );
	return bordersSize;
}

Float WindowSDL::getScale() const {
	int realX, realY;
	int scaledX, scaledY;
	SDL_GL_GetDrawableSize( mSDLWindow, &realX, &realY );
	SDL_GetWindowSize( mSDLWindow, &scaledX, &scaledY );
	return (Float)realX / (Float)scaledX;
}

bool WindowSDL::hasNativeMessageBox() const {
	return true;
}

Uint32 toSDLMsgBoxType( const Window::MessageBoxType& type ) {
	switch ( type ) {
		case Window::MessageBoxType::Error:
			return SDL_MESSAGEBOX_ERROR;
		case Window::MessageBoxType::Warning:
			return SDL_MESSAGEBOX_WARNING;
		case Window::MessageBoxType::Information:
			return SDL_MESSAGEBOX_INFORMATION;
	}
	return SDL_MESSAGEBOX_INFORMATION;
}

bool WindowSDL::showMessageBox( const MessageBoxType& type, const std::string& title,
								const std::string& message ) {
	return 0 == SDL_ShowSimpleMessageBox( toSDLMsgBoxType( type ), title.c_str(), message.c_str(),
										  mSDLWindow );
}

SDL_Window* WindowSDL::getSDLWindow() const {
	return mSDLWindow;
}

void WindowSDL::startOnScreenKeyboard() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	showOSK( getWindowHandler() );
#elif defined( EE_X11_PLATFORM )
	showOSK();
#endif
}

void WindowSDL::stopOnScreenKeyboard() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	hideOSK();
#elif defined( EE_X11_PLATFORM )
	hideOSK();
#endif
}

bool WindowSDL::isOnScreenKeyboardActive() const {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return WIN_OSK_VISIBLE;
#elif defined( EE_X11_PLATFORM )
	return ONBOARD_PID != 0;
#else
	return false;
#endif
}

void WindowSDL::startTextInput() {
	if ( mWindow.WindowConfig.UseScreenKeyboard ) {
		startOnScreenKeyboard();
	} else {
		SDL_StartTextInput();
	}
}

bool WindowSDL::isTextInputActive() const {
	if ( mWindow.WindowConfig.UseScreenKeyboard )
		return isOnScreenKeyboardActive();
	return SDL_TRUE == SDL_IsTextInputActive();
}

void WindowSDL::stopTextInput() {
	if ( mWindow.WindowConfig.UseScreenKeyboard ) {
		stopOnScreenKeyboard();
	} else {
		SDL_StopTextInput();
	}
}

void WindowSDL::setTextInputRect( const Rect& rect ) {
	SDL_Rect r;
	r.x = rect.Left;
	r.y = rect.Top;
	r.w = rect.getSize().getWidth();
	r.h = rect.getSize().getHeight();

	SDL_SetTextInputRect( &r );
}

void WindowSDL::clearComposition() {
#if SDL_VERSION_ATLEAST( 2, 0, 22 )
	SDL_ClearComposition();
#endif
}

bool WindowSDL::hasScreenKeyboardSupport() const {
	return SDL_TRUE == SDL_HasScreenKeyboardSupport();
}

bool WindowSDL::isScreenKeyboardShown() const {
	return SDL_TRUE == SDL_IsScreenKeyboardShown( mSDLWindow );
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
