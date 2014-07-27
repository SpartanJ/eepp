#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
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
#include <eepp/graphics/renderer/gl.hpp>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <eepp/system/zip.hpp>
#include <jni.h>

static std::string SDL_AndroidGetApkPath() {
	static std::string apkPath = "";

	if ( "" == apkPath ) {
		jmethodID mid;
		jobject context;
		jobject fileObject;
		const char *path;

		JNIEnv *env = (JNIEnv*)SDL_AndroidGetJNIEnv();

		jclass ActivityClass = env->GetObjectClass((jobject)SDL_AndroidGetActivity());

		// context = SDLActivity.getContext();
		mid = env->GetStaticMethodID(ActivityClass,"getContext","()Landroid/content/Context;");

		context = env->CallStaticObjectMethod(ActivityClass, mid);

		// fileObj = context.getFilesDir();
		mid = env->GetMethodID(env->GetObjectClass(context),"getPackageCodePath", "()Ljava/lang/String;");

		fileObject = env->CallObjectMethod(context, mid);

		jboolean isCopy;
		path = env->GetStringUTFChars((jstring)fileObject, &isCopy);

		apkPath = std::string( path );
	}

	return apkPath;
}
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS || EE_PLATFORM == EE_PLATFORM_ANDROID
#define SDL2_THREADED_GLCONTEXT
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

WindowSDL::WindowSDL( WindowSettings Settings, ContextSettings Context ) :
	Window( Settings, Context, eeNew( ClipboardSDL, ( this ) ), eeNew( InputSDL, ( this ) ), eeNew( CursorManagerSDL, ( this ) ) ),
	mSDLWindow( NULL ),
	mGLContext( NULL ),
	mGLContextThread( NULL ),
	mWMinfo( NULL )
#if EE_PLATFORM == EE_PLATFORM_ANDROID
	,
	mZip( eeNew( Zip, () ) )
#endif
{
	Create( Settings, Context );
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

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	eeSAFE_DELETE( mZip );
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

	SDL_DisplayMode dpm;
	SDL_GetDesktopDisplayMode( 0, &dpm );

	mWindow.DesktopResolution = Sizei( dpm.w, dpm.h );

	#if EE_PLATFORM == EE_PLATFORM_ANDROID
		mWindow.WindowConfig.Style = WindowStyle::Fullscreen | WindowStyle::UseDesktopResolution;
	#endif

	if ( mWindow.WindowConfig.Style & WindowStyle::UseDesktopResolution ) {
		mWindow.WindowConfig.Width	= mWindow.DesktopResolution.Width();
		mWindow.WindowConfig.Height	= mWindow.DesktopResolution.Height();
	}

	mWindow.Flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	if ( mWindow.WindowConfig.Style & WindowStyle::Resize ) {
		mWindow.Flags |= SDL_WINDOW_RESIZABLE;
	}

	if ( mWindow.WindowConfig.Style & WindowStyle::NoBorder ) {
		mWindow.Flags |= SDL_WINDOW_BORDERLESS;
	}

	SetGLConfig();

	Uint32 mTmpFlags = mWindow.Flags;

	if ( mWindow.WindowConfig.Style & WindowStyle::Fullscreen ) {
		mTmpFlags |= SDL_WINDOW_FULLSCREEN;
	}

	mSDLWindow = SDL_CreateWindow( mWindow.WindowConfig.Caption.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWindow.WindowConfig.Width, mWindow.WindowConfig.Height, mTmpFlags );

	if ( NULL == mSDLWindow ) {
		eePRINTL( "Unable to create window: %s", SDL_GetError() );

		LogFailureInit( "WindowSDL", GetVersion() );

		return false;
	}

	/// In some platforms it will not create the desired window size, so we query the real window size created
	int w, h;
	SDL_GetWindowSize( mSDLWindow, &w, &h );

	mWindow.WindowConfig.Width	= w;
	mWindow.WindowConfig.Height	= h;
	mWindow.WindowSize			= Sizei( mWindow.WindowConfig.Width, mWindow.WindowConfig.Height );

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
			#if defined( EE_GLES2 ) && !defined( EE_GLES1 )
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
		SDL_GL_SetAttribute( SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1 );

		mGLContext			= SDL_GL_CreateContext( mSDLWindow );
		mGLContextThread	= SDL_GL_CreateContext( mSDLWindow );
	#else
		mGLContext			= SDL_GL_CreateContext( mSDLWindow );
	#endif

	if ( NULL == mGLContext
	#ifdef SDL2_THREADED_GLCONTEXT
		 || NULL == mGLContextThread
	#endif
	)
	{
		eePRINTL( "Unable to create context: %s", SDL_GetError() );

		LogFailureInit( "WindowSDL", GetVersion() );

		return false;
	}

	SDL_GL_SetSwapInterval( ( mWindow.ContextConfig.VSync ? 1 : 0 ) );								// VSync

	SDL_GL_MakeCurrent( mSDLWindow, mGLContext );

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

	if ( "" != mWindow.WindowConfig.Icon ) {
		Icon( mWindow.WindowConfig.Icon );
	}

	/// Init the clipboard after the window creation
	reinterpret_cast<ClipboardSDL*> ( mClipboard )->Init();

	/// Init the input after the window creation
	reinterpret_cast<InputSDL*> ( mInput )->Init();

	mCursorManager->Set( SYS_CURSOR_ARROW );

	#if EE_PLATFORM == EE_PLATFORM_ANDROID
	std::string apkPath( SDL_AndroidGetApkPath() );

	eePRINTL( "Opening application APK in: %s", apkPath.c_str() );

	if ( mZip->Open( apkPath ) )
		eePRINTL( "APK opened succesfully!" );
	else
		eePRINTL( "Failed to open APK!" );

	LogSuccessfulInit( GetVersion(), apkPath );
	#else
	LogSuccessfulInit( GetVersion() );
	#endif

	return true;
}

