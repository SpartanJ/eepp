#include <eepp/window/backend/SDL/joystickmanagersdl.hpp>
#include <eepp/window/backend/SDL/joysticksdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

#if !defined( EE_COMPILER_MSVC )
#include <SDL/SDL.h>
#else
#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL {

JoystickManagerSDL::JoystickManagerSDL() :
	JoystickManager()
{
}

JoystickManagerSDL::~JoystickManagerSDL() {
}

void JoystickManagerSDL::Update() {
	if ( mInit ) {
		SDL_JoystickUpdate();

		for ( Uint32 i = 0; i < mCount; i++ )
			if ( NULL != mJoysticks[i] )
				mJoysticks[i]->Update();
	}
}

void JoystickManagerSDL::Open() {
	int error = SDL_InitSubSystem( SDL_INIT_JOYSTICK );

	if ( !error ) {
		mCount = SDL_NumJoysticks();

		for ( Uint32 i = 0; i < mCount; i++ )
			Create(i);

		mInit = true;
	}
}

void JoystickManagerSDL::Close() {
	if ( SDL_WasInit( SDL_INIT_JOYSTICK ) ) {
		SDL_QuitSubSystem( SDL_INIT_JOYSTICK );

		mInit = false;
	}
}

void JoystickManagerSDL::Create( const Uint32& index ) {
	if ( NULL != mJoysticks[ index ] )
		mJoysticks[ index ]->ReOpen();
	else
		mJoysticks[ index ] = eeNew( JoystickSDL, ( index ) );
}

}}}}

#endif
