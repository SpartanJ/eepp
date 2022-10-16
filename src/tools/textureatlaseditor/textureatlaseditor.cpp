#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/tools/textureatlaseditor.hpp>
#include <eepp/ui/uimessagebox.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitheme.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::Scene;
using namespace EE::Window;
using namespace EE::UI;
using namespace EE::UI::Tools;

EE::Window::Window* win = NULL;
UIMessageBox* MsgBox = NULL;
TextureAtlasEditor* Editor = NULL;

bool onCloseRequestCallback( EE::Window::Window* ) {
	if ( NULL != Editor && Editor->isEdited() ) {
		MsgBox = UIMessageBox::New(
			UIMessageBox::OK_CANCEL,
			"Do you really want to close the texture atlas editor?\nAll changes will be lost." );
		MsgBox->addEventListener( Event::MsgBoxConfirmClick,
								  []( const Event* ) { win->close(); } );
		MsgBox->addEventListener( Event::OnClose, []( const Event* ) { MsgBox = NULL; } );
		MsgBox->setTitle( "Close Texture Atlas Editor?" );
		MsgBox->center();
		MsgBox->show();
		return false;
	} else {
		return true;
	}
}

void mainLoop() {
	win->getInput()->update();

	if ( win->getInput()->isKeyUp( KEY_ESCAPE ) && NULL == MsgBox &&
		 onCloseRequestCallback( win ) ) {
		win->close();
	}

	UISceneNode* uiSceneNode = SceneManager::instance()->getUISceneNode();

	if ( win->getInput()->isKeyUp( KEY_F6 ) ) {
		uiSceneNode->setHighlightFocus( !uiSceneNode->getHighlightFocus() );
		uiSceneNode->setHighlightOver( !uiSceneNode->getHighlightOver() );
	}

	if ( win->getInput()->isKeyUp( KEY_F7 ) ) {
		uiSceneNode->setDrawBoxes( !uiSceneNode->getDrawBoxes() );
	}

	if ( win->getInput()->isKeyUp( KEY_F8 ) ) {
		uiSceneNode->setDrawDebugData( !uiSceneNode->getDrawDebugData() );
	}

	SceneManager::instance()->update();

	if ( uiSceneNode->invalidated() ) {
		win->clear();

		SceneManager::instance()->draw();

		win->display();
	} else {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
		win->getInput()->waitEvent( Milliseconds( win->hasFocus() ? 16 : 100 ) );
#endif
	}
}

EE_MAIN_FUNC int main( int, char*[] ) {
	DisplayManager* displayManager = Engine::instance()->getDisplayManager();
	displayManager->enableScreenSaver();
	displayManager->enableMouseFocusClickThrough();
	displayManager->disableBypassCompositor();
	Display* currentDisplay = displayManager->getDisplayIndex( 0 );
	Float pixelDensity = currentDisplay->getPixelDensity();

	std::string resPath( Sys::getProcessPath() );

	win = Engine::instance()->createWindow(
		WindowSettings( 1280, 720, "eepp - Texture Atlas Editor", WindowStyle::Default,
						WindowBackend::Default, 32, resPath + "assets/icon/ee.png", pixelDensity ),
		ContextSettings( true, GLv_default, true, 24, 1, 0, true ) );

	if ( win->isOpen() ) {
		PixelDensity::setPixelDensity( eemax( win->getScale(), pixelDensity ) );

		win->setCloseRequestCallback( onCloseRequestCallback );

		UISceneNode* uiSceneNode = UISceneNode::New();

		SceneManager::instance()->add( uiSceneNode );

		{
			std::string pd;
			if ( PixelDensity::getPixelDensity() >= 1.5f )
				pd = "1.5x";
			else if ( PixelDensity::getPixelDensity() >= 2.f )
				pd = "2x";

			FontTrueType* font = FontTrueType::New( "NotoSans-Regular",
													resPath + "assets/fonts/NotoSans-Regular.ttf" );

			/*UITheme* theme = UITheme::load( "uitheme" + pd, "uitheme" + pd, resPath +
			 * "assets/ui/uitheme" + pd + ".eta", font, resPath + "assets/ui/uitheme.css" );*/

			UITheme* theme =
				UITheme::load( "uitheme", "uitheme", "", font, resPath + "assets/ui/breeze.css" );

			uiSceneNode->combineStyleSheet( theme->getStyleSheet() );

			uiSceneNode->getUIThemeManager()
				->setDefaultEffectsEnabled( true )
				->setDefaultTheme( theme )
				->setDefaultFont( font )
				->add( theme );
		}

		Editor = TextureAtlasEditor::New();

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();

	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
