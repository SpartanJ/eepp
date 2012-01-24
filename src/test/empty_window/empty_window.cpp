#include "../../ee.h"

// EE_MAIN_FUNC is needed for some platforms to export the main function as C function.
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window
	cWindow * win = cEngine::instance()->CreateWindow( WindowSettings( 800, 600, 32, WindowStyle::Default, "", "eepp - Empty Window" ), ContextSettings(  ) );

	// Check if created
	if ( win->Created() )
	{
		// Get input pointer
		cInput * imp = win->GetInput();

		// Application loop
		while ( win->Running() )
		{
			// Update the input
			imp->Update();

			// Check if ESCAPE key is pressed
			if ( imp->IsKeyDown( KEY_ESCAPE ) )
			{
				// Close the window
				win->Close();
			}

			// Draw frame
			win->Display();

			// Sleep thread for 10 ms
			eeSleep( 10 );
		}
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	cEngine::DestroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	EE::MemoryManager::LogResults();

	return 0;
}
