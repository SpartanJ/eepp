#include <eepp/ee.hpp>

EE::Window::Window * win			= NULL;
TTFFont * TTF			= NULL;
TTFFont * TTFO			= NULL;
TTFFont * TTF2			= NULL;
TextureFont * TexF		= NULL;
TextureFont * TexF2	= NULL;
TextCache TTFCache;
TextCache TTF2Cache;
TextCache TTFOCache;
TextCache TexFCache;
TextCache TexF2Cache;
TextCache TxtCache;

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

		// Create a new True Type Font
		TTF		= TTFFont::New( "DejaVuSansMonoOutline" );
		TTFO	= TTFFont::New( "DejaVuSansMonoOutlineFreetype" );
		TTF2	= TTFFont::New( "DejaVuSansMono" );
		TexF	= TextureFont::New( "ProggySquareSZ" );
		TexF2	= TextureFont::New( "conchars" );

		// Load the TTF font
		TTF->load( AppPath + "assets/fonts/DejaVuSansMono.ttf", 18, TTF_STYLE_NORMAL, 128, RGB(255,255,255), 3, RGB(0,0,0), true );

		// Change the default method to use for outlining the font glyphs
		TTFFont::OutlineMethod = TTFFont::OutlineFreetype;

		// Create the exact same font than before but using the new outlining method
		TTFO->load( AppPath + "assets/fonts/DejaVuSansMono.ttf", 18, TTF_STYLE_NORMAL, 128, RGB(255,255,255), 3, RGB(0,0,0), true );

		TTF2->load( AppPath + "assets/fonts/DejaVuSansMono.ttf", 24, TTF_STYLE_NORMAL, 128, RGB(255,255,255), 0, RGB(0,0,0), true );

		// Save the TTF font so then it can be loaded as a TextureFont
		TTF->save( AppPath + "assets/temp/DejaVuSansMono.png", AppPath + "assets/temp/DejaVuSansMono.fnt" );

		// Load the texture font, previusly generated from a True Type Font
		// First load the texture
		Uint32 TexFid = TextureFactory::instance()->load( AppPath + "assets/fonts/ProggySquareSZ.png" );
		TexF->load( TexFid,  AppPath + "assets/fonts/ProggySquareSZ.dat" );

		// Load a monospaced texture font from image ( using the texture loader to set the color key )
		TextureLoader TexLoader( AppPath + "assets/fonts/conchars.png" );
		TexLoader.setColorKey( RGB(0,0,0) );
		TexLoader.load();;
		TexF2->load( TexLoader.getId(), 32 );

		// Set the font to the text cache
		TTFCache.setFont( TTF );
		// Set a text to render
		TTFCache.setText( "Lorem ipsum dolor sit amet, consectetur adipisicing elit." );

		TTFOCache.setFont( TTFO );
		TTFOCache.setText( TTFCache.getText() );

		TTF2Cache.setFont( TTF2 );
		TTF2Cache.setText( TTFCache.getText() );

		// Set the font color
		TTF2Cache.setColor( RGB(0,0,0) );

		TexFCache.setFont( TexF );
		TexFCache.setText( TTFCache.getText() );
		TexFCache.setColor( RGB(0,0,0) );

		TexF2Cache.setFont( TexF2 );
		TexF2Cache.setText( TTFCache.getText() );

		// Create a new text string
		String Txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

		// Make the text fit the screen width ( wrap the text )
		TTF2->shrinkText( Txt, win->getWidth() - 96 );

		// Create a new text cache to draw on screen
		// The cached text will
		TxtCache.create( TTF2, Txt, ColorA(0,0,0,255) );

		// Set the text cache to be centered
		TxtCache.setFlags( FONT_DRAW_CENTER );

		// Set the font color to a substring of the text
		// To be able to set the color of the font, create the font as white
		// Create a gradient
		size_t size = TxtCache.getText().size();

		for ( size_t i = 0; i < size; i++ ) {
			TxtCache.setColor( ColorA(255*i/size,0,0,255), i, i+1 );
		}

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
