#include "cengine.hpp"
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

#include "backend/SDL/cwindowsdl.hpp"
#include "backend/SDL/cbackendsdl.hpp"

namespace EE { namespace Window {

cEngine::cEngine() :
	mBackend( NULL ),
	mWindow( NULL )
{
	mBackend = eeNew( Backend::SDL::cBackendSDL, () );
}

cEngine::~cEngine() {
	Destroy();

	Physics::cPhysicsManager::DestroySingleton();

	Graphics::Private::cFrameBufferManager::DestroySingleton();

	Graphics::Private::cVertexBufferManager::DestroySingleton();

	cGlobalBatchRenderer::DestroySingleton();

	cTextureFactory::DestroySingleton();

	cShapeGroupManager::DestroySingleton();

	cFontManager::DestroySingleton();

	UI::cUIManager::DestroySingleton();

	Audio::cAudioListener::DestroySingleton();

	Graphics::cGL::DestroySingleton();

	cShaderProgramManager::DestroySingleton();

	cLog::DestroySingleton();

	HaikuTTF::hkFontManager::DestroySingleton();

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
	cWindow * window = eeNew( Backend::SDL::cWindowSDL, ( Settings, Context ) );
	
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
	if ( NULL != window ) {
		mWindow = window;
	
		mWindow->SetDefaultContext();
	}
}

Uint32 cEngine::GetWindowCount() const {
	return mWindows.size();
}

bool cEngine::Running() const {
	return NULL != mWindow;
}

void cEngine::Close() {
	Destroy();
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
