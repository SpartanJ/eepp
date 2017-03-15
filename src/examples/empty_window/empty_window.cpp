#include <eepp/ee.hpp>

#define GL_STENCIL_TEST				0x0B90
#define GL_EQUAL				0x0202
#define GL_NEVER				0x0200
#define GL_KEEP					0x1E00
#define GL_REPLACE				0x1E01

EE::Window::Window * win = NULL;
float circ = 0, circ2 = 0;
int op = 1;

void mainLoop()
{
	// Clear the screen buffer
	win->clear();

	// Create an instance of the primitive renderer
	Primitives p;

	// Change the color
	p.setColor( ColorA( 0, 255, 0, 150 ) );

	// Update the input
	win->getInput()->update();

	// Check if ESCAPE key is pressed
	if ( win->getInput()->isKeyDown( KEY_ESCAPE ) ) {
		// Close the window
		win->close();
	}

	circ += win->getElapsed().asMilliseconds() * 0.5f * op;
	circ2 += win->getElapsed().asMilliseconds() * 0.75f;

	if ( op == 1 && circ > 340 )
	{
		op = -1;
	}
	else if ( op == -1 && circ < 20 )
	{
		op = 1;
	}

	Vector2f winCenter( win->getWidth() * 0.5f, win->getHeight() * 0.5f );


	GLi->enable(GL_STENCIL_TEST);
	GLi->stencilMask(0xFF);
	GLi->stencilFunc(GL_NEVER, 1, 0xFF);
	GLi->stencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

	p.drawCircle( winCenter, 150, 40 );

	GLi->stencilFunc(GL_EQUAL, 0, 0xFF);

	// Draw a circle
	p.drawArc( winCenter, 200, 40, circ, circ2 );

	GLi->disable(GL_STENCIL_TEST);

/*
	GLi->Enable(GL_STENCIL_TEST);

	GLi->StencilFunc(GL_ALWAYS, 1, 1);
	GLi->ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	GLi->StencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);

	p.SetColor( ColorA( 255, 255, 255, 255 ) );
	p.DrawCircle( winCenter, 150, 40 );

	GLi->StencilFunc(GL_NOTEQUAL, 1, 1);
	GLi->StencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	GLi->ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	// Draw a circle
	p.SetColor( ColorA( 0, 255, 0, 150 ) );
	p.DrawArc( winCenter, 200, 40, circ, circ2 );

	GLi->Disable(GL_STENCIL_TEST);
*/

	// Draw frame
	win->display();
}

// EE_MAIN_FUNC is needed by some platforms to be able to find the real application main
EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window with vsync enabled
	win = Engine::instance()->createWindow( WindowSettings( 960, 640, "eepp - Empty Window" ), ContextSettings( true ) );

	// Check if created
	if ( win->isOpen() ) {
		// Set window background color
		win->setClearColor( RGB( 50, 50, 50 ) );

		GLi->polygonMode( );

		// Set the MainLoop function and run it
		// This is the application loop, it will loop until the window is closed.
		// This is only a requirement if you want to support Emscripten builds ( WebGL + Canvas ).
		// This is the same as, except for Emscripten.
		// while ( win->Running() )
		// {
		//		MainLoop();
		// }
		win->runMainLoop( &mainLoop );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
