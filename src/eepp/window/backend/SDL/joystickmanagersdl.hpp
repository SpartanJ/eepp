#ifndef EE_WINDOWCJOYSTICKMANAGERSDL_HPP
#define EE_WINDOWCJOYSTICKMANAGERSDL_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/joystickmanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API JoystickManagerSDL : public JoystickManager {
	public:
		JoystickManagerSDL();

		virtual ~JoystickManagerSDL();

		void update();

		void close();

		void open();
	protected:
		void create( const Uint32& index );
};

}}}}

#endif

#endif
