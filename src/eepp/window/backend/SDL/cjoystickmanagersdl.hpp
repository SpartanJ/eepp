#ifndef EE_WINDOWCJOYSTICKMANAGERSDL_HPP
#define EE_WINDOWCJOYSTICKMANAGERSDL_HPP

#include <eepp/window/cbackend.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

#include <eepp/window/cjoystickmanager.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API JoystickManagerSDL : public JoystickManager {
	public:
		JoystickManagerSDL();

		virtual ~JoystickManagerSDL();

		void Update();

		void Close();

		void Open();
	protected:
		void Create( const Uint32& index );
};

}}}}

#endif

#endif
