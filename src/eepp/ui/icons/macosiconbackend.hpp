#ifndef EE_UI_ICONS_MACOSICONBACKEND_HPP
#define EE_UI_ICONS_MACOSICONBACKEND_HPP

#include "systemiconbackend.hpp"

#if EE_PLATFORM == EE_PLATFORM_MACOS

namespace EE { namespace UI { namespace Private {

/** NSWorkspace icons (macOS). The Cocoa side is macosiconimage.mm. */
class MacOSIconBackend : public SystemIconBackend {
  public:
	std::unique_ptr<Graphics::Image> load( const Request& request, int size ) override;

	bool hasShellIcons() const override { return true; }
};

}}} // namespace EE::UI::Private

#endif

#endif
