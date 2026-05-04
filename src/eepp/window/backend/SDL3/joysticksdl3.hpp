#ifndef EE_WINDOWCJOYSTICKSDL3_HPP
#define EE_WINDOWCJOYSTICKSDL3_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL3/base.hpp>

#ifdef EE_BACKEND_SDL3

#include <eepp/window/joystick.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API JoystickSDL : public Joystick {
  public:
	JoystickSDL( const Uint32& index, SDL_JoystickID id );

	virtual ~JoystickSDL();

	void close();

	void open();

	void update();

	Uint8 getHat( const Int32& index );

	Float getAxis( const Int32& axis );

	Vector2i getBallMotion( const Int32& ball );

	bool isPlugged() const;

  protected:
	SDL_Joystick* mJoystick;
	SDL_JoystickID mId;
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
#endif
