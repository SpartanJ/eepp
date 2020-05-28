#ifndef EE_WINDOWCINPUTSDL2_HPP
#define EE_WINDOWCINPUTSDL2_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/input.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API InputSDL : public Input {
  public:
	~InputSDL();

	void update();

	bool grabInput();

	void grabInput( const bool& Grab );

	void injectMousePos( const Uint16& x, const Uint16& y );

	Vector2i queryMousePos();

	void captureMouse( const bool& capture );

	bool isMouseCaptured() const;

	std::string getKeyName( const Keycode& keycode ) const;

	Keycode getKeyFromName( const std::string& keycode ) const;

	std::string getScancodeName( const Scancode& scancode ) const;

	Scancode getScancodeFromName( const std::string& scancode ) const;

	Keycode getKeyFromScancode( const Scancode& scancode ) const;

	Scancode getScancodeFromKey( const Keycode& scancode ) const;

  protected:
	friend class WindowSDL;
	Float mDPIScale;

	InputSDL( EE::Window::Window* window );

	void init();
};

}}}} // namespace EE::Window::Backend::SDL2

#endif

#endif
