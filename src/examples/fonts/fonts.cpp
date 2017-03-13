#include <eepp/ee.hpp>
#include <eepp/graphics/fonttruetype.hpp>
#include <eepp/graphics/text.hpp>

EE::Window::Window * win			= NULL;
FontTrueType * fontTest;
Uint32 nextGliph = 0;
Clock timer;
Text text;

void mainLoop()
{
	// Clear the screen buffer
	win->clear();

	// Update the input
	win->getInput()->update();

	// Check if ESCAPE key is pressed
	if ( win->getInput()->isKeyDown( KEY_ESCAPE ) ) {
		// Close the window
		win->close();
	}

/*
	Float YPos = 32;

	// Draw the text on screen
	TTFCache.draw( win->getWidth() * 0.5f - TTFCache.getTextWidth() * 0.5f, YPos );

	TTFOCache.draw( ( win->getWidth() - TTFOCache.getTextWidth() ) * 0.5f, ( YPos += TTFCache.getTextHeight() + 24 ) );

	TTF2Cache.draw( ( win->getWidth() - TTF2Cache.getTextWidth() ) * 0.5f, ( YPos += TTFOCache.getTextHeight() + 24 ) );

	TexFCache.draw( ( win->getWidth() - TexFCache.getTextWidth() ) * 0.5f, ( YPos += TTF2Cache.getTextHeight() + 24 ) );

	TexF2Cache.draw( ( win->getWidth() - TexF2Cache.getTextWidth() ) * 0.5f, ( YPos += TexFCache.getTextHeight() + 24 ) );

	// Draw the cached text
	TxtCache.draw( ( win->getWidth() - TxtCache.getTextWidth() ) * 0.5f, ( YPos += TexF2Cache.getTextHeight() + 24 ) );

	// Text rotated and scaled
	TTFCache.draw( ( win->getWidth() - TTFCache.getTextWidth() ) * 0.5f, 512 + 32, Vector2f( 0.75f, 0.75f ), 12.5f );
*/
	/*if ( timer.getElapsedTime().asMilliseconds() > 50 ) {
		fontTest.getGlyph( nextGliph, 48, false );
		nextGliph++;
		timer.restart();
	}*/

	text.draw( ( win->getWidth() - text.getTextWidth() ) * 0.5f, 0 );

	// Draw frame
	win->display();
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window
	win = Engine::instance()->createWindow( WindowSettings( 960, 640, "eepp - Fonts" ), ContextSettings( true ) );

	// Set window background color
	win->setBackColor( RGB(255,255,255) );

	// Check if created
	if ( win->isOpen() ) {
		// Get the application path
		std::string AppPath = Sys::getProcessPath();

		// Save the TTF font so then it can be loaded as a TextureFont
		//TTF->save( AppPath + "assets/temp/DejaVuSansMono.png", AppPath + "assets/temp/DejaVuSansMono.fnt" );

		// Create a new text string
		String Txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

		fontTest = FontTrueType::New( "DejaVuSansMono" );
		fontTest->loadFromFile( AppPath + "assets/fonts/DejaVuSansMono.ttf" );
		fontTest->shrinkText( Txt, 24, false, 2, win->getWidth() - 96 );
		text.setFont( fontTest );
		text.setCharacterSize( 24 );
		text.setFillColor( 0xFFFFFFFF );
		text.setOutlineThickness( 2 );
		text.setFlags( FONT_DRAW_CENTER );
		text.setText( Txt );

		win->setBackColor( RGB(230,230,230) );

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
