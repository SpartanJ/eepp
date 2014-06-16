#include <eepp/ee.hpp>

EE::Window::Window * win = NULL;

void MainLoop()
{
	// Clear the screen buffer
	win->Clear();

	// Create an instance of the primitive renderer
	cPrimitives p;

	// Change the color
	p.SetColor( ColorA( 0, 255, 0, 150 ) );

	// Update the input
	win->GetInput()->Update();

	// Check if ESCAPE key is pressed
	if ( win->GetInput()->IsKeyDown( KEY_ESCAPE ) ) {
		// Close the window
		win->Close();
	}

	// Draw a circle
	p.DrawCircle( Vector2f( win->GetWidth() * 0.5f, win->GetHeight() * 0.5f ), 200, 50 );

	// Draw frame
	win->Display();
}

// EE_MAIN_FUNC is needed by some platforms to be able to find the real application main
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window with vsync enabled
	win = Engine::instance()->CreateWindow( WindowSettings( 960, 640, "eepp - Empty Window" ), ContextSettings( true ) );

	// Check if created
	if ( win->Created() ) {
		// Set window background color
		win->BackColor( RGB( 50, 50, 50 ) );

		// Set the MainLoop function and run it
		// This is the application loop, it will loop until the window is closed.
		// This is only a requirement if you want to support Emscripten builds ( WebGL + Canvas ).
		// This is the same as, except for Emscripten.
		// while ( win->Running() )
		// {
		//		MainLoop();
		// }
		win->RunMainLoop( &MainLoop );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	Engine::DestroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::ShowResults();

	return EXIT_SUCCESS;
}
