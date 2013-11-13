#include <eepp/ee.hpp>

cWindow * win = NULL;

void MainLoop()
{
	win->Clear();

	// Create an instance of the primitive renderer
	cPrimitives p;

	// Change the color
	p.SetColor( eeColorA( 0, 255, 0, 150 ) );

	// Update the input
	win->GetInput()->Update();

	// Check if ESCAPE key is pressed
	if ( win->GetInput()->IsKeyDown( KEY_ESCAPE ) ) {
		// Close the window
		win->Close();
	}

	// Draw a circle
	p.DrawCircle( eeVector2f( win->GetWidth() * 0.5f, win->GetHeight() * 0.5f ), 200 );

	// Draw frame
	win->Display(false);
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window with vsync enabled
	win = cEngine::instance()->CreateWindow( WindowSettings( 960, 640, "eepp - Empty Window" ), ContextSettings( true ) );

	// Check if created
	if ( win->Created() ) {
		// Set window background color
		win->BackColor( eeColor( 50, 50, 50 ) );

		win->RunMainLoop( &MainLoop, 60 );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	cEngine::DestroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
