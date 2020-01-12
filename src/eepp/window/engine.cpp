#include <eepp/graphics/fontmanager.hpp>
#include <eepp/graphics/framebuffermanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/ninepatchmanager.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/shaderprogrammanager.hpp>
#include <eepp/graphics/textureatlasmanager.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/vertexbuffermanager.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/physics/physicsmanager.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/backendsdl2.hpp>
#include <eepp/window/backend/SDL2/platformhelpersdl2.hpp>
#include <eepp/window/engine.hpp>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <eepp/system/zip.hpp>
#endif

#define BACKEND_SDL2 1

#ifndef DEFAULT_BACKEND

#if defined( EE_BACKEND_SDL2 )
#define DEFAULT_BACKEND BACKEND_SDL2
#endif

#endif

namespace EE { namespace Window {

SINGLETON_DECLARE_IMPLEMENTATION( Engine )

Engine::Engine() :
	mBackend( NULL ),
	mWindow( NULL ),
	mSharedGLContext( true ),
	mMainThreadId( 0 ),
	mPlatformHelper( NULL ),
	mZip( NULL ),
	mDisplayManager( NULL ) {
#if EE_PLATFORM == EE_PLATFORM_ANDROID
	mZip = Zip::New();
	mZip->open( getPlatformHelper()->getApkPath() );

	FileSystem::changeWorkingDirectory( getPlatformHelper()->getExternalStoragePath() );
#else
	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );
#endif