bool WindowSDL::IsThreadedGLContext() {
#ifdef SDL2_THREADED_GLCONTEXT
	return true;
#else
	return false;
#endif
}

void WindowSDL::SetGLContextThread() {
	SDL_GL_MakeCurrent( mSDLWindow, mGLContextThread );
}

void WindowSDL::UnsetGLContextThread() {
	SDL_GL_MakeCurrent( mSDLWindow, NULL );
}

std::string WindowSDL::GetVersion() {
	SDL_version ver;

	SDL_GetVersion( &ver );

	return String::StrFormated( "SDL %d.%d.%d", ver.major, ver.minor, ver.patch );
}

void WindowSDL::CreatePlatform() {
	eeSAFE_DELETE( mPlatform );

#ifdef EE_USE_WMINFO
	mWMinfo = eeNew( WMInfo, ( mSDLWindow ) );
#endif

#if defined( EE_X11_PLATFORM )
	mPlatform = eeNew( Platform::cX11Impl, ( this, mWMinfo->GetWindowHandler(), mWMinfo->GetWindow(), mWMinfo->GetWindow(), NULL, NULL ) );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	mPlatform = eeNew( Platform::cWinImpl, ( this, GetWindowHandler() ) );
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	mPlatform = eeNew( Platform::cOSXImpl, ( this ) );
#else
	Window::CreatePlatform();
#endif
}

void WindowSDL::SetGLConfig() {
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

	SDL_SetWindowTitle( mSDLWindow, Caption.c_str() );
}

bool WindowSDL::Active() {
	Uint32 flags = 0;

	flags = SDL_GetWindowFlags( mSDLWindow );

	return 0 != ( ( flags & SDL_WINDOW_INPUT_FOCUS ) && ( flags & SDL_WINDOW_MOUSE_FOCUS ) );
}

bool WindowSDL::Visible() {
	Uint32 flags = 0;

	flags = SDL_GetWindowFlags( mSDLWindow );

	return 0 != ( ( flags & SDL_WINDOW_SHOWN ) && !( flags & SDL_WINDOW_MINIMIZED ) );
}

void WindowSDL::Size( Uint32 Width, Uint32 Height, bool Windowed ) {
	if ( ( !Width || !Height ) ) {
		Width	= mWindow.DesktopResolution.Width();
		Height	= mWindow.DesktopResolution.Height();
	}

	if ( this->Windowed() == Windowed && Width == mWindow.WindowConfig.Width && Height == mWindow.WindowConfig.Height )
		return;

	eePRINTL( "Switching from %s to %s. Width: %d Height %d.", this->Windowed() ? "windowed" : "fullscreen", Windowed ? "windowed" : "fullscreen", Width, Height );

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
	} else {
		SDL_SetWindowFullscreen( mSDLWindow, Windowed ? SDL_FALSE : SDL_TRUE );
	}

	SDL_SetWindowSize( mSDLWindow, Width, Height );

	if ( this->Windowed() && !Windowed ) {
		mWinPos = Position();

		SetGLConfig();

		SDL_SetWindowFullscreen( mSDLWindow, Windowed ? SDL_FALSE : SDL_TRUE );
	}

	if ( !this->Windowed() && Windowed ) {
		Position( mWinPos.x, mWinPos.y );
	}

	BitOp::SetBitFlagValue( &mWindow.WindowConfig.Style, WindowStyle::Fullscreen, !Windowed );

	mDefaultView.SetView( 0, 0, Width, Height );

	Setup2D();

	SDL_PumpEvents();

	SDL_FlushEvent( SDL_WINDOWEVENT );

	mCursorManager->Reload();

	SendVideoResizeCb();
}

