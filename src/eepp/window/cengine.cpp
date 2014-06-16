#include <eepp/window/cengine.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/inifile.hpp>
#include <eepp/graphics/ctexturefactory.hpp>
#include <eepp/graphics/cfontmanager.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <eepp/graphics/cshaderprogrammanager.hpp>
#include <eepp/graphics/ctextureatlasmanager.hpp>
#include <eepp/graphics/cframebuffermanager.hpp>
#include <eepp/graphics/cvertexbuffermanager.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/audio/audiolistener.hpp>
#include <eepp/helper/haikuttf/hkfontmanager.hpp>
#include <eepp/physics/cphysicsmanager.hpp>
#include <eepp/network/ssl/sslsocket.hpp>
#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL/cbackendsdl.hpp>
#include <eepp/window/backend/SDL2/cbackendsdl2.hpp>
#include <eepp/window/backend/SFML/cbackendsfml.hpp>
#include <eepp/graphics/renderer/cgl.hpp>

#define BACKEND_SDL			1
#define BACKEND_SDL2		2
#define BACKEND_SFML		3

#ifndef DEFAULT_BACKEND

#if defined( EE_BACKEND_SDL2 )
#define DEFAULT_BACKEND		BACKEND_SDL2
#elif defined( EE_BACKEND_SDL_1_2 )
#define DEFAULT_BACKEND		BACKEND_SDL
#elif defined( EE_BACKEND_SFML_ACTIVE )
#define DEFAULT_BACKEND		BACKEND_SFML
#endif

#endif

