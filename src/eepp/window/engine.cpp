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
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/parsermatcher.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
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
#endif

	TextureAtlasManager::createSingleton();
}

Engine::~Engine() {
	GlobalBatchRenderer::destroySingleton();

	TextureAtlasManager::destroySingleton();

	NinePatchManager::destroySingleton();

	Scene::SceneManager::destroySingleton();

	CSS::StyleSheetSpecification::destroySingleton();

	Doc::SyntaxDefinitionManager::destroySingleton();

	FontManager::destroySingleton();

	TextureFactory::destroySingleton();

	Graphics::Renderer::destroySingleton();

	ShaderProgramManager::destroySingleton();

	PackManager::destroySingleton();

	Graphics::Private::FrameBufferManager::destroySingleton();

	Graphics::Private::VertexBufferManager::destroySingleton();

	VirtualFileSystem::destroySingleton();

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

	RegExCache::destroySingleton();

	ParserMatcherManager::destroySingleton();

	Log::destroySingleton();
}

void Engine::destroy() {
	for ( auto& it : mWindows ) {
		eeSAFE_DELETE( it.second );
	}

	mWindows.clear();

	mWindow = NULL;
}

Backend::WindowBackendLibrary* Engine::createSDL2Backend( const WindowSettings& ) {
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
#else
	return NULL;
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

	mWindows.insert( { mWindow->getWindowID(), mWindow } );

	PixelDensity::setPixelDensity( Settings.PixelDensity );

	return window;
}

void Engine::destroyWindow( EE::Window::Window* window ) {
	mWindows.erase( window->getWindowID() );

	if ( window == mWindow ) {
		if ( mWindows.size() > 0 ) {
			mWindow = mWindows.begin()->second;
		} else {
			mWindow = NULL;
		}
	}

	eeSAFE_DELETE( window );
}

bool Engine::existsWindow( EE::Window::Window* window ) {
	for ( auto& it : mWindows ) {
		if ( it.second == window )
			return true;
	}

	return false;
}

void Engine::forEachWindow( std::function<void( EE::Window::Window* )> cb ) {
	for ( auto& it : mWindows )
		cb( it.second );
}

