#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char*[] ) {
	// Create a new window
	auto* win = Engine::instance()->createWindow(
		WindowSettings( 1024, 768, "eepp - VBO - FBO and Batch Rendering" ),
		ContextSettings( true ) );

	// Set window background color
	win->setClearColor( RGB( 50, 50, 50 ) );

	// Check if created
	if ( win->isOpen() ) {
		auto VBO = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN );
		auto VBO2 = VertexBuffer::New( VERTEX_FLAGS_PRIMITIVE, PRIMITIVE_TRIANGLE_FAN );
		auto FBO = FrameBuffer::New( 200, 200 );
		auto* Batch = BatchRenderer::New();
		Float ang = 0;
		Float scale = 1;
		bool side = false;
		Polygon2f Poly( Polygon2f::createRoundedRectangle( 0, 0, 200, 50 ) );

		// Create the Vertex Buffer, the vertex buffer stores the vertex data in the GPU, making the
		// rendering much faster In the case that Vertex Buffer Object is not supported by the GPU,
		// it will fallback to a immediate-mode vertex buffer
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

		// Application loop
		win->runMainLoop( [&] {
			win->clear();
			win->getInput()->update();
			if ( win->getInput()->isKeyDown( KEY_ESCAPE ) )
				win->close();

			FBO->bind();
			VBO->bind();
			VBO->draw();
			VBO->unbind();
			VBO2->bind();
			VBO2->draw();
			VBO2->unbind();
			FBO->unbind();

			for ( int y = 0; y < 5; y++ )
				for ( int x = 0; x < 5; x++ )
					FBO->getTexture()->draw( x * 200, y * 200, -ang, Vector2f::One,
											 Color( 255, 255, 255, 100 ) );

			Float halfWidth = win->getWidth() * 0.5f;
			Float halfHeight = win->getHeight() * 0.5f;
			Batch->setBatchRotation( ang );
			Batch->setBatchScale( scale );
			Batch->setBatchCenter( Vector2f( halfWidth, halfHeight ) );

			Float x = halfWidth - 256.f;
			Float y = halfHeight - 256.f;
			Quad2f quad( Vector2f( x, y ), Vector2f( x, y + 32.f ), Vector2f( x + 32.f, y + 32.f ),
						 Vector2f( x + 32.f, y ) );
			quad.rotate( ang, Vector2f( x + 16.f, y + 16.f ) );

			Batch->quadsBegin();
			for ( Uint32 column = 0; column < 16; column++ ) {
				for ( Uint32 row = 0; row < 16; row++ ) {
					Float offsetX = static_cast<Float>( column ) * 32.f;
					Float offsetY = static_cast<Float>( row ) * 32.f;
					Batch->quadsSetColor( Color( column * 16, 255, 255, 150 ) );
					Batch->batchQuadFree( quad[0].x + offsetX, quad[0].y + offsetY,
										  quad[1].x + offsetX, quad[1].y + offsetY,
										  quad[2].x + offsetX, quad[2].y + offsetY,
										  quad[3].x + offsetX, quad[3].y + offsetY );
				}
			}
			Batch->draw();

			ang += win->getElapsed().asMilliseconds() * 0.1f;
			ang = ang >= 360 ? 0 : ang;
			if ( scale >= 1.5f ) {
				scale = 1.5f;
				side = true;
			} else if ( scale <= 0.5f ) {
				scale = 0.5f;
				side = false;
			}
			scale += ( side ? -1.f : 1.f ) * win->getElapsed().asMilliseconds() * 0.00025f;
			win->display();
		} );

		eeSAFE_DELETE( Batch );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
