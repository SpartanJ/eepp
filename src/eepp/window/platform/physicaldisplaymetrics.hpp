#ifndef EE_WINDOW_PHYSICALDISPLAYMETRICS_HPP
#define EE_WINDOW_PHYSICALDISPLAYMETRICS_HPP

#include <eepp/math/rect.hpp>

namespace EE { namespace Window { namespace Private {

/** Optional native monitor measurements. Queries run on the UI thread and return zero when
 * physical dimensions are unavailable. Backends cache results until the display mode changes. */
class PhysicalDisplayMetrics {
  public:
	static Float getDPI( const Math::Rect& bounds );
};

}}} // namespace EE::Window::Private

#endif
