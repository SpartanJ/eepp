#ifndef EE_BACKEND_SDL2_WMINFO_HPP
#define EE_BACKEND_SDL2_WMINFO_HPP

#include <eepp/window/backend/SDL2/base.hpp>
#include <eepp/window/windowhandle.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class WMInfo {
	public:
		WMInfo( SDL_Window * win );

		~WMInfo();

		#if defined( EE_X11_PLATFORM )
		X11Window GetWindow();
		#endif

		eeWindowHandle GetWindowHandler();
	protected:
		void * mWMInfo;
};

}}}}

#endif

