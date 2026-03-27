#ifndef EE_WINDOW_BACKEND_HELPER_HPP
#define EE_WINDOW_BACKEND_HELPER_HPP

#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <string>

struct HWND__;
typedef HWND__* HWND;
#endif

namespace EE { namespace Window { namespace Backend {

class BackendHelper {
  public:
#if EE_PLATFORM == EE_PLATFORM_WIN
	static bool isProcessRunning( const char* processName, bool killProcess = false );

	static bool processLaunch( std::string command, HWND windowHwnd );

	static int showOSK( HWND windowHwnd );

	static int hideOSK();

	static bool isOSKActive();

	static bool isDarkModeEnabled();

	static void setUserTheme( HWND hwnd );
#elif defined( EE_X11_PLATFORM )
	static void showOSK();

	static void hideOSK();

	static bool isOSKActive();
#else
	static void hideOSK();

	static bool isOSKActive();
#endif
};

}}} // namespace EE::Window::Backend

#endif
