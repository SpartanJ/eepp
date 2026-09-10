#include <algorithm>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Scene {

SINGLETON_DECLARE_IMPLEMENTATION( SceneManager )

bool SceneManager::isActive() {
	return Engine::isEngineRunning() && SceneManager::existsSingleton() &&
		   !SceneManager::isShuttingDown();
}

SceneManager::SceneManager() : mUISceneNode( NULL ), mScopedUISceneNode( NULL ) {}

SceneManager::~SceneManager() {
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
		if ( mUISceneNode == sceneNode )
			mUISceneNode = nullptr;
		if ( mScopedUISceneNode == sceneNode )
			mScopedUISceneNode = nullptr;
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
		if ( sceneNode->isUISceneNode() ) {
			auto context = sceneNode->asType<UISceneNode>()->makeCurrent();
			sceneNode->draw();
		} else {
			auto context = Engine::instance()->makeWindowCurrent( sceneNode->getWindow() );
			sceneNode->draw();
		}
	}
}

void SceneManager::draw( EE::Window::Window* window ) {
	for ( auto& sceneNode : mSceneNodes ) {
		if ( sceneNode->getWindow() != window )
			continue;
		if ( sceneNode->isUISceneNode() ) {
			auto context = sceneNode->asType<UISceneNode>()->makeCurrent();
			sceneNode->draw();
		} else {
			auto context = Engine::instance()->makeWindowCurrent( window );
			sceneNode->draw();
		}
	}
}

void SceneManager::update( const Time& elapsed ) {
	for ( auto& sceneNode : mSceneNodes ) {
		if ( sceneNode->isUISceneNode() ) {
			auto context = sceneNode->asType<UISceneNode>()->makeCurrent();
			sceneNode->update( elapsed );
		} else {
			auto context = Engine::instance()->makeWindowCurrent( sceneNode->getWindow() );
			sceneNode->update( elapsed );
		}
	}
}

void SceneManager::update() {
	update( mClock.getElapsedTimeAndReset() );
}

UISceneNode* SceneManager::getUISceneNode() {
	if ( mScopedUISceneNode )
		return mScopedUISceneNode;

	if ( Engine::existsSingleton() ) {
		if ( mUISceneNode && mUISceneNode->getWindow() == Engine::instance()->getCurrentWindow() )
			return mUISceneNode;
		if ( auto* scene = getUISceneNode( Engine::instance()->getCurrentWindow() ) )
			return scene;
	}

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

UISceneNode* SceneManager::getUISceneNode( EE::Window::Window* window ) {
	for ( auto& sceneNode : mSceneNodes ) {
		if ( sceneNode->isUISceneNode() && sceneNode->getWindow() == window )
			return sceneNode->asType<UISceneNode>();
	}
	return nullptr;
}

void SceneManager::setCurrentUISceneNode( UISceneNode* uiSceneNode ) {
	mUISceneNode = uiSceneNode;
}

UISceneNode* SceneManager::setScopedUISceneNode( UISceneNode* uiSceneNode ) {
	UISceneNode* previous = mScopedUISceneNode;
	mScopedUISceneNode = uiSceneNode;
	return previous;
}

void SceneManager::destroyScenes( EE::Window::Window* window ) {
	for ( auto it = mSceneNodes.begin(); it != mSceneNodes.end(); ) {
		SceneNode* sceneNode = *it;
		if ( sceneNode->getWindow() != window ) {
			++it;
			continue;
		}
		if ( mUISceneNode == sceneNode )
			mUISceneNode = nullptr;
		if ( mScopedUISceneNode == sceneNode )
			mScopedUISceneNode = nullptr;
		it = mSceneNodes.erase( it );
		eeSAFE_DELETE( sceneNode );
	}
}

Time SceneManager::getElapsed() const {
	return mClock.getElapsedTime();
}

}} // namespace EE::Scene
