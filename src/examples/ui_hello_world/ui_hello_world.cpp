#include <eepp/ee.hpp>

EE::Window::Window* win = NULL;

void mainLoop() {
	win->getInput()->update();

	if ( win->getInput()->isKeyUp( KEY_ESCAPE ) ) {
		win->close();
	}

	// Update the UI scene.
	SceneManager::instance()->update();

	// Check if the UI has been invalidated ( needs redraw ).
	if ( SceneManager::instance()->getUISceneNode()->invalidated() ) {
		win->clear();

		// Redraw the UI scene.
		SceneManager::instance()->draw();

		win->display();
	} else {
		Sys::sleep( Milliseconds( 8 ) );
	}
}

EE_MAIN_FUNC int main( int argc, char* argv[] ) {
	win = Engine::instance()->createWindow( WindowSettings( 640, 480, "eepp - UI Hello World" ),
											ContextSettings( true ) );

	if ( win->isOpen() ) {
		// Load a font to use as the default font in our UI.
		FontTrueType* font =
			FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );

		// Create a new scene node to add our widgets.
		UISceneNode* uiSceneNode = UISceneNode::New();

		// Set the default font used in the scene node (otherwise we won't have any font to create
		// text views.
		uiSceneNode->getUIThemeManager()->setDefaultFont( font );

		// Add the new scene node to the scene manager.
		SceneManager::instance()->add( uiSceneNode );

		// Create a very simple Hello World with a TextView and a PushButton.
		uiSceneNode->loadLayoutFromString( R"xml(
			<LinearLayout layout_width="match_parent"
						  layout_height="match_parent"
						  orientation="vertical">
				<TextView id="text_view"
						  layout_width="match_parent"
						  layout_height="wrap_content"
						  text="Hello, I am a TextView" />
				<PushButton id="button_view"
						layout_width="match_parent"
						layout_height="wrap_content"
						text="Hello, I am a PushButton" />
			</LinearLayout>
		)xml" );

		// Set the style to our "Hello World" widgets.
		uiSceneNode->setStyleSheet( R"css(
			* {
				font-size: 22dp;
			}

			TextView {
				background-color: white;
				color: black;
				text-align: center;
			}

			PushButton {
				background-color: red;
				color: white;
			}
		)css" );

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
