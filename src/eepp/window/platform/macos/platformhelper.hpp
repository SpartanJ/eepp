#ifndef EE_WINDOW_PLATFORM_MACOS_PLATFORMHELPER_HPP
#define EE_WINDOW_PLATFORM_MACOS_PLATFORMHELPER_HPP

#include <eepp/config.hpp>

namespace EE { namespace Window { namespace Private {

bool setNativeScrollMomentumEnabled( bool enabled );

bool setWindowTitleBarSeparatorVisible( void* window, bool visible );

bool setWindowTitleBarColor( void* window, Uint8 red, Uint8 green, Uint8 blue );

}}} // namespace EE::Window::Private

#endif // EE_WINDOW_PLATFORM_MACOS_PLATFORMHELPER_HPP
