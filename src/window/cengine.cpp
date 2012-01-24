#include "cengine.hpp"
#include "../system/cpackmanager.hpp"
#include "../graphics/ctexturefactory.hpp"
#include "../graphics/cfontmanager.hpp"
#include "../graphics/cglobalbatchrenderer.hpp"
#include "../graphics/cshaderprogrammanager.hpp"
#include "../graphics/cshapegroupmanager.hpp"
#include "../graphics/cframebuffermanager.hpp"
#include "../graphics/cvertexbuffermanager.hpp"
#include "../ui/cuimanager.hpp"
#include "../audio/caudiolistener.hpp"
#include "../graphics/glhelper.hpp"
#include "../helper/haikuttf/hkfontmanager.hpp"
#include "../physics/cphysicsmanager.hpp"

#include "backend/SDL/cbackendsdl.hpp"
#include "backend/SDL2/cbackendsdl.hpp"
#include "backend/allegro5/cbackendal.hpp"

#define BACKEND_SDL			1
#define BACKEND_ALLEGRO		2

#ifndef DEFAULT_BACKEND

#if defined( EE_BACKEND_SDL_ACTIVE )
#define DEFAULT_BACKEND		BACKEND_SDL
#elif defined( EE_BACKEND_ALLEGRO_ACTIVE )
#define DEFAULT_BACKEND		BACKEND_ALLEGRO
#endif

#endif

namespace EE { namespace Window {

SINGLETON_DECLARE_IMPLEMENTATION(cEngine)

cEngine::cEngine() :
	mBackend( NULL ),
	mWindow( NULL )
{
	#if DEFAULT_BACKEND == BACKEND_SDL

	#if defined( EE_SDL_VERSION_1_3 ) || defined( EE_SDL_VERSION_2 )
	mBackend = eeNew( Backend::SDL2::cBackendSDL, () );
	#else
	mBackend = eeNew( Backend::SDL::cBackendSDL, () );
	#endif

	#elif DEFAULT_BACKEND == BACKEND_ALLEGRO
	mBackend = eeNew( Backend::Al::cBackendAl, () );
	#endif
}

cEngine::~cEngine() {
	Physics::cPhysicsManager::DestroySingleton();

	Graphics::Private::cFrameBufferManager::DestroySingleton();

	Graphics::Private::cVertexBufferManager::DestroySingleton();

	cGlobalBatchRenderer::DestroySingleton();

	cTextureFactory::DestroySingleton();

	cShapeGroupManager::DestroySingleton();

	cFontManager::DestroySingleton();

	UI::cUIManager::DestroySingleton();

	Graphics::cGL::DestroySingleton();

	cShaderProgramManager::DestroySingleton();

	cPackManager::DestroySingleton();

	cLog::DestroySingleton();

	HaikuTTF::hkFontManager::DestroySingleton();

	Destroy();

	eeSAFE_DELETE( mBackend );
}

void cEngine::Destroy() {
	std::list<cWindow*>::iterator it;

	for ( it = mWindows.begin(); it != mWindows.end(); it++ ) {
		eeSAFE_DELETE( *it );
	}

	mWindow = NULL;
}

cWindow * cEngine::CreateWindow( WindowSettings Settings, ContextSettings Context ) {
	#if DEFAULT_BACKEND == BACKEND_SDL

	#if defined( EE_SDL_VERSION_1_3 ) || defined( EE_SDL_VERSION_2 )
	cWindow * window = eeNew( Backend::SDL2::cWindowSDL, ( Settings, Context ) );
	#else
	cWindow * window = eeNew( Backend::SDL::cWindowSDL, ( Settings, Context ) );
	#endif

	#elif DEFAULT_BACKEND == BACKEND_ALLEGRO
	cWindow * window = eeNew( Backend::Al::cWindowAl, ( Settings, Context ) );
	#endif

	if ( NULL == mWindow ) {
		mWindow = window;
	}

	mWindows.push_back( mWindow );

	return window;
}

void cEngine::DestroyWindow( cWindow * window ) {
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

bool cEngine::ExistsWindow( cWindow * window ) {
	std::list<cWindow*>::iterator it;

	for ( it = mWindows.begin(); it != mWindows.end(); it++ ) {
		if ( (*it) == window )
			return true;
	}

	return false;
}

cWindow * cEngine::GetCurrentWindow() const {
	return mWindow;
}

void cEngine::SetCurrentWindow( cWindow * window ) {
	if ( NULL != window && window != mWindow ) {
		mWindow = window;

		mWindow->SetCurrent();
	}
}

Uint32 cEngine::GetWindowCount() const {
	return mWindows.size();
}

bool cEngine::Running() const {
	return NULL != mWindow;
}

eeFloat cEngine::Elapsed() const {
	eeASSERT( Running() );

	return mWindow->Elapsed();
}

const Uint32& cEngine::GetWidth() const {
	eeASSERT( Running() );

	return mWindow->GetWidth();
}

const Uint32& cEngine::GetHeight() const {
	eeASSERT( Running() );

	return mWindow->GetHeight();
}

}}
