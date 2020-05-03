#include <eepp/ee.hpp>

// This file is used to test some UI related stuffs.
// It's not a benchmark or a real test suite.
// It's just used to test whatever I need to test at any given moment.

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
	win = Engine::instance()->createWindow( WindowSettings( 320, 240, "eepp - UI Hello World" ),
											ContextSettings( true ) );

	if ( win->isOpen() ) {
		PixelDensity::setPixelDensity(
			Engine::instance()->getDisplayManager()->getDisplayIndex( 0 )->getPixelDensity() );
		FontTrueType* font =
			FontTrueType::New( "NotoSans-Regular", "assets/fonts/NotoSans-Regular.ttf" );
		UISceneNode* uiSceneNode = UISceneNode::New();
		SceneManager::instance()->add( uiSceneNode );
		uiSceneNode->getUIThemeManager()->setDefaultFont( font );
		StyleSheetParser styleSheetParser;
		styleSheetParser.loadFromFile( "assets/ui/breeze.css" );
		uiSceneNode->setStyleSheet( styleSheetParser.getStyleSheet() );
		std::vector<String> strings;
		for ( size_t i = 0; i < 10000; i++ )
			strings.emplace_back( String::format(
				"This is a very long string number %ld. Cover the full width of the listbox.",
				i ) );

		auto* vlay = UILinearLayout::NewVertical();
		vlay->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

		Clock clock, total;
		/* ListBox test *//*
		auto* lbox = UIListBox::New();
		std::cout << "Time New: " << clock.getElapsed().asMilliseconds() << " ms" << std::endl;
		lbox->setParent( vlay );
		std::cout << "Time setParent: " << clock.getElapsed().asMilliseconds() << " ms"
				  << std::endl;
		lbox->setLayoutMargin( Rect( 4, 4, 4, 4 ) );
		std::cout << "Time setLayoutMargin: " << clock.getElapsed().asMilliseconds() << " ms"
				  << std::endl;
		lbox->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );
		std::cout << "Time setLayoutSizePolicy: " << clock.getElapsed().asMilliseconds() << " ms"
				  << std::endl;
		for ( size_t i = 0; i < 10; i++ )
			lbox->addListBoxItem( String::format(
				"This is a very long string number %ld. Cover the full width of the listbox.",
				i ) );
		std::cout << "Time addListBoxItem: " << clock.getElapsed().asMilliseconds() << " ms"
				  << std::endl;
		lbox->addListBoxItems( strings );
		std::cout << "Time addListBoxItems: " << clock.getElapsed().asMilliseconds() << " ms"
				  << std::endl;
		*/
		/* Create Widget test */
		for ( size_t i = 0; i < 5000; i++ ) {
			auto* widget = UIWidget::New();
			widget->setParent( vlay );
			//widget->setLayoutSizePolicy( SizePolicy::WrapContent, SizePolicy::WrapContent );
		}
		std::cout << "Time total: " << total.getElapsedTime().asMilliseconds() << " ms"
				  << std::endl;

		win->close();

		win->runMainLoop( &mainLoop );
	}

	Engine::destroySingleton();
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
