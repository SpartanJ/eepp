#include <algorithm>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/tools/uiinspectorserver.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Scene {

SINGLETON_DECLARE_IMPLEMENTATION( SceneManager )

bool SceneManager::isActive() {
	return Engine::isEngineRunning() && SceneManager::existsSingleton() &&
		   !SceneManager::isShuttingDown();
}

SceneManager::SceneManager() : mCurrentSceneNode( nullptr ) {}

SceneManager::~SceneManager() {
	UIInspectorServer::stop();
	for ( auto& it : mSceneNodes ) {
		SceneNode* node = it;
		eeSAFE_DELETE( node );
	}

	mSceneNodes.clear();
}

SceneNode* SceneManager::add( SceneNode* sceneNode ) {
	mSceneNodes.push_back( sceneNode );
	if ( !mCurrentSceneNode )
		setCurrentSceneNode( sceneNode );
	if ( sceneNode->isUISceneNode() )
		UIInspectorServer::startFromEnvironment( sceneNode->asType<UISceneNode>() );
	return sceneNode;
}

bool SceneManager::remove( SceneNode* sceneNode ) {
	auto it = std::find( mSceneNodes.begin(), mSceneNodes.end(), sceneNode );
	if ( it != mSceneNodes.end() ) {
		if ( mCurrentSceneNode == sceneNode )
			mCurrentSceneNode = nullptr;
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
		auto context = sceneNode->makeCurrent();
		sceneNode->draw();
	}
}

void SceneManager::draw( EE::Window::Window* window ) {
	for ( auto& sceneNode : mSceneNodes ) {
		if ( sceneNode->getWindow() != window )
			continue;
		auto context = sceneNode->makeCurrent();
		sceneNode->draw();
	}
}

void SceneManager::update( const Time& elapsed ) {
	// A scene update can create another native window (including through an accessibility action),
	// which appends to mSceneNodes and may reallocate the vector. Keep the current pass stable and
	// let newly-created scenes participate in the next update. A scene removed during the pass
	// shrinks the vector, so bound by the live size as well.
	const size_t sceneCount = mSceneNodes.size();
	for ( size_t i = 0; i < sceneCount && i < mSceneNodes.size(); ++i ) {
		auto* sceneNode = mSceneNodes[i];
		auto context = sceneNode->makeCurrent();
		sceneNode->update( elapsed );
	}
	UIInspectorServer::pump();
}

void SceneManager::update() {
	update( mClock.getElapsedTimeAndReset() );
}

SceneNode* SceneManager::getCurrentSceneNode() const {
	return mCurrentSceneNode;
}

UISceneNode* SceneManager::getUISceneNode() {
	return mCurrentSceneNode && mCurrentSceneNode->isUISceneNode()
			   ? mCurrentSceneNode->asType<UISceneNode>()
			   : nullptr;
}

UISceneNode* SceneManager::getUISceneNode( EE::Window::Window* window ) {
	for ( auto& sceneNode : mSceneNodes ) {
		if ( sceneNode->isUISceneNode() && sceneNode->getWindow() == window )
			return sceneNode->asType<UISceneNode>();
	}
	return nullptr;
}

void SceneManager::forEachSceneNode( const std::function<void( SceneNode* )>& callback ) const {
	for ( auto* sceneNode : mSceneNodes )
		callback( sceneNode );
}

SceneNode* SceneManager::setCurrentSceneNode( SceneNode* sceneNode ) {
	SceneNode* previous = mCurrentSceneNode;
	mCurrentSceneNode = sceneNode;
	return previous;
}

void SceneManager::destroyScenes( EE::Window::Window* window ) {
	for ( auto it = mSceneNodes.begin(); it != mSceneNodes.end(); ) {
		SceneNode* sceneNode = *it;
		if ( sceneNode->getWindow() != window ) {
			++it;
			continue;
		}
		if ( mCurrentSceneNode == sceneNode )
			mCurrentSceneNode = nullptr;
		it = mSceneNodes.erase( it );
		eeSAFE_DELETE( sceneNode );
	}
}

Time SceneManager::getElapsed() const {
	return mClock.getElapsedTime();
}

}} // namespace EE::Scene
