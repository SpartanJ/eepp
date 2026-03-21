#ifndef EE_BACKEND_SDL3_WMINFO_HPP
#define EE_BACKEND_SDL3_WMINFO_HPP

#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/windowhandle.hpp>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

class EE_API WMInfo {
  public:
	WMInfo( SDL_Window* win );

	~WMInfo();

#if defined( EE_X11_PLATFORM )
	X11Window getWindow() const;
#endif

	eeWindowHandle getWindowHandler() const;

  protected:
	SDL_PropertiesID mProps;
};

}}}} // namespace EE::Window::Backend::SDL3

#endif
