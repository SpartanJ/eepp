#include <algorithm>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Scene {

SINGLETON_DECLARE_IMPLEMENTATION( SceneManager )

bool SceneManager::isActive() {
	return EE::Window::Engine::isEngineRunning() && SceneManager::existsSingleton() &&
		   !SceneManager::instance()->isShuttingDown();
}

SceneManager::SceneManager() : mUISceneNode( NULL ), mIsShuttingDown( false ) {}

SceneManager::~SceneManager() {
	mIsShuttingDown = true;

	for ( auto& it : mSceneNodes ) {
		SceneNode* node = it;
		eeSAFE_DELETE( node );
	}

	mSceneNodes.clear();
}

SceneNode* SceneManager::add( SceneNode* sceneNode ) {
	mSceneNodes.push_back( sceneNode );
	return sceneNode;
}

bool SceneManager::remove( SceneNode* sceneNode ) {
	auto it = std::find( mSceneNodes.begin(), mSceneNodes.end(), sceneNode );
	if ( it != mSceneNodes.end() ) {
		mSceneNodes.erase( it );
		return true;
	}
	return false;
}

size_t SceneManager::count() const {
	return mSceneNodes.size();
}

void SceneManager::draw() {
	for ( auto& sceneNode : mSceneNodes ) {
		sceneNode->draw();
	}
}

void SceneManager::update( const Time& elapsed ) {
	for ( auto& sceneNode : mSceneNodes ) {
		sceneNode->update( elapsed );
	}
}

void SceneManager::update() {
	update( mClock.getElapsedTimeAndReset() );
}

bool SceneManager::isShuttingDown() const {
	return mIsShuttingDown;
}

UISceneNode* SceneManager::getUISceneNode() {
	if ( NULL == mUISceneNode ) {
		for ( auto& sceneNode : mSceneNodes ) {
			if ( sceneNode->isUISceneNode() ) {
				mUISceneNode = sceneNode->asType<UISceneNode>();
				break;
			}
		}
	}

	return mUISceneNode;
}

void SceneManager::setCurrentUISceneNode( UISceneNode* uiSceneNode ) {
	mUISceneNode = uiSceneNode;
}

Time SceneManager::getElapsed() const {
	return mClock.getElapsedTime();
}

}} // namespace EE::Scene
