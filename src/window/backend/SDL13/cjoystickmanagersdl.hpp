#ifndef EE_WINDOWCJOYSTICKMANAGERSDL13_HPP
#define EE_WINDOWCJOYSTICKMANAGERSDL13_HPP

#include "../../cbackend.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL_1_3

#include "../../cjoystickmanager.hpp"

namespace EE { namespace Window { namespace Backend { namespace SDL13 {

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
