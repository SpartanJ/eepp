#ifndef EE_WINDOW_PLATFORM_MACOS_PLATFORMHELPER_HPP
#define EE_WINDOW_PLATFORM_MACOS_PLATFORMHELPER_HPP

#include <eepp/config.hpp>
#include <eepp/math/rect.hpp>
#include <functional>

namespace EE { namespace Window { namespace Private {

class ApplicationReopenHandler {
  public:
	static bool setCallback( std::function<bool()> callback );
};

class WindowVisibility {
  public:
	static bool deferHide( void* window, Uint32 windowId, void ( *complete )( Uint32 ),
						   bool requestNativeExit = true );

	static void cancelHide( void* window );
};

bool setNativeScrollMomentumEnabled( bool enabled );

Float getPhysicalDisplayDPI( const Math::Rect& bounds );

bool setWindowTitleBarSeparatorVisible( void* window, bool visible );

bool setWindowTitleBarColor( void* window, Uint8 red, Uint8 green, Uint8 blue );

}}} // namespace EE::Window::Private

#endif // EE_WINDOW_PLATFORM_MACOS_PLATFORMHELPER_HPP
