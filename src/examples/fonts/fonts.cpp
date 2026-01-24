#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char*[] ) {
	EE::Window::Window* win = NULL;

	// Create a new window
	win = Engine::instance()->createWindow( WindowSettings( 1024, 768, "eepp - Fonts" ),
											ContextSettings( true ) );

	// Set window background color
	win->setClearColor( RGB( 230, 230, 230 ) );

	// Check if created
	if ( win->isOpen() ) {
		// Change the current working directory to the binary path to ensure the assets location
		// is always correct even if we load the application from other directory than the binary
		// path.
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		// Create a new text string
		String Txt( "Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod "
					"tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, "
					"quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo "
					"consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse "
					"cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non "
					"proident, sunt in culpa qui officia deserunt mollit anim id est laborum." );

		FontTrueType* fontTest = FontTrueType::New( "DejaVuSansMono" );
		fontTest->loadFromFile( "assets/fonts/DejaVuSansMono.ttf" );

		FontTrueType* fontTest2 = FontTrueType::New( "NotoSans-Regular" );
		fontTest2->loadFromFile( "assets/fonts/NotoSans-Regular.ttf" );

		FontTrueType* fontEmoji = FontTrueType::New( "NotoEmoji-Regular" );
		fontEmoji->loadFromFile( "assets/fonts/NotoEmoji-Regular.ttf" );

		FontTrueType* fontEmojiColor = FontTrueType::New( "NotoColorEmoji" );
		fontEmojiColor->loadFromFile( "assets/fonts/NotoColorEmoji.ttf" );

		FontBMFont* fontBMFont = FontBMFont::New( "bmfont" );
		fontBMFont->loadFromFile( "assets/fonts/bmfont.fnt" );

		FontSprite* fontSprite = FontSprite::New(
			"alagard" ); // Alagard - Hewett Tsoi ( https://www.dafont.com/alagard.font )
		fontSprite->loadFromFile( "assets/fonts/custom_alagard.png", Color::Fuchsia, 32, -4 );

		Text text;
		text.setFont( fontTest );
		text.setFontSize( 24 );
		text.setAlign( TEXT_ALIGN_CENTER );
		text.setString( Txt );
		text.hardWrapText( win->getWidth() - 96 );

		// Set the font color to a substring of the text
		// Create a gradient
		int size = (int)text.getString().size();

		for ( int i = 0; i < size; i++ ) {
			text.setFillColor( Color( 255 * i / size, 0, 0, 255 ), i, i + 1 );
		}

		Text text2;
		text2.setFont( fontTest2 );
		text2.setString( "Lorem ipsum dolor sit amet, consectetur adipisicing elit. 👽" );
		text2.setFontSize( 32 );
		text2.setFillColor( Color::Black );

		Text text3;
		text3.setFont( fontTest );
		text3.setString( text2.getString() );
		text3.setFontSize( 24 );
		text3.setFillColor( Color( 255, 255, 255, 255 ) );
		text3.setOutlineThickness( 2 );
		text3.setOutlineColor( Color( 0, 0, 0, 255 ) );

		Text text4;
		text4.setFont( fontBMFont );
		text4.setString( text2.getString() );
		text4.setFontSize( 45 );
		text4.setFillColor( Color::Black );

		Text text5;
		text5.setFont( fontSprite );
		text5.setString( text2.getString() );
		text5.setFontSize( 38 );

		Text text6;
		text6.setFont( fontEmojiColor );
		text6.setFontSize( 64 );
		text6.setString( "👽 😀 💩 😃 👻" );

		Text text7;
		text7.setFont( fontEmoji );
		text7.setFontSize( 32 );
		text7.setString( "👽 😀 💩 😃 👻" );
		text7.setFillColor( Color::Gray );
		text7.setOutlineThickness( 2 );
		text7.setOutlineColor( Color( 0, 0, 0, 255 ) );

		// Application loop
		win->runMainLoop( [&] {
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

			text7.draw( ( win->getWidth() - text7.getTextWidth() ) * 0.5f, 400 );

			// Text rotated and scaled
			text2.draw( ( win->getWidth() - text2.getTextWidth() ) * 0.5f, 430,
						Vector2f( 1.1f, 1.1f ), 12.5f );

			text3.draw( ( win->getWidth() - text3.getTextWidth() ) * 0.5f, 560 );

			text4.draw( ( win->getWidth() - text4.getTextWidth() ) * 0.5f, 590 );

			text5.draw( ( win->getWidth() - text5.getTextWidth() ) * 0.5f, 640 );

			text6.draw( ( win->getWidth() - text6.getTextWidth() ) * 0.5f, 690 );

			// Draw frame
			win->display();
		} );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	// Fonts are autoreleased by the engine
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
