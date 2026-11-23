#ifndef EE_WINDOW_X11SCALESETTINGS_HPP
#define EE_WINDOW_X11SCALESETTINGS_HPP

#include <eepp/config.hpp>

namespace EE { namespace Window { namespace Private {

struct X11ScaleSettings {
	Float windowScale{ 0.f };
	Float logicalScale{ 0.f };
};

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
X11ScaleSettings getX11ScaleSettings();
#else
inline X11ScaleSettings getX11ScaleSettings() {
	return {};
}
#endif

}}} // namespace EE::Window::Private

#endif
