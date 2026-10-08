#include <eepp/window/platformhelper.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#endif

namespace EE { namespace Window {

bool PlatformHelper::setNativeScrollMomentumEnabled( bool enabled ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::setNativeScrollMomentumEnabled( enabled );
#else
	(void)enabled;
	return false;
#endif
}

bool PlatformHelper::setWindowTitleBarSeparatorVisible( void* window, bool visible ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::setWindowTitleBarSeparatorVisible( window, visible );
#else
	(void)window;
	(void)visible;
	return false;
#endif
}

bool PlatformHelper::setWindowTitleBarColor( void* window, Uint8 red, Uint8 green, Uint8 blue ) {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	return Private::setWindowTitleBarColor( window, red, green, blue );
#else
	(void)window;
	(void)red;
	(void)green;
	(void)blue;
	return false;
#endif
}

}} // namespace EE::Window
