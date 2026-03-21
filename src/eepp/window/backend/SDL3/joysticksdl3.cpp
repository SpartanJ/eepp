#include <eepp/window/backend/SDL3/joysticksdl3.hpp>

#ifdef EE_BACKEND_SDL3

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

JoystickSDL::JoystickSDL( const Uint32& index, SDL_JoystickID id ) : Joystick( index ), mJoystick( nullptr ), mId( id ) {
	open();
}

JoystickSDL::~JoystickSDL() {
	close();
}

void JoystickSDL::open() {
	mJoystick = SDL_OpenJoystick( mId );
	if ( nullptr != mJoystick ) {
		mName = SDL_GetJoystickName( mJoystick );
		mHats = SDL_GetNumJoystickHats( mJoystick );
		mButtons = eemin( SDL_GetNumJoystickButtons( mJoystick ), 32 );
		mAxes = SDL_GetNumJoystickAxes( mJoystick );
		mBalls = SDL_GetNumJoystickBalls( mJoystick );
		mButtonDown = mButtonDownLast = mButtonUp = 0;
	}
}

void JoystickSDL::close() {
	if ( nullptr != mJoystick )
		SDL_CloseJoystick( mJoystick );
	mJoystick = nullptr;
	mName = "";
	mHats = mButtons = mAxes = mBalls = 0;
}

void JoystickSDL::update() {
	if ( nullptr != mJoystick ) {
		clearStates();
		for ( Int32 i = 0; i < mButtons; i++ ) {
			updateButton( i, 0 != SDL_GetJoystickButton( mJoystick, i ) );
		}
	}
}

Uint8 JoystickSDL::getHat( const Int32& index ) {
	if ( index >= 0 && index < mHats )
		return SDL_GetJoystickHat( mJoystick, index );
	return HAT_CENTERED;
}

Float JoystickSDL::getAxis( const Int32& axis ) {
	if ( axis >= 0 && axis < mAxes ) {
		return static_cast<Float>( SDL_GetJoystickAxis( mJoystick, axis ) ) / 32768.0f;
	}
	return 0;
}

Vector2i JoystickSDL::getBallMotion( const Int32& ball ) {
	Vector2i v;
	if ( ball >= 0 && ball < mBalls )
		SDL_GetJoystickBall( mJoystick, ball, &v.x, &v.y );
	return v;
}

bool JoystickSDL::isPlugged() const {
	return nullptr != mJoystick;
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
