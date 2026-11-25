#ifndef EE_WINDOW_PHYSICALDISPLAYMETRICS_HPP
#define EE_WINDOW_PHYSICALDISPLAYMETRICS_HPP

#include <eepp/math/rect.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#include <eepp/window/windowhandle.hpp>
#endif
#include <string>

namespace EE::Window {

class Display;
class DisplayManager;

} // namespace EE::Window

namespace EE { namespace Window { namespace Private {

/** Optional native monitor measurements. Queries run on the UI thread and return zero when
 * physical dimensions are unavailable. Backends cache results until the display mode changes. */
class PhysicalDisplayMetrics {
  public:
	static Float getDPI( const Math::Rect& bounds );

#if EE_PLATFORM == EE_PLATFORM_WIN
	static Float getScale( const Math::Rect& bounds );

	static std::string getPersistentId( eeWindowHandle window );
	static std::string getPersistentMonitorId( void* monitor );
#endif

	static std::string getPersistentId( const Math::Rect& bounds );

	/** Returns a best-effort key for a unique Wayland display label. */
	static std::string getWaylandPersistentId( const Display* display, DisplayManager* manager );
};

}}} // namespace EE::Window::Private

#endif
