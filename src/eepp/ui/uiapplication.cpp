#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>

using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::Scene;

namespace EE { namespace UI {

UIApplication::UIApplication( const WindowSettings& windowSettings, bool loadBaseResources,
							  const ContextSettings& contextSettings ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();

	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	Float pixelDensity = currentDisplay->getPixelDensity();

	mWindow = Engine::instance()->createWindow( windowSettings, contextSettings );

	if ( !mWindow->isOpen() )
		return;

	mDidRun = true;

	PixelDensity::setPixelDensity( eemax( mWindow->getScale(), pixelDensity ) );

	if ( !loadBaseResources )
		return;

	FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

	FontTrueType* font =
		FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

	mUISceneNode = UISceneNode::New();
	mUISceneNode->getUIThemeManager()->setDefaultFont( font );
	SceneManager::instance()->add( mUISceneNode );

	mUISceneNode->getRoot()->addClass( "appbackground" );

	UITheme* theme = UITheme::load( "uitheme", "uitheme", "", font, "assets/ui/breeze.css" );
	mUISceneNode->setStyleSheet( theme->getStyleSheet() );
	mUISceneNode->getUIThemeManager()
		->setDefaultEffectsEnabled( true )
		->setDefaultTheme( theme )
		->setDefaultFont( font )
		->add( theme );
}

UIApplication::~UIApplication() {
	Engine::destroySingleton();
	MemoryManager::showResults();
}

EE::Window::Window* UIApplication::getWindow() const {
	return mWindow;
}

UISceneNode* UIApplication::getUI() const {
	return mUISceneNode;
}

int UIApplication::run() {
	mWindow->runMainLoop( [this]() {
		mWindow->getInput()->update();
		SceneManager::instance()->update();

		if ( mUISceneNode->invalidated() ) {
			mWindow->clear();

			SceneManager::instance()->draw();

			mWindow->display();
		} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
			mWindow->getInput()->waitEvent( Milliseconds( mWindow->hasFocus() ? 16 : 100 ) );
#endif
		}
	} );

	return mDidRun ? EXIT_SUCCESS : EXIT_FAILURE;
}

}} // namespace EE::UI