void WindowSDL::SwapBuffers() {	
	SDL_GL_SwapWindow( mSDLWindow );
}

std::vector<DisplayMode> WindowSDL::GetDisplayModes() const {
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

void WindowSDL::SetGamma( Float Red, Float Green, Float Blue ) {
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

eeWindowHandle	WindowSDL::GetWindowHandler() {
	if ( NULL != mWMinfo ) {
		return mWMinfo->GetWindowHandler();
	}

	return 0;
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

			SDL_SetWindowIcon( mSDLWindow, TempGlyphSheet );

			SDL_FreeSurface( TempGlyphSheet );

			return true;
		}
	}

	return false;
}

void WindowSDL::Minimize() {
	SDL_MinimizeWindow( mSDLWindow );
}

void WindowSDL::Maximize() {
	SDL_MaximizeWindow( mSDLWindow );
}

void WindowSDL::Hide() {
	SDL_HideWindow( mSDLWindow );
}

void WindowSDL::Raise() {
	SDL_RaiseWindow( mSDLWindow );
}

void WindowSDL::Show() {
	SDL_ShowWindow( mSDLWindow );
}

void WindowSDL::Position( Int16 Left, Int16 Top ) {
	SDL_SetWindowPosition( mSDLWindow, Left, Top );
}

Vector2i WindowSDL::Position() {
	Vector2i p;

	SDL_GetWindowPosition( mSDLWindow, &p.x, &p.y );

	return p;
}

void WindowSDL::UpdateDesktopResolution() {
	SDL_DisplayMode dpm;
	SDL_GetDesktopDisplayMode( SDL_GetWindowDisplayIndex( mSDLWindow ), &dpm );

	mWindow.DesktopResolution = Sizei( dpm.w, dpm.h );
}

const Sizei& WindowSDL::GetDesktopResolution() {
	UpdateDesktopResolution();
	return Window::GetDesktopResolution();
}

SDL_Window * WindowSDL::GetSDLWindow() const {
	return mSDLWindow;
}

void WindowSDL::StartTextInput() {
	SDL_StartTextInput();
}

bool WindowSDL::IsTextInputActive() {
	return SDL_TRUE == SDL_IsTextInputActive();
}

void WindowSDL::StopTextInput() {
	SDL_StopTextInput();
}

void WindowSDL::SetTextInputRect( Recti& rect ) {
	SDL_Rect r;

	r.x = rect.Left;
	r.y = rect.Top;
	r.w = rect.Size().Width();
	r.h = rect.Size().Height();

	SDL_SetTextInputRect( &r );

	rect.Left	= r.x;
	rect.Top	= r.y;
	rect.Right	= rect.Left + r.w;
	rect.Bottom	= rect.Top + r.h;
}

bool WindowSDL::HasScreenKeyboardSupport() {
	return SDL_TRUE == SDL_HasScreenKeyboardSupport();
}

bool WindowSDL::IsScreenKeyboardShown() {
	return SDL_TRUE == SDL_IsScreenKeyboardShown( mSDLWindow );
}

#if EE_PLATFORM == EE_PLATFORM_ANDROID
void * WindowSDL::GetJNIEnv() {
	return SDL_AndroidGetJNIEnv();
}

void * WindowSDL::GetActivity() {
	return SDL_AndroidGetActivity();
}

int WindowSDL::GetExternalStorageState() {
	return SDL_AndroidGetExternalStorageState();
}

std::string WindowSDL::GetInternalStoragePath() {
	return std::string( SDL_AndroidGetInternalStoragePath() );
}

std::string WindowSDL::GetExternalStoragePath() {
	return std::string( SDL_AndroidGetExternalStoragePath() );
}

std::string WindowSDL::GetApkPath() {
	return SDL_AndroidGetApkPath();
}
#endif

}}}}

#endif
