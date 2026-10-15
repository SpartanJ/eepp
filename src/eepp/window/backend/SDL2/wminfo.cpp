#include <eepp/window/backend/SDL2/wminfo.hpp>

#ifdef EE_BACKEND_SDL2

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOS ||    \
	( defined( SDL_VIDEO_DRIVER_X11 ) && defined( EE_X11_PLATFORM ) ) ||     \
	defined( SDL_VIDEO_DRIVER_WAYLAND ) || EE_PLATFORM == EE_PLATFORM_IOS || \
	EE_PLATFORM_ANDROID == EE_PLATFORM
#define EE_WMINFO

#include <eepp/window/backend/SDL2/syswm.hpp>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

#ifdef EE_WMINFO
WMInfo::WMInfo( SDL_Window* win ) : mWMInfo( eeMalloc( sizeof( SDL_SysWMinfo ) ) ) {
	SDL_SysWMinfo* info = static_cast<SDL_SysWMinfo*>( mWMInfo );
	*info = {};
	SDL_VERSION( &info->version );
	if ( !SDL_GetWindowWMInfo( win, info ) )
		info->subsystem = SDL_SYSWM_UNKNOWN;
}
#else
WMInfo::WMInfo( SDL_Window* win ) : mWMInfo( nullptr ) {}
#endif

WMInfo::~WMInfo() {
#ifdef EE_WMINFO
	eeSAFE_FREE( mWMInfo );
#endif
}

#if defined( SDL_VIDEO_DRIVER_X11 ) && defined( EE_X11_PLATFORM )
X11Window WMInfo::getWindow() const {
	SDL_SysWMinfo* info = static_cast<SDL_SysWMinfo*>( mWMInfo );
	return info && info->subsystem == SDL_SYSWM_X11 ? info->info.x11.window : 0;
}
#endif

eeWindowHandle WMInfo::getWindowHandler() const {
#if EE_PLATFORM == EE_PLATFORM_WIN || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_MACOS
	SDL_SysWMinfo* info = static_cast<SDL_SysWMinfo*>( mWMInfo );
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
	return info && info->subsystem == SDL_SYSWM_WINDOWS ? info->info.win.window : 0;
#elif defined( SDL_VIDEO_DRIVER_X11 ) && defined( EE_X11_PLATFORM )
	return info && info->subsystem == SDL_SYSWM_X11 ? info->info.x11.display : 0;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return info && info->subsystem == SDL_SYSWM_COCOA ? info->info.cocoa.window : 0;
#else
	return 0;
#endif
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
