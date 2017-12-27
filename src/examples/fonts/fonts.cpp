#include <eepp/ee.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>

EE::Window::Window * win			= NULL;
FontTrueType * fontTest;
FontTrueType * fontTest2;
Text text;
Text text2;
Text text3;

void mainLoop() {
	// Clear the screen buffer
	win->clear();

	// Update the input
	win->getInput()->update();

	// Check if ESCAPE key is pressed
	if ( win->getInput()->isKeyDown( KEY_ESCAPE ) ) {
		// Close the window
		win->close();
	}

	text.draw( ( win->getWidth() - text.getTextWidth() ) * 0.5f, 32 );

	text2.draw( ( win->getWidth() - text2.getTextWidth() ) * 0.5f, 300 );

	// Text rotated and scaled
	text2.draw( ( win->getWidth() - text2.getTextWidth() ) * 0.5f, 430, Vector2f(1.1f,1.1f), 12.5f );

	text3.draw( ( win->getWidth() - text3.getTextWidth() ) * 0.5f, 560 );

	// Draw frame
	win->display();
}

EE_MAIN_FUNC int main (int argc, char * argv []) {
	// Create a new window
	win = Engine::instance()->createWindow( WindowSettings( 960, 640, "eepp - Fonts" ), ContextSettings( true ) );

	// Set window background color
	win->setClearColor( RGB(230,230,230) );

	// Check if created
	if ( win->isOpen() ) {
		// Get the application path
		std::string AppPath = Sys::getProcessPath();

		// Create a new text string
		String Txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

		fontTest = FontTrueType::New( "DejaVuSansMono" );
		fontTest->loadFromFile( AppPath + "assets/fonts/DejaVuSansMono.ttf" );

		text.setFont( fontTest );
		text.setCharacterSize( 24 );
		text.setAlign( TEXT_ALIGN_CENTER );
		text.setString( Txt );
		text.shrinkText( win->getWidth() - 96 );

		// Set the font color to a substring of the text
		// Create a gradient
		int size = (int)Txt.size();

		for ( int i = 0; i < size; i++ ) {
			text.setFillColor( Color(255*i/size,0,0,255), i, i+1 );
		}

		fontTest2 = FontTrueType::New( "NotoSans-Regular" );
		fontTest2->loadFromFile( AppPath + "assets/fonts/NotoSans-Regular.ttf" );

		text2.setFont( fontTest2 );
		text2.setString( "Lorem ipsum dolor sit amet, consectetur adipisicing elit." );
		text2.setCharacterSize( 32 );
		text2.setFillColor( Color::Black );

		text3.setFont( fontTest );
		text3.setString( text2.getString() );
		text3.setOutlineThickness( 2 );
		text3.setCharacterSize( 24 );
		text3.setFillColor( Color(255,255,255,255) );
		text3.setOutlineColor( Color(0,0,0,255) );

		// Application loop
		win->runMainLoop( &mainLoop );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	// Fonts are autoreleased by the engine
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
