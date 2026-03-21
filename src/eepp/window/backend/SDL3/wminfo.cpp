#include <eepp/window/backend/SDL3/wminfo.hpp>

#ifdef EE_BACKEND_SDL3

#include <SDL3/SDL.h>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

WMInfo::WMInfo( SDL_Window* win ) : mProps( SDL_GetWindowProperties( win ) ) {}

WMInfo::~WMInfo() {}

#if defined( EE_X11_PLATFORM )
X11Window WMInfo::getWindow() const {
	return 0;
}
#endif

eeWindowHandle WMInfo::getWindowHandler() const {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return (eeWindowHandle)SDL_GetPointerProperty( mProps, SDL_PROP_WINDOW_WIN32_HWND_POINTER, 0 );
#elif defined( EE_X11_PLATFORM )
	return (eeWindowHandle)SDL_GetNumberProperty( mProps, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0 );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return (eeWindowHandle)SDL_GetPointerProperty( mProps, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER,
												   0 );
#else
	return 0;
#endif
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
