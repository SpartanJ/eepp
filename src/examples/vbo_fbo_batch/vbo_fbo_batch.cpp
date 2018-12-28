#include <eepp/ee.hpp>

EE::Window::Window * win			= NULL;
VertexBuffer * VBO		= NULL;
VertexBuffer * VBO2	= NULL;
FrameBuffer * FBO		= NULL;

// The batch renderer class is designed to take control of almost all the rendering needed by the engine.
// Controls that the rendering is only done when is needed, preventing redundant OpenGL API calls
// Usually the user will not need to use this class manually, since eepp controls this internally.
// The engine uses the singleton class GlobalBatchRenderer instance to render textures and primitives.
BatchRenderer * Batch = BatchRenderer::New();

Float ang = 0, scale = 1;
bool side = false;

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

	// Bind the Frame Buffer, everything rendered from here will be rendered in the frame buffer
	FBO->bind();
	{
		// Bind the buffered data ( activate the buffer )
		VBO->bind();

		// Draw the buffered data
		VBO->draw();

		// Unbind the buffered data
		VBO->unbind();

		// Same as above
		VBO2->bind();
		VBO2->draw();
		VBO2->unbind();
	}
	// Unbind the frame buffer. Stops rendering to the frame buffer
	FBO->unbind();

	// Draw the frame buffer many times
	for ( int y = 0; y < 5; y++ ) {
		for ( int x = 0; x < 5; x++ ) {
			FBO->getTexture()->draw( x * 200, y * 200, -ang, Vector2f::One, Color(255,255,255,100) );
		}
	}

	Float HWidth	= win->getWidth() * 0.5f;
	Float HHeight	= win->getHeight() * 0.5f;

	// The batch can be rotated, scale and moved
	Batch->setBatchRotation( ang );
	Batch->setBatchScale( scale );
	Batch->setBatchCenter( Vector2f( HWidth, HHeight ) );

	// Create a quad to render
	Float aX = HWidth - 256.f;
	Float aY = HHeight - 256.f;
	Quad2f TmpQuad(
		Vector2f( aX	   , aY 		),
		Vector2f( aX	   , aY + 32.f  ),
		Vector2f( aX + 32.f, aY + 32.f  ),
		Vector2f( aX + 32.f, aY 		)
	);
	TmpQuad.rotate( ang, Vector2f( aX + 16.f, aY + 16.f ) );

	// Begin drawing quads
	Batch->quadsBegin();

	// Add some quads to the batch renderer
	for ( Uint32 z = 0; z < 16; z++ ) {
		for ( Uint32 y = 0; y < 16; y++ ) {
			Float tmpx = (Float)z * 32.f;
			Float tmpy = (Float)y * 32.f;

			// Add the quad to the batch
			Batch->quadsSetColor( Color( z * 16, 255, 255, 150 ) );
			Batch->batchQuadFree( TmpQuad[0].x + tmpx, TmpQuad[0].y + tmpy, TmpQuad[1].x + tmpx, TmpQuad[1].y + tmpy, TmpQuad[2].x + tmpx, TmpQuad[2].y + tmpy, TmpQuad[3].x + tmpx, TmpQuad[3].y + tmpy );
		}
	}

	// Draw the batched quads
	Batch->draw();

	// Add the rotation angle
	ang+=win->getElapsed().asMilliseconds() * 0.1f;
	ang = (ang>=360) ? 0 : ang;

	// Change the scale value
	if (scale>=1.5f) {
		scale = 1.5f;
		side = true;
	} else if (scale<=0.5f) {
		side = false;
		scale = 0.5f;
	}
	scale = (!side) ? scale+win->getElapsed().asMilliseconds() * 0.00025f : scale-win->getElapsed().asMilliseconds() * 0.00025f;

	// Draw frame
	win->display();
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window
	win = Engine::instance()->createWindow( WindowSettings( 1024, 768, "eepp - VBO - FBO and Batch Rendering" ), ContextSettings( true ) );

	// Set window background color
	win->setClearColor( RGB( 50, 50, 50 ) );

	// Check if created
	if ( win->isOpen() ) {
		Polygon2f Poly( Polygon2f::createRoundedRectangle( 0, 0, 200, 50 ) );

		// Create the Vertex Buffer, the vertex buffer stores the vertex data in the GPU, making the rendering much faster
		// In the case that Vertex Buffer Object is not supported by the GPU, it will fallback to a inmediate-mode vertex buffer
		VBO		= VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN );
		VBO2	= VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN );

		// Add the vertex and vertex colors to the Vertex Buffer
		if ( NULL != VBO && NULL != VBO2 ) {
			for ( Uint32 i = 0; i < Poly.getSize(); i++ ) {
				VBO->addVertex( Poly[i] );
				VBO->addColor( Color( 100 + i, 255 - i, 150 + i, 100 ) );
			}

			Poly.rotate( 90, Poly.getBounds().getCenter() );

			for ( Uint32 i = 0; i < Poly.getSize(); i++ ) {
				VBO2->addVertex( Poly[i] );
				VBO2->addColor( Color( 100 + i, 255 - i, 150 + i, 100 ) );
			}

			// Compile the Vertex Buffer, this uploads the data to the GPU
			VBO->compile();
			VBO2->compile();
		}

		// Create a new frame buffer. It will use Framebuffer Objects if available, otherwise it will try to fallback to PBuffers.
		FBO = FrameBuffer::New( 200, 200 );

		// Application loop
		win->runMainLoop( &mainLoop );

		// Release the allocated objects ( VBOs and FBOs need to be released manually )
		eeSAFE_DELETE( VBO );
		eeSAFE_DELETE( VBO2 );
		eeSAFE_DELETE( FBO );
		eeSAFE_DELETE( Batch );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
