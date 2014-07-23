#include <eepp/window/backend/SDL2/wminfo.hpp>

#ifdef EE_BACKEND_SDL2

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_MACOSX || defined( EE_X11_PLATFORM ) || EE_PLATFORM == EE_PLATFORM_IOS || EE_PLATFORM_ANDROID == EE_PLATFORM
	#if !defined( EE_COMPILER_MSVC ) && EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS && !defined( EE_SDL2_FROM_ROOTPATH )
	#include <SDL2/SDL_syswm.h>
	#else
	#include <SDL_syswm.h>
	#endif
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

WMInfo::WMInfo( SDL_Window * win ) :
	mWMInfo( eeNew( SDL_SysWMinfo, () ) )
{
	SDL_SysWMinfo * info = static_cast<SDL_SysWMinfo*> ( mWMInfo );
	SDL_VERSION( &info->version );
	SDL_GetWindowWMInfo ( win, info );
}

WMInfo::~WMInfo() {
	SDL_SysWMinfo * info = static_cast<SDL_SysWMinfo*> ( mWMInfo );
	eeSAFE_DELETE( info );
}

#if defined( EE_X11_PLATFORM )
X11Window WMInfo::GetWindow() {
	SDL_SysWMinfo * info = static_cast<SDL_SysWMinfo*> ( mWMInfo );
	return info->info.x11.window;
}
#endif

eeWindowHandle WMInfo::GetWindowHandler() {
	SDL_SysWMinfo * info = static_cast<SDL_SysWMinfo*> ( mWMInfo );
#if EE_PLATFORM == EE_PLATFORM_WIN
	return info->info.win.window;
#elif defined( EE_X11_PLATFORM )
	return info->info.x11.display;
#elif EE_PLATFORM == EE_PLATFORM_MACOSX
	return info->info.cocoa.window;
#else
	return 0;
#endif
}


}}}}

#endif
