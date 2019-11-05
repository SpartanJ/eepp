#include <eepp/scene/scenemanager.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/ui/uiscenenode.hpp>


namespace EE { namespace Scene {

SINGLETON_DECLARE_IMPLEMENTATION(SceneManager)

SceneManager::SceneManager() :
	mUISceneNode( NULL ),
	mIsShootingDown( false )
{}

SceneManager::~SceneManager() {
	mIsShootingDown = true;

	for ( auto it = mResources.begin() ; it != mResources.end(); it++ ) {
		eeSAFE_DELETE( (*it) );
	}

	mResources.clear();
}

void SceneManager::draw() {
	for ( auto it = mResources.begin() ; it != mResources.end(); it++ ) {
		SceneNode * sceneNode = (*it);

		sceneNode->draw();
	}
}

void SceneManager::update( const Time& elapsed ) {
	for ( auto it = mResources.begin() ; it != mResources.end(); it++ ) {
		SceneNode * sceneNode = (*it);

		sceneNode->update( elapsed );
	}
}

void SceneManager::update() {
	update( mClock.getElapsed() );
}

bool SceneManager::isShootingDown() const {
	return mIsShootingDown;
}

UISceneNode * SceneManager::getUISceneNode() {
	if ( NULL == mUISceneNode ) {
		for ( auto it = mResources.begin() ; it != mResources.end(); it++ ) {
			SceneNode * sceneNode = (*it);

			if ( sceneNode->isUISceneNode() ) {
				mUISceneNode = sceneNode->asType<UISceneNode>();
				break;
			}
		}
	}

	return mUISceneNode;
}

void SceneManager::setCurrentUISceneNode( UISceneNode * uiSceneNode ) {
	mUISceneNode = uiSceneNode;
}

}}
