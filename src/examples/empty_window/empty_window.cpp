#include <eepp/ee.hpp>

EE::Window::Window * win = NULL;
float circ = 0, circ2 = 0;
int op = 1;
ArcDrawable arcDrawable( 200, 64 );
CircleDrawable circleDrawableMask( 150, 64 );

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

	ClippingMask * clippingMask = Renderer::instance()->getClippingMask();

	circleDrawableMask.setPosition( winCenter );

	clippingMask->setMaskMode( ClippingMask::Exclusive );
	clippingMask->clearMasks();
	clippingMask->appendMask( circleDrawableMask );
	clippingMask->stencilMaskEnable();

	arcDrawable.setArcAngle( circ );
	arcDrawable.setArcStartAngle( circ2 );
	arcDrawable.draw( winCenter );

	clippingMask->stencilMaskDisable();

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
		win->setClearColor( Color( 50, 50, 50 ) );

		arcDrawable.setColor( ColorA( 0, 255, 0, 150 ) );
		arcDrawable.setFillMode( DRAW_FILL );

		circleDrawableMask.setColor( ColorA( 0, 255, 0, 150 ) );
		circleDrawableMask.setFillMode( DRAW_FILL );

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
