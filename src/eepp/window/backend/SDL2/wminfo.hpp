#ifndef EE_BACKEND_SDL2_WMINFO_HPP
#define EE_BACKEND_SDL2_WMINFO_HPP

#include <eepp/window/backend/SDL2/base.hpp>
#include <eepp/window/windowhandle.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class EE_API WMInfo {
  public:
	WMInfo( SDL_Window* win );

	~WMInfo();

#if defined( EE_X11_PLATFORM )
	X11Window getWindow();
#endif

	eeWindowHandle getWindowHandler();

  protected:
	void* mWMInfo;
};

}}}} // namespace EE::Window::Backend::SDL2

#endif
