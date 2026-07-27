#include <eepp/graphics/framebufferregistry.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/resourcescope.hpp>
#include <eepp/graphics/shaderprogramregistry.hpp>
#include <eepp/graphics/systemfontresolver.hpp>
#include <eepp/graphics/textlayout.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/graphics/vertexbufferregistry.hpp>
#include <eepp/network/http.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/packregistry.hpp>
#include <eepp/system/parsermatcher.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/system/virtualfilesystem.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/backendsdl2.hpp>
#include <eepp/window/backend/SDL2/platformhelpersdl2.hpp>
#if defined( EE_SDL_VERSION_3 )
#include <eepp/window/backend/SDL3/backendsdl3.hpp>
#include <eepp/window/backend/SDL3/platformhelpersdl3.hpp>
#endif
#include <eepp/window/engine.hpp>

#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <eepp/system/zip.hpp>
#endif

#define BACKEND_SDL2 1
#define BACKEND_SDL3 2

#ifndef DEFAULT_BACKEND

#if defined( EE_BACKEND_SDL3 )
#define DEFAULT_BACKEND BACKEND_SDL3
#elif defined( EE_BACKEND_SDL2 )
#define DEFAULT_BACKEND BACKEND_SDL2
#endif

#endif

using namespace EE::Graphics;

