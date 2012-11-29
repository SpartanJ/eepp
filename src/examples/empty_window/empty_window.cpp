#include <eepp/ee.hpp>

// EE_MAIN_FUNC is needed for some platforms to export the main function as C function.
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window
	cWindow * win = cEngine::instance()->CreateWindow( WindowSettings( 960, 640, 32, WindowStyle::Default, "", "eepp - Empty Window" ), ContextSettings(  ) );

	// Set window background color
	win->BackColor( eeColor( 50, 50, 50 ) );

	// Check if created
	if ( win->Created() )
	{
		// Get input pointer
		cInput * imp = win->GetInput();

		eeFloat ang = 0;

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

			ang += cEngine::instance()->Elapsed() * 0.01;

			// Create an instance of the primitive renderer
			cPrimitives p;

			// Set the primitive color
			p.SetColor( eeColorA( 0, 150, 0, 150 ) );

			// Draw a rectangle
			p.DrawRectangle( 100, 100, win->GetWidth() - 200, win->GetHeight() - 200, ang );

			// Change the color
			p.SetColor( eeColorA( 0, 255, 0, 150 ) );

			// Draw a circle
			p.DrawCircle( win->GetWidth() / 2, win->GetHeight() / 2, 200 );

			// Draw frame
			win->Display();

			// Sleep thread for 10 ms
			Sys::Sleep( 10 );
		}
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	cEngine::DestroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	EE::MemoryManager::LogResults();

	return 0;
}