EE::Window::Window* Engine::getWindowID( const Uint32& winID ) {
	for ( auto& it : mWindows ) {
		if ( it.first == winID )
			return it.second;
	}
	return nullptr;
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

bool Engine::isEngineRunning() {
	return existsSingleton() && Engine::instance()->isRunning();
}

bool Engine::isRunninMainThread() {
	return isEngineRunning() && Engine::instance()->isMainThread();
}

bool Engine::isRunning() const {
	return NULL != mWindow;
}

WindowBackend Engine::getDefaultBackend() const {
#if DEFAULT_BACKEND == BACKEND_SDL2
	return WindowBackend::SDL2;
#else
	return WindowBackend::Default;
#endif
}

WindowSettings Engine::createWindowSettings( IniFile* ini, std::string iniKeyName ) {
	eeASSERT( NULL != ini );

	ini->readFile();

	int Width = ini->getValueI( iniKeyName, "width", 800 );
	int Height = ini->getValueI( iniKeyName, "height", 600 );
	int BitColor = ini->getValueI( iniKeyName, "bitcolor", 32 );
	bool Windowed = ini->getValueB( iniKeyName, "windowed", true );
	bool Resizeable = ini->getValueB( iniKeyName, "resizeable", true );
	bool Borderless = ini->getValueB( iniKeyName, "borderless", false );
	bool useDesktopResolution = ini->getValueB( iniKeyName, "usedesktopresolution", false );
	std::string pixelDensityStr = ini->getValue( iniKeyName, "pixeldensity" );
	float pixelDensity = PixelDensity::getPixelDensity();
	bool useScreenKeyboard =
		ini->getValueB( iniKeyName, "usescreenkeyboard", EE_SCREEN_KEYBOARD_ENABLED );

	if ( !pixelDensityStr.empty() ) {
		if ( String::toLower( pixelDensityStr ) == "auto" ) {
			Display* currentDisplay = Engine::instance()->getDisplayManager()->getDisplayIndex( 0 );
			pixelDensity = currentDisplay->getPixelDensity();
		} else {
			float pd = 1;
			bool res = String::fromString( pd, pixelDensityStr );

			if ( res )
				pixelDensity = pd;
		}
	}

	std::string backend = ini->getValue( iniKeyName, "backend", "" );
	WindowBackend winBackend = getDefaultBackend();

	String::toLowerInPlace( backend );

	if ( "sdl2" == backend )
		winBackend = WindowBackend::SDL2;

	Uint32 Style = WindowStyle::Titlebar;

	if ( Borderless )
		Style = WindowStyle::Borderless;

	if ( useDesktopResolution )
		Style |= WindowStyle::UseDesktopResolution;

	if ( !Windowed )
		Style |= WindowStyle::Fullscreen;

	if ( Resizeable )
		Style |= WindowStyle::Resize;

	std::string icon = ini->getValue( iniKeyName, "winicon", "" );
	std::string title = ini->getValue( iniKeyName, "wintitle", "" );

	WindowSettings WinSettings( Width, Height, title, Style, winBackend, BitColor, icon,
								pixelDensity, useScreenKeyboard );

	return WinSettings;
}

WindowSettings Engine::createWindowSettings( std::string iniPath, std::string iniKeyName ) {
	IniFile Ini( iniPath );

	return createWindowSettings( &Ini, iniKeyName );
}

ContextSettings Engine::createContextSettings( IniFile* ini, std::string iniKeyName,
											   bool vsyncEnabledByDefault ) {
	eeASSERT( NULL != ini );

	ini->readFile();

	bool VSync = ini->getValueB( iniKeyName, "vsync", vsyncEnabledByDefault );
	std::string GLVersion = ini->getValue( iniKeyName, "glversion", "0" );
	int depthBufferSize = ini->getValueI( iniKeyName, "depthbuffersize", 24 );
	int stencilBufferSize = ini->getValueI( iniKeyName, "stencilbuffersize", 1 );
	int multisamples = ini->getValueI( iniKeyName, "multisamples", 0 );
	int frameRateLimit = ini->getValueI( iniKeyName, "frameratelimit", 0 );
	bool doubleBuffering = ini->getValueB( iniKeyName, "doublebuffering", true );
	bool sharedGLContext = ini->getValueB( iniKeyName, "sharedglcontext", false );

	return ContextSettings( VSync, Renderer::glVersionFromString( GLVersion ), doubleBuffering,
							depthBufferSize, stencilBufferSize, multisamples, sharedGLContext,
							frameRateLimit );
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

bool Engine::isThreaded() {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN && !defined( __EMSCRIPTEN_PTHREADS__ )
	return false;
#else
	return true;
#endif
}

Uint32 Engine::getMainThreadId() {
	return mMainThreadId;
}

bool Engine::isMainThread() const {
	return Thread::getCurrentThreadId() == Engine::instance()->getMainThreadId();
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

bool Engine::openURI( const std::string& url ) {
	if ( nullptr == getPlatformHelper() )
		return false;

	if ( !LuaPattern::hasMatches( url, "^%w+://" ) )
		return openURI( "file://" + url );

	if ( String::startsWith( url, "file://" ) ) {
		std::string absolutePath( FileSystem::getCurrentWorkingDirectory() );
		FileSystem::dirAddSlashAtEnd( absolutePath );
		if ( "Windows" != Sys::getPlatform() ) {
			if ( !String::startsWith( url, "file:///" ) ) {
				absolutePath += url.substr( 7 );
				getPlatformHelper()->openURL( "file://" + absolutePath );
			}
		} else {
			std::string relativePath( url.substr( 7 ) );
			if ( ( relativePath.size() == 1 ) ||
				 ( relativePath.size() >= 2 && relativePath[1] != ':' ) ) {
				absolutePath += relativePath;
				getPlatformHelper()->openURL( "file://" + absolutePath );
			}
		}
	}

	return getPlatformHelper()->openURL( url );
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