namespace EE { namespace Window {

static UintPtr sMainThreadId{ 0 };

SINGLETON_DECLARE_IMPLEMENTATION( Engine )

Engine::Engine() :
	mBackend( NULL ),
	mWindow( NULL ),
	mSharedGLContext( true ),
	mPlatformHelper( NULL ),
	mZip( NULL ),
	mDisplayManager( NULL ),
	mGlobalResourceCatalog( ResourceCatalog::New() ),
	mDefaultResourceScope( ResourceScope::New() ) {
	mDefaultResourceScope->importCatalog( mGlobalResourceCatalog );
#if EE_PLATFORM == EE_PLATFORM_ANDROID
	mZip = Zip::New();
	mZip->open( getPlatformHelper()->getApkPath() );

	FileSystem::changeWorkingDirectory( getPlatformHelper()->getExternalStoragePath() );
#endif

	UISceneNode::openAsyncResourceMainThreadQueue();
}

Engine::~Engine() {
	mIsShuttingDown = true;

	// Stop and join pool-owned HTTP clients before any scene or graphics resource their callbacks
	// can reach is destroyed.
	Network::Http::Pool::getGlobal().clear();

	// Reject new UI resource deliveries before scenes begin destruction. Rejected closures remain
	// queued until scene-owned producers have joined, allowing their captures to be released here
	// on the Engine/main thread.
	UISceneNode::beginAsyncResourceMainThreadQueueShutdown();
	if ( mWindow )
		mWindow->setCurrent();

	Scene::SceneManager::destroySingleton();
	UISceneNode::finishAsyncResourceMainThreadQueueShutdown();

	// Scene destruction can leave borrowed textures in the batch. Destroying the batch explicitly
	// discards those submissions while their resource managers are still alive.
	GlobalBatchRenderer::destroySingleton();

	// Cached layouts retain shaped runs that refer to managed fonts.
	TextLayout::clearLayoutCache();

	CSS::StyleSheetSpecification::destroySingleton();

	Doc::SyntaxDefinitionManager::destroySingleton();

	Graphics::Private::FrameBufferRegistry::destroySingleton();

	Graphics::Private::VertexBufferRegistry::destroySingleton();

	// Catalogs are the final intentional texture owners. Clear them while the factory and current
	// graphics context are still available for deferred release collection.
	mDefaultResourceScope.reset();
	mGlobalResourceCatalog.reset();

	if ( TextureFactory* textureFactory = TextureFactory::existsSingleton() )
		textureFactory->collectReleasedTextures();

	TextureFactory::destroySingleton();

	// Shader and renderer destructors issue GL commands. Programs must go first while GLi and the
	// current window context are still valid.
	ShaderProgramRegistry::destroySingleton();

	Graphics::Renderer::destroySingleton();

	PackRegistry::destroySingleton();

#ifdef EE_SSL_SUPPORT
	Network::SSL::SSLSocket::end();
#endif

	VirtualFileSystem::destroySingleton();

	// Windows own the GL contexts and must outlive every GPU resource and graphics manager above.
	destroy();

#if EE_PLATFORM == EE_PLATFORM_ANDROID
	eeSAFE_DELETE( mZip );
#endif

	eeSAFE_DELETE( mPlatformHelper );

	eeSAFE_DELETE( mDisplayManager );

	eeSAFE_DELETE( mBackend );

	SystemFontResolver::destroySingleton();

	RegExCache::destroySingleton();

	ParserMatcherManager::destroySingleton();

	Log::destroySingleton();
}

std::shared_ptr<ResourceCatalog> Engine::getGlobalResourceCatalog() const {
	return mGlobalResourceCatalog;
}

std::shared_ptr<ResourceScope> Engine::getDefaultResourceScope() const {
	return mDefaultResourceScope;
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

#ifdef EE_BACKEND_SDL3
Backend::WindowBackendLibrary* Engine::createSDL3Backend( const WindowSettings& ) {
#if defined( EE_SDL_VERSION_3 )
	return eeNew( Backend::SDL3::WindowBackendSDL3, () );
#else
	return NULL;
#endif
}

EE::Window::Window* Engine::createSDL3Window( const WindowSettings& Settings,
											  const ContextSettings& Context ) {
#if defined( EE_SDL_VERSION_3 )
	if ( NULL == mBackend ) {
		mBackend = createSDL3Backend( Settings );
	}

	return eeNew( Backend::SDL3::WindowSDL, ( Settings, Context ) );
#else
	return NULL;
#endif
}
#endif

EE::Window::Window* Engine::createDefaultWindow( const WindowSettings& Settings,
												 const ContextSettings& Context ) {
#if DEFAULT_BACKEND == BACKEND_SDL2
	return createSDL2Window( Settings, Context );
#elif DEFAULT_BACKEND == BACKEND_SDL3
	return createSDL3Window( Settings, Context );
#else
	return NULL;
#endif
}

EE::Window::Window* Engine::createWindow( WindowSettings Settings, ContextSettings Context ) {
	EE::Window::Window* window = NULL;

	if ( NULL != mWindow ) {
		Settings.Backend = mWindow->getWindowInfo()->WindowConfig.Backend;
	} else {
		sMainThreadId = Thread::getCurrentThreadId();
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

	if ( Settings.PixelDensity > 0 )
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
	return existsSingleton() && !isShuttingDown() && Engine::instance()->isRunning();
}

bool Engine::isRunning() const {
	return NULL != mWindow && !mIsShuttingDown;
}

WindowBackend Engine::getDefaultBackend() const {
#if DEFAULT_BACKEND == BACKEND_SDL2
	return WindowBackend::SDL2;
#elif DEFAULT_BACKEND == BACKEND_SDL3
	return WindowBackend::SDL3;
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
	else if ( "sdl3" == backend )
		winBackend = WindowBackend::SDL3;

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
	int frameRateLimit = ini->getValueI( iniKeyName, "frameratelimit",
										 ContextSettings::FrameRateLimitScreenRefreshRate );
	bool doubleBuffering = ini->getValueB( iniKeyName, "doublebuffering", true );
	bool sharedGLContext = ini->getValueB( iniKeyName, "sharedglcontext", false );

	return ContextSettings( VSync, frameRateLimit, multisamples,
							Renderer::glVersionFromString( GLVersion ), sharedGLContext,
							doubleBuffering, depthBufferSize, stencilBufferSize );
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

UintPtr Engine::getMainThreadId() {
	return sMainThreadId;
}

bool Engine::isMainThread() {
	return Thread::getCurrentThreadId() == Engine::getMainThreadId();
}

PlatformHelper* Engine::getPlatformHelper() {
	if ( NULL == mPlatformHelper ) {
#if DEFAULT_BACKEND == BACKEND_SDL2
		mPlatformHelper = eeNew( Backend::SDL2::PlatformHelperSDL2, () );
#elif DEFAULT_BACKEND == BACKEND_SDL3
		mPlatformHelper = eeNew( Backend::SDL3::PlatformHelperSDL3, () );
#endif
	}

	return mPlatformHelper;
}

DisplayManager* Engine::getDisplayManager() {
	if ( NULL == mDisplayManager ) {
#if DEFAULT_BACKEND == BACKEND_SDL2
		mDisplayManager = eeNew( Backend::SDL2::DisplayManagerSDL2, () );
#elif DEFAULT_BACKEND == BACKEND_SDL3
		mDisplayManager = eeNew( Backend::SDL3::DisplayManagerSDL3, () );
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

#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS

extern "C" int EE_SDL_main( int argc, char* argv[] );

extern "C" int SDL_main( int argc, char* argv[] ) {
	EngineInitializer engineInitializer;
	return EE_SDL_main( argc, argv );
}

#endif

}} // namespace EE::Window
