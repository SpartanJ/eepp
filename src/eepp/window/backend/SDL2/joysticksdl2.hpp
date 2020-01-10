#ifndef EE_WINDOWCJOYSTICKSDL2_HPP
#define EE_WINDOWCJOYSTICKSDL2_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/joystick.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API JoystickSDL : public Joystick {
  public:
	JoystickSDL( const Uint32& index );

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
};

}}}} // namespace EE::Window::Backend::SDL2

#endif

#endif
