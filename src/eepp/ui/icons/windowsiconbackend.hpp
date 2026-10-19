#ifndef EE_UI_ICONS_WINDOWSICONBACKEND_HPP
#define EE_UI_ICONS_WINDOWSICONBACKEND_HPP

#include "systemiconbackend.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace UI { namespace Private {

/** Windows shell icons: file types, special folders and stock icons. Initializes COM itself on
 * the loading thread. */
class WindowsIconBackend : public SystemIconBackend {
  public:
	std::unique_ptr<Graphics::Image> load( const Request& request, int size ) override;

	bool hasShellIcons() const override { return true; }
};

}}} // namespace EE::UI::Private

#endif

#endif
