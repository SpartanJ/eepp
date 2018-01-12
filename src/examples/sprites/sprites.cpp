#include <eepp/ee.hpp>

EE::Window::Window * win = NULL;

// Define a interpolation to control the Rock sprite angle
Interpolation1d RockAngle;

Interpolation1d PlanetAngle;

// Create a primitive drawer instance to draw the AABB of the Rock
Primitives P;
Sprite Rock;
Sprite Planet;
Sprite Blindy;

// Define a user sprite event
static const Uint32 USER_SPRITE_EVENT = Sprite::SPRITE_EVENT_USER + 1;

// Get the sprite event callback
void spriteCallback( Uint32 Event, Sprite * Sprite, void * UserData ) {
	// Sprite Animation entered the first frame?
	if ( Event == Sprite::SPRITE_EVENT_FIRST_FRAME ) {
		// Fire a user Event
		Sprite->fireEvent( USER_SPRITE_EVENT );
	} else if ( Event == USER_SPRITE_EVENT ) {
		// Create an interpolation to change the angle of the sprite
		Interpolation1d * RotationInterpolation = reinterpret_cast<Interpolation1d*>( UserData );
		RotationInterpolation->clear();
		RotationInterpolation->add( Sprite->getRotation() );
		RotationInterpolation->add( Sprite->getRotation() + 45.f );
		RotationInterpolation->setDuration( Milliseconds( 500 ) );
		RotationInterpolation->setType( Ease::BounceOut ); // Set the easing effect used for the interpolation
		RotationInterpolation->start();

		// Scale the sprite
		if ( Sprite->getScale().x < 3 ) {
			Sprite->setScale( Sprite->getScale() + 0.25f );
		}
	}
}

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

	// Check if the D key was pressed
	if ( win->getInput()->isKeyUp( KEY_D ) ) {
		// Reverse the Rock animation
		Rock.setReverseAnimation( !Rock.getReverseAnimation() );
	}

	// Update the angle interpolation
	PlanetAngle.update( win->getElapsed() );
	RockAngle.update( win->getElapsed() );

	// Set the Planet and Rock angle from the interpolation
	Planet.setRotation( PlanetAngle.getPosition() );
	Rock.setRotation( RockAngle.getPosition() );

	// Draw the static planet sprite
	Planet.draw();

	// Draw the animated Rock sprite
	Rock.draw();

	// Draw the blindy animation
	Blindy.draw();

	// Draw the Rock Axis-Aligned Bounding Box
	P.setColor( Color( 255, 255, 255, 255 ) );
	P.drawRectangle( Rock.getAABB() );

	// Draw the Rock Quad
	P.setColor( Color( 255, 0, 0, 255 ) );
	P.drawQuad( Rock.getQuad() );

	// Draw frame
	win->display();
}

EE_MAIN_FUNC int main (int argc, char * argv [])
{
	// Create a new window
	win = Engine::instance()->createWindow( WindowSettings( 640, 480, "eepp - Sprites" ), ContextSettings( true ) );

	// Check if created
	if ( win->isOpen() ) {
		// Set the application current directory path
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		// Load the rock texture
		Uint32 PlanetId	= TextureFactory::instance()->loadFromFile( "assets/sprites/7.png" );
		Uint32 RockId	= TextureFactory::instance()->loadFromFile( "assets/sprites/5.png" );

		// Load a previously generated texture atlas that contains the TextureRegions needed to load an animated sprite
		TextureAtlasLoader Blindies( "assets/atlases/bnb.eta" );

		// Create the animated rock spriteR
		// Load the rock frames from the texture, adding the frames manually
		for ( Int32 my = 0; my < 4; my++ ) {
			for( Int32 mx = 0; mx < 8; mx++ ) {
				// DestSize as 0,0 will use the TextureRegion size
				Rock.addFrame( RockId, Sizef( 0, 0 ), Vector2i( 0, 0 ), Rect( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );
			}
		}

		// Create a static sprite
		Planet.createStatic( PlanetId );

		// It will look for a TextureRegion ( in any Texture Atlas loaded, or the GlobalTextureAtlas ) animation by its name, it will search
		// for "gn00" to "gnXX" to create a new animation
		// see TextureAtlasManager::GetTextureRegionsByPattern for more information.
		// This is the easiest way to load animated sprites.
		Blindy.addFramesByPattern( "gn" );

		// Set the sprite animation speed, set in Frames per Second
		// Sprites are auto-animated by default.
		Rock.setAnimationSpeed( 32 );

		// Set the render mode of the sprite
		Blindy.setRenderMode( RENDER_MIRROR );

		// Set the Blend Mode of the sprite
		Blindy.setBlendMode( BlendAdd );

		// Set the primitive fill mode
		P.setFillMode( DRAW_LINE );

		// Set the sprites position to the screen center
		Vector2i ScreenCenter( Engine::instance()->getCurrentWindow()->getWidth() / 2, Engine::instance()->getCurrentWindow()->getHeight() / 2 );

		Planet.setPosition( Vector2f( ScreenCenter.x - Planet.getAABB().getSize().getWidth() / 2,
									  ScreenCenter.y - Planet.getAABB().getSize().getHeight() / 2 ) );

		Rock.setPosition( Vector2f( ScreenCenter.x - Rock.getAABB().getSize().getWidth() / 2,
						  ScreenCenter.y - Rock.getAABB().getSize().getHeight() / 2 ) );

		Blindy.setPosition( Vector2f( ScreenCenter.x - Blindy.getAABB().getSize().getWidth() / 2,
							ScreenCenter.y - Blindy.getAABB().getSize().getHeight() / 2 ) );

		// Set the planet angle interpolation
		PlanetAngle.add( 0 );
		PlanetAngle.add( 360 );
		PlanetAngle.setDuration( Seconds( 10 ) );
		PlanetAngle.setLoop( true );
		PlanetAngle.start();

		// Create a Event callback for the rock sprite
		Rock.setEventsCallback( cb::Make3( &spriteCallback ), &RockAngle );

		// Application loop
		win->runMainLoop( &mainLoop );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
