#include <eepp/ee.hpp>

EE_MAIN_FUNC int main( int, char*[] ) {
	// Create a new window
	auto win = Engine::instance()->createWindow( WindowSettings( 640, 480, "eepp - Sprites" ),
											  ContextSettings( true ) );

	// Check if created
	if ( win->isOpen() ) {
		// Change the current working directory to the binary path to ensure the assets location
		// is always correct even if we load the application from other directory than the binary
		// path.
		FileSystem::changeWorkingDirectory( Sys::getProcessPath() );

		// Keep all graphics resources in this scope so they are released before Engine shutdown.
		TexturePtr PlanetTex = TextureFactory::instance()->loadFromFile( "assets/sprites/7.png" );
		TexturePtr RockTex = TextureFactory::instance()->loadFromFile( "assets/sprites/5.png" );
		Sprite Rock;
		Sprite Planet;
		Sprite Monster;
		Interpolation1d RockAngle;
		Interpolation1d PlanetAngle;
		Primitives P;

		// Load a previously generated texture atlas that contains the TextureRegions needed to load
		// an animated sprite.
		TextureAtlasLoader Blindies( "assets/atlases/bnb.eta" );

		// Create the animated rock spriteR
		// Load the rock frames from the texture, adding the frames manually
		for ( Int32 my = 0; my < 4; my++ ) {
			for ( Int32 mx = 0; mx < 8; mx++ ) {
				// DestSize as 0,0 will use the TextureRegion size
				Rock.addFrame( RockTex, Sizef( 0, 0 ), Vector2i( 0, 0 ),
							   Rect( mx * 64, my * 64, mx * 64 + 64, my * 64 + 64 ) );
			}
		}

		// Create a static sprite
		Planet.createStatic( PlanetTex );

		// It will look for a TextureRegion animation in the default resource scope by its name. It
		// will search for "gn00" to "gnXX" to create a new animation. See
		// ResourceScope::findTextureRegionsByPattern for more information.
		// This is the easiest way to load animated sprites.
		Monster.addFramesByPattern( "gn" );

		// Set the sprite animation speed, set in Frames per Second
		// Sprites are auto-animated by default.
		Rock.setAnimationSpeed( 32 );

		// Set the render mode of the sprite
		Monster.setRenderMode( RENDER_MIRROR );

		// Set the Blend Mode of the sprite
		Monster.setBlendMode( BlendMode::Add() );

		// Set the primitive fill mode
		P.setFillMode( DRAW_LINE );

		// Set the sprites position to the screen center
		Vector2i ScreenCenter( Engine::instance()->getCurrentWindow()->getWidth() / 2,
							   Engine::instance()->getCurrentWindow()->getHeight() / 2 );

		Planet.setPosition(
			Vector2f( ScreenCenter.x - Planet.getAABB().getSize().getWidth() / 2,
					  ScreenCenter.y - Planet.getAABB().getSize().getHeight() / 2 ) );

		Rock.setPosition( Vector2f( ScreenCenter.x - Rock.getAABB().getSize().getWidth() / 2,
									ScreenCenter.y - Rock.getAABB().getSize().getHeight() / 2 ) );

		Monster.setPosition(
			Vector2f( ScreenCenter.x - Monster.getAABB().getSize().getWidth() / 2,
					  ScreenCenter.y - Monster.getAABB().getSize().getHeight() / 2 ) );

		// Set the planet angle interpolation
		PlanetAngle.add( 0 );
		PlanetAngle.add( 360 );
		PlanetAngle.setDuration( Seconds( 10 ) );
		PlanetAngle.setLoop( true );
		PlanetAngle.start();

		// Create a Event callback for the rock sprite
		Rock.setEventsCallback(
			[userSpriteEvent = static_cast<Uint32>( Sprite::SPRITE_EVENT_USER + 1 )](
				Uint32 event, Sprite* sprite, void* userData ) {
				if ( event == Sprite::SPRITE_EVENT_FIRST_FRAME ) {
					sprite->fireEvent( userSpriteEvent );
				} else if ( event == userSpriteEvent ) {
					auto* rotationInterpolation = static_cast<Interpolation1d*>( userData );
					rotationInterpolation->clear();
					rotationInterpolation->add( sprite->getRotation() );
					rotationInterpolation->add( sprite->getRotation() + 45.f );
					rotationInterpolation->setDuration( Milliseconds( 500 ) );
					rotationInterpolation->setType( Ease::BounceOut );
					rotationInterpolation->start();

					if ( sprite->getScale().x < 3 )
						sprite->setScale( sprite->getScale() + 0.25f );
				}
			},
			&RockAngle );

		// Application loop
		win->runMainLoop( [&] {
			win->clear();
			win->getInput()->update();

			if ( win->getInput()->isKeyDown( KEY_ESCAPE ) )
				win->close();

			if ( win->getInput()->isKeyUp( KEY_D ) )
				Rock.setReverseAnimation( !Rock.getReverseAnimation() );

			PlanetAngle.update( win->getElapsed() );
			RockAngle.update( win->getElapsed() );
			Planet.setRotation( PlanetAngle.getPosition() );
			Rock.setRotation( RockAngle.getPosition() );

			Planet.draw();
			Rock.draw();
			Monster.draw();

			P.setColor( Color( 255, 255, 255, 255 ) );
			P.drawRectangle( Rock.getAABB() );
			P.setColor( Color( 255, 0, 0, 255 ) );
			P.drawQuad( Rock.getQuad() );

			win->display();
		} );
	}

	// Destroy the engine instance. Destroys all the windows and engine singletons.
	Engine::destroySingleton();

	// If was compiled in debug mode it will print the memory manager report
	MemoryManager::showResults();

	return EXIT_SUCCESS;
}
