#include <eepp/window/backend/SDL2/joysticksdl2.hpp>

#ifdef EE_BACKEND_SDL2

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS &&  \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && !defined( EE_COMPILER_MSVC ) && \
	!defined( EE_SDL2_FROM_ROOTPATH )
#include <SDL2/SDL_revision.h>
#else
#include <SDL_revision.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

JoystickSDL::JoystickSDL( const Uint32& index ) : Joystick( index ), mJoystick( nullptr ) {
	open();
}

JoystickSDL::~JoystickSDL() {
	close();
}

void JoystickSDL::open() {
	mJoystick = SDL_JoystickOpen( mIndex );

	if ( nullptr != mJoystick ) {
		mName = SDL_JoystickName( mJoystick );
		mHats = SDL_JoystickNumHats( mJoystick );
		mButtons = eemin( SDL_JoystickNumButtons( mJoystick ), 32 );
		mAxes = SDL_JoystickNumAxes( mJoystick );
		mBalls = SDL_JoystickNumBalls( mJoystick );

		mButtonDown = mButtonDownLast = mButtonUp = 0;
	}
}

void JoystickSDL::close() {
	if ( nullptr != mJoystick )
		SDL_JoystickClose( mJoystick );

	mJoystick = nullptr;
	mName = "";
	mHats = mButtons = mAxes = mBalls = 0;
}

void JoystickSDL::update() {
	if ( nullptr != mJoystick ) {
		clearStates();

		for ( Int32 i = 0; i < mButtons; i++ ) {
			updateButton( i, 0 != SDL_JoystickGetButton( mJoystick, i ) );
		}
	}
}

Uint8 JoystickSDL::getHat( const Int32& index ) {
	if ( index >= 0 && index < mHats )
		return SDL_JoystickGetHat( mJoystick, index );

	return HAT_CENTERED;
}

Float JoystickSDL::getAxis( const Int32& axis ) {
	if ( axis >= 0 && axis < mAxes ) {
		return (Float)SDL_JoystickGetAxis( mJoystick, axis ) / 32768.f;
	}

	return 0;
}

Vector2i JoystickSDL::getBallMotion( const Int32& ball ) {
	Vector2i v;

	if ( ball >= 0 && ball < mBalls )
		SDL_JoystickGetBall( mJoystick, ball, &v.x, &v.y );

	return v;
}

bool JoystickSDL::isPlugged() const {
	return nullptr != mJoystick;
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
