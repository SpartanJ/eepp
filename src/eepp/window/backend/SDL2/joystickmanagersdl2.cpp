#include <eepp/window/backend/SDL2/joystickmanagersdl2.hpp>
#include <eepp/window/backend/SDL2/joysticksdl2.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

JoystickManagerSDL::JoystickManagerSDL() :
	JoystickManager(),
	mAsyncInit( &JoystickManagerSDL::openAsync, this )
{
}

JoystickManagerSDL::~JoystickManagerSDL() {
}

void JoystickManagerSDL::update() {
	if ( mInit ) {
		SDL_JoystickUpdate();

		for ( Uint32 i = 0; i < mCount; i++ )
			if ( NULL != mJoysticks[i] )
				mJoysticks[i]->update();
	}
}

void JoystickManagerSDL::openAsync() {
	Sys::sleep(Milliseconds(500));

	int error = SDL_InitSubSystem( SDL_INIT_JOYSTICK );

	if ( !error ) {
		mCount = SDL_NumJoysticks();

		for ( Uint32 i = 0; i < mCount; i++ )
			create(i);

		mInit = true;
	}
}

void JoystickManagerSDL::open() {
	mAsyncInit.launch();
}

void JoystickManagerSDL::close() {
	if ( SDL_WasInit( SDL_INIT_JOYSTICK ) ) {
		SDL_QuitSubSystem( SDL_INIT_JOYSTICK );
		
		mInit = false;
	}
}

void JoystickManagerSDL::create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->reOpen();
	else
		mJoysticks[ index ] = eeNew( JoystickSDL, ( index ) );
}

}}}}

#endif
