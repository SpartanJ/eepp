#ifndef EE_WINDOWCJOYSTICKMANAGERSDL_HPP
#define EE_WINDOWCJOYSTICKMANAGERSDL_HPP

#include "../../cbackend.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

#include "../../cjoystickmanager.hpp"
#include <SDL/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL {

class EE_API cJoystickManagerSDL : public cJoystickManager {
	public:
		cJoystickManagerSDL();
		
		virtual ~cJoystickManagerSDL();
		
		void Update();

		void Close();

		void Open();
	protected:
		void Create( const Uint32& index );
};

}}}}

#endif

#endif