namespace EE { namespace Window {

SINGLETON_DECLARE_IMPLEMENTATION(Engine)

Engine::Engine() :
	mBackend( NULL ),
	mWindow( NULL ),
	mSharedGLContext( false ),
	mMainThreadId( 0 )
{
	cTextureAtlasManager::CreateSingleton();
}

Engine::~Engine() {
	Physics::cPhysicsManager::DestroySingleton();

	Graphics::Private::cFrameBufferManager::DestroySingleton();

	Graphics::Private::cVertexBufferManager::DestroySingleton();

	cGlobalBatchRenderer::DestroySingleton();

	cTextureFactory::DestroySingleton();

	cTextureAtlasManager::DestroySingleton();

	cFontManager::DestroySingleton();

	UI::cUIManager::DestroySingleton();

	Graphics::cGL::DestroySingleton();

	cShaderProgramManager::DestroySingleton();

	PackManager::DestroySingleton();

	Log::DestroySingleton();

	HaikuTTF::hkFontManager::DestroySingleton();

	#ifdef EE_SSL_SUPPORT
	Network::SSL::SSLSocket::End();
	#endif

	Destroy();

	eeSAFE_DELETE( mBackend );
}

void Engine::Destroy() {
	std::list<Window*>::iterator it;

	for ( it = mWindows.begin(); it != mWindows.end(); it++ ) {
		eeSAFE_DELETE( *it );
	}

	mWindow = NULL;
}

Backend::WindowBackend * Engine::CreateSDLBackend( const WindowSettings &Settings ) {
#if defined( EE_SDL_VERSION_1_2 )
	return eeNew( Backend::SDL::WindowBackendSDL, () );
#else
	return NULL;
#endif
}

Backend::WindowBackend * Engine::CreateSDL2Backend( const WindowSettings &Settings ) {
#if defined( EE_SDL_VERSION_2 )
	return eeNew( Backend::SDL2::WindowBackendSDL2, () );
#else
	return NULL;
#endif
}

Backend::WindowBackend * Engine::CreateSFMLBackend( const WindowSettings &Settings ) {
#if defined( EE_BACKEND_SFML_ACTIVE )
	return eeNew( Backend::SFML::WindowBackendSFML, () );
#else
	return NULL;
#endif
}

EE::Window::Window * Engine::CreateSDLWindow( const WindowSettings& Settings, const ContextSettings& Context ) {
#if defined( EE_SDL_VERSION_1_2 )
	if ( NULL == mBackend ) {
		mBackend	= CreateSDLBackend( Settings );
	}

	return eeNew( Backend::SDL::WindowSDL, ( Settings, Context ) );
#else
	return NULL;
#endif
}

EE::Window::Window * Engine::CreateSDL2Window( const WindowSettings& Settings, const ContextSettings& Context ) {
#if defined( EE_SDL_VERSION_2 )
	if ( NULL == mBackend ) {
		mBackend	= CreateSDL2Backend( Settings );
	}

	return eeNew( Backend::SDL2::WindowSDL, ( Settings, Context ) );
#else
	return NULL;
#endif
}

EE::Window::Window * Engine::CreateSFMLWindow( const WindowSettings& Settings, const ContextSettings& Context ) {
#if defined( EE_BACKEND_SFML_ACTIVE )

	if ( NULL == mBackend ) {
		mBackend	= CreateSFMLBackend( Settings );
	}

	return eeNew( Backend::SFML::WindowSFML, ( Settings, Context ) );
#else
	return NULL;
#endif
}

EE::Window::Window * Engine::CreateDefaultWindow( const WindowSettings& Settings, const ContextSettings& Context ) {
#if DEFAULT_BACKEND == BACKEND_SDL
	return CreateSDLWindow( Settings, Context );
#elif DEFAULT_BACKEND == BACKEND_SDL2
	return CreateSDL2Window( Settings, Context );
#elif DEFAULT_BACKEND == BACKEND_SFML
	return CreateSFMLWindow( Settings, Context );
#endif
}

EE::Window::Window * Engine::CreateWindow( WindowSettings Settings, ContextSettings Context ) {
	EE::Window::Window * window = NULL;

	if ( NULL != mWindow ) {
		Settings.Backend	= mWindow->GetWindowInfo()->WindowConfig.Backend;
	} else {
		mMainThreadId	= Thread::GetCurrentThreadId();
	}

	switch ( Settings.Backend ) {
		case WindowBackend::SDL:		window = CreateSDLWindow( Settings, Context );		break;
		case WindowBackend::SDL2:		window = CreateSDL2Window( Settings, Context );		break;
		case WindowBackend::SFML:		window = CreateSFMLWindow( Settings, Context );		break;
		case WindowBackend::Default:
		default:						window = CreateDefaultWindow( Settings, Context );	break;
	}

	if ( NULL == window ) {
		window = CreateDefaultWindow( Settings, Context );
	}

	if ( NULL == mWindow ) {
		mWindow = window;
	}

	mWindows.push_back( mWindow );

	return window;
}

void Engine::DestroyWindow( EE::Window::Window * window ) {
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

bool Engine::ExistsWindow( EE::Window::Window * window ) {
	std::list<Window*>::iterator it;

	for ( it = mWindows.begin(); it != mWindows.end(); it++ ) {
		if ( (*it) == window )
			return true;
	}

	return false;
}

EE::Window::Window * Engine::GetCurrentWindow() const {
	return mWindow;
}

void Engine::SetCurrentWindow( EE::Window::Window * window ) {
	if ( NULL != window && window != mWindow ) {
		mWindow = window;

		mWindow->SetCurrent();
	}
}

Uint32 Engine::GetWindowCount() const {
	return mWindows.size();
}

bool Engine::Running() const {
	return NULL != mWindow;
}

Time Engine::Elapsed() const {
	eeASSERT( Running() );

	return mWindow->Elapsed();
}

const Uint32& Engine::GetWidth() const {
	eeASSERT( Running() );

	return mWindow->GetWidth();
}

const Uint32& Engine::GetHeight() const {
	eeASSERT( Running() );

	return mWindow->GetHeight();
}

Uint32 Engine::GetDefaultBackend() const {
#if DEFAULT_BACKEND == BACKEND_SDL
	return WindowBackend::SDL;
#elif DEFAULT_BACKEND == BACKEND_SDL2
	return WindowBackend::SDL2;
#elif DEFAULT_BACKEND == BACKEND_SFML
	return WindowBackend::SFML;
#endif
}

WindowSettings Engine::CreateWindowSettings( IniFile * ini, std::string iniKeyName ) {
	eeASSERT ( NULL != ini );

	ini->ReadFile();

	int Width 			= ini->GetValueI( iniKeyName, "Width", 800 );
	int Height			= ini->GetValueI( iniKeyName, "Height", 600 );
	int BitColor		= ini->GetValueI( iniKeyName, "BitColor", 32);
	bool Windowed		= ini->GetValueB( iniKeyName, "Windowed", true );
	bool Resizeable		= ini->GetValueB( iniKeyName, "Resizeable", true );

	std::string Backend = ini->GetValue( iniKeyName, "Backend", "" );
	Uint32 WinBackend	= GetDefaultBackend();

	String::ToLower( Backend );

	if ( "sdl2" == Backend )		WinBackend	= WindowBackend::SDL2;
	else if ( "sdl" == Backend )	WinBackend	= WindowBackend::SDL;
	else if ( "sfml" == Backend )	WinBackend	= WindowBackend::SFML;

	Uint32 Style = WindowStyle::Titlebar;

	if ( !Windowed )
		Style |= WindowStyle::Fullscreen;

	if ( Resizeable )
		Style |= WindowStyle::Resize;

	std::string Icon	= ini->GetValue( iniKeyName, "WinIcon", "" );
	std::string Caption	= ini->GetValue( iniKeyName, "WinCaption", "" );

	WindowSettings WinSettings( Width, Height, Caption, Style, WinBackend, BitColor, Icon );

	#if EE_PLATFORM == EE_PLATFORM_IOS
	//! @TODO: Check if SDL2 default win settings are being forced ( it wasn't working fine some time ago )
	WinSettings.Width	= 960;
	WinSettings.Height	= 640;
	WinSettings.Style	= WindowStyle::NoBorder;
	#endif

	return WinSettings;
}

WindowSettings Engine::CreateWindowSettings( std::string iniPath, std::string iniKeyName ) {
	IniFile Ini( iniPath );

	return CreateWindowSettings( &Ini, iniKeyName );
}

ContextSettings Engine::CreateContextSettings( IniFile * ini, std::string iniKeyName ) {
	eeASSERT ( NULL != ini );

	ini->ReadFile();

	bool VSync					= ini->GetValueB( iniKeyName, "VSync", true );
	std::string GLVersion		= ini->GetValue( iniKeyName, "GLVersion", "0" );

	String::ToLower( GLVersion );

	EEGL_version GLVer;
	if (		"3" == GLVersion || "opengl 3" == GLVersion || "gl3" == GLVersion || "opengl3" == GLVersion )									GLVer = GLv_3;
	else if (	"4" == GLVersion || "opengl es 2" == GLVersion || "gles2" == GLVersion || "opengles2" == GLVersion || "es2" == GLVersion )		GLVer = GLv_ES2;
	else if (	"5" == GLVersion || "opengl 3 core profile" == GLVersion ||
				"gl3cp" == GLVersion || "opengl3cp" == GLVersion || "opengl core profile" == GLVersion ||
				"core profile" == GLVersion || "cp" == GLVersion )																				GLVer = GLv_3CP;
	else if (	"opengl es 1" == GLVersion || "gles1" == GLVersion || "gl es 1" == GLVersion || "opengl es1" == GLVersion ||
				"opengles1" == GLVersion || "es1" == GLVersion || "gles 1" == GLVersion )														GLVer = GLv_ES1;
	else if (	"2" == GLVersion || "opengl 2" == GLVersion || "gl2" == GLVersion || "gl 2" == GLVersion )										GLVer = GLv_2;
	else																																		GLVer = GLv_default;

	bool doubleBuffering 		= ini->GetValueB( iniKeyName, "DoubleBuffering", true );
	int depthBufferSize 		= ini->GetValueI( iniKeyName, "DepthBufferSize", 24 );
	int stencilBufferSize 		= ini->GetValueI( iniKeyName, "StencilBufferSize", 1 );

	return ContextSettings( VSync, GLVer, doubleBuffering, depthBufferSize, stencilBufferSize );
}

ContextSettings Engine::CreateContextSettings( std::string iniPath, std::string iniKeyName ) {
	IniFile Ini( iniPath );

	return CreateContextSettings( &Ini );
}

void Engine::EnableSharedGLContext() {
	mSharedGLContext = true;
}

void Engine::DisableSharedGLContext() {
	mSharedGLContext = false;
}

bool Engine::IsSharedGLContextEnabled() {
	return mSharedGLContext;
}

Uint32 Engine::GetMainThreadId() {
	return mMainThreadId;
}

}}
