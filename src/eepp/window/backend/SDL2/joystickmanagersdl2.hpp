#ifndef EE_WINDOWCJOYSTICKMANAGERSDL2_HPP
#define EE_WINDOWCJOYSTICKMANAGERSDL2_HPP

#include <eepp/window/backend.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

#include <eepp/window/joystickmanager.hpp>
#include <eepp/system/thread.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API JoystickManagerSDL : public JoystickManager {
	public:
		JoystickManagerSDL();
		
		virtual ~JoystickManagerSDL();
		
		void Update();

		void Close();

		void Open();
	protected:
		void Create( const Uint32& index );

		void OpenAsync();

		Thread mAsyncInit;
};

}}}}

#endif

#endif