	TextureAtlasManager::createSingleton();
}

Engine::~Engine() {
	Physics::PhysicsManager::destroySingleton();

	GlobalBatchRenderer::destroySingleton();

	TextureAtlasManager::destroySingleton();

	NinePatchManager::destroySingleton();

	Scene::SceneManager::destroySingleton();

	CSS::StyleSheetSpecification::destroySingleton();

	FontManager::destroySingleton();

	TextureFactory::destroySingleton();

	Graphics::Renderer::destroySingleton();

	ShaderProgramManager::destroySingleton();

	PackManager::destroySingleton();

	Graphics::Private::FrameBufferManager::destroySingleton();

	Graphics::Private::VertexBufferManager::destroySingleton();

	VirtualFileSystem::destroySingleton();

	Log::destroySingleton();

#ifdef EE_SSL_SUPPORT
	Network::SSL::SSLSocket::end();
#endif

	Network::Http::Pool::getGlobal().clear();

	destroy();

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	eeSAFE_DELETE( mZip );
#endif

	eeSAFE_DELETE( mPlatformHelper );

	eeSAFE_DELETE( mDisplayManager );

	eeSAFE_DELETE( mBackend );
}

void Engine::destroy() {
	std::list<Window*>::iterator it;

	for ( it = mWindows.begin(); it != mWindows.end(); ++it ) {
		eeSAFE_DELETE( *it );
	}

	mWindow = NULL;
}

Backend::WindowBackend* Engine::createSDL2Backend( const WindowSettings& Settings ) {
#if defined( EE_SDL_VERSION_2 )
	return eeNew( Backend::SDL2::WindowBackendSDL2, () );
#else
	return NULL;
#endif
}

EE::Window::Window* Engine::createSDL2Window( const WindowSettings& Settings,
											  const ContextSettings& Context ) {
#if defined( EE_SDL_VERSION_2 )
	if ( NULL == mBackend ) {
		mBackend = createSDL2Backend( Settings );
	}

	return eeNew( Backend::SDL2::WindowSDL, ( Settings, Context ) );
#else
	return NULL;
#endif
}

EE::Window::Window* Engine::createDefaultWindow( const WindowSettings& Settings,
												 const ContextSettings& Context ) {
#if DEFAULT_BACKEND == BACKEND_SDL2
	return createSDL2Window( Settings, Context );
#endif
}

EE::Window::Window* Engine::createWindow( WindowSettings Settings, ContextSettings Context ) {
	EE::Window::Window* window = NULL;

	if ( NULL != mWindow ) {
		Settings.Backend = mWindow->getWindowInfo()->WindowConfig.Backend;
	} else {
		mMainThreadId = Thread::getCurrentThreadId();
	}

	switch ( Settings.Backend ) {
		case WindowBackend::Default:
		default:
			window = createDefaultWindow( Settings, Context );
			break;
	}

	if ( NULL == window ) {
		window = createDefaultWindow( Settings, Context );
	}

	setCurrentWindow( window );

	mWindows.push_back( mWindow );

	PixelDensity::setPixelDensity( Settings.PixelDensity );

	return window;
}

void Engine::destroyWindow( EE::Window::Window* window ) {
	mWindows.remove( window );

	if ( window == mWindow ) {
		if ( mWindows.size() > 0 ) {
			mWindow = mWindows.back();
		} else {
			mWindow = NULL;
		}
	}

	eeSAFE_DELETE( window );
}

bool Engine::existsWindow( EE::Window::Window* window ) {
	std::list<Window*>::iterator it;

	for ( it = mWindows.begin(); it != mWindows.end(); ++it ) {
		if ( ( *it ) == window )
			return true;
	}

	return false;
}

EE::Window::Window* Engine::getCurrentWindow() const {
	return mWindow;
}

void Engine::setCurrentWindow( EE::Window::Window* window ) {
	if ( NULL != window && window != mWindow ) {
		mWindow = window;

		mWindow->setCurrent();
	}
}

Uint32 Engine::getWindowCount() const {
	return mWindows.size();
}

bool Engine::isRunning() const {
	return NULL != mWindow;
}

Uint32 Engine::getDefaultBackend() const {
#if DEFAULT_BACKEND == BACKEND_SDL2
	return WindowBackend::SDL2;
#endif
}

WindowSettings Engine::createWindowSettings( IniFile* ini, std::string iniKeyName ) {
	eeASSERT( NULL != ini );

	ini->readFile();

	int Width = ini->getValueI( iniKeyName, "Width", 800 );
	int Height = ini->getValueI( iniKeyName, "Height", 600 );
	int BitColor = ini->getValueI( iniKeyName, "BitColor", 32 );
	bool Windowed = ini->getValueB( iniKeyName, "Windowed", true );
	bool Resizeable = ini->getValueB( iniKeyName, "Resizeable", true );
	bool Borderless = ini->getValueB( iniKeyName, "Borderless", false );
	bool UseDesktopResolution = ini->getValueB( iniKeyName, "UseDesktopResolution", false );
	std::string pixelDensityStr = ini->getValue( iniKeyName, "PixelDensity" );
	float pixelDensity = PixelDensity::getPixelDensity();
	bool useScreenKeyboard = ini->getValueB( iniKeyName, "UseScreenKeyboard" );

	if ( !pixelDensityStr.empty() ) {
		if ( String::toLower( pixelDensityStr ) == "auto" ) {
			Display* currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex( 0 );
			pixelDensity = PixelDensity::toFloat( currentDisplay->getPixelDensity() );
		} else {
			float pd = 1;
			bool res = String::fromString<float>( pd, pixelDensityStr );

			if ( res )
				pixelDensity = pd;
		}
	}

	std::string Backend = ini->getValue( iniKeyName, "Backend", "" );
	Uint32 WinBackend = getDefaultBackend();

	String::toLowerInPlace( Backend );

	if ( "sdl2" == Backend )
		WinBackend = WindowBackend::SDL2;

	Uint32 Style = WindowStyle::Titlebar;

	if ( Borderless )
		Style = WindowStyle::Borderless;

	if ( UseDesktopResolution )
		Style |= WindowStyle::UseDesktopResolution;

	if ( !Windowed )
		Style |= WindowStyle::Fullscreen;

	if ( Resizeable )
		Style |= WindowStyle::Resize;

	std::string Icon = ini->getValue( iniKeyName, "WinIcon", "" );
	std::string Caption = ini->getValue( iniKeyName, "WinCaption", "" );

	WindowSettings WinSettings( Width, Height, Caption, Style, WinBackend, BitColor, Icon,
								pixelDensity, useScreenKeyboard );

	return WinSettings;
}

WindowSettings Engine::createWindowSettings( std::string iniPath, std::string iniKeyName ) {
	IniFile Ini( iniPath );

	return createWindowSettings( &Ini, iniKeyName );
}

ContextSettings Engine::createContextSettings( IniFile* ini, std::string iniKeyName ) {
	eeASSERT( NULL != ini );

	ini->readFile();

	bool VSync = ini->getValueB( iniKeyName, "VSync", true );
	std::string GLVersion = ini->getValue( iniKeyName, "GLVersion", "0" );

	String::toLowerInPlace( GLVersion );

	EEGL_version GLVer;
	if ( "3" == GLVersion || "opengl 3" == GLVersion || "gl3" == GLVersion ||
		 "opengl3" == GLVersion )
		GLVer = GLv_3;
	else if ( "4" == GLVersion || "opengl es 2" == GLVersion || "gles2" == GLVersion ||
			  "opengles2" == GLVersion || "es2" == GLVersion )
		GLVer = GLv_ES2;
	else if ( "5" == GLVersion || "opengl 3 core profile" == GLVersion || "gl3cp" == GLVersion ||
			  "opengl3cp" == GLVersion || "opengl core profile" == GLVersion ||
			  "core profile" == GLVersion || "cp" == GLVersion )
		GLVer = GLv_3CP;
	else if ( "opengl es 1" == GLVersion || "gles1" == GLVersion || "gl es 1" == GLVersion ||
			  "opengl es1" == GLVersion || "opengles1" == GLVersion || "es1" == GLVersion ||
			  "gles 1" == GLVersion )
		GLVer = GLv_ES1;
	else if ( "2" == GLVersion || "opengl 2" == GLVersion || "gl2" == GLVersion ||
			  "gl 2" == GLVersion )
		GLVer = GLv_2;
	else
		GLVer = GLv_default;

	int depthBufferSize = ini->getValueI( iniKeyName, "DepthBufferSize", 24 );
	int stencilBufferSize = ini->getValueI( iniKeyName, "StencilBufferSize", 1 );
	int multisamples = ini->getValueI( iniKeyName, "Multisamples", 0 );
	int frameRateLimit = ini->getValueI( iniKeyName, "FrameRateLimit", 0 );
	bool doubleBuffering = ini->getValueB( iniKeyName, "DoubleBuffering", true );
	bool sharedGLContext = ini->getValueB( iniKeyName, "SharedGLContext", false );

	return ContextSettings( VSync, GLVer, doubleBuffering, depthBufferSize, stencilBufferSize,
							multisamples, sharedGLContext, frameRateLimit );
}

ContextSettings Engine::createContextSettings( std::string iniPath, std::string iniKeyName ) {
	IniFile Ini( iniPath );

	return createContextSettings( &Ini, iniKeyName );
}

void Engine::enableSharedGLContext() {
	mSharedGLContext = true;
}

void Engine::disableSharedGLContext() {
	mSharedGLContext = false;
}

bool Engine::isSharedGLContextEnabled() {
	return mSharedGLContext && mWindow->isThreadedGLContext();
}

Uint32 Engine::getMainThreadId() {
	return mMainThreadId;
}

PlatformHelper* Engine::getPlatformHelper() {
	if ( NULL == mPlatformHelper ) {
#if DEFAULT_BACKEND == BACKEND_SDL2
		mPlatformHelper = eeNew( Backend::SDL2::PlatformHelperSDL2, () );
#endif
	}

	return mPlatformHelper;
}

DisplayManager* Engine::getDisplayManager() {
	if ( NULL == mDisplayManager ) {
#if DEFAULT_BACKEND == BACKEND_SDL2
		mDisplayManager = eeNew( Backend::SDL2::DisplayManagerSDL2, () );
#endif
	}

	return mDisplayManager;
}

struct EngineInitializer {
	EngineInitializer() { Engine::createSingleton(); }

	~EngineInitializer() { Engine::destroySingleton(); }
};

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS

#else

extern "C" int EE_SDL_main( int argc, char* argv[] );

extern "C" int SDL_main( int argc, char* argv[] ) {
	EngineInitializer engineInitializer;
	return EE_SDL_main( argc, argv );
}

#endif

}} // namespace EE::Window
