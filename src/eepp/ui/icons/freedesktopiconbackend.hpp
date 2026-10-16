#ifndef EE_UI_ICONS_FREEDESKTOPICONBACKEND_HPP
#define EE_UI_ICONS_FREEDESKTOPICONBACKEND_HPP

#include "freedesktopiconlookup.hpp"
#include "systemiconbackend.hpp"

#ifdef EE_SYSTEM_ICONS_FREEDESKTOP

#include <mutex>

namespace EE { namespace UI { namespace Private {

/** Icons of the user's freedesktop.org icon theme (Linux, BSD). */
class FreedesktopIconBackend : public SystemIconBackend {
  public:
	explicit FreedesktopIconBackend( std::string themeName );

	std::string themeName() override;

	std::unique_ptr<Graphics::Image> load( const Request& request, int size ) override;

  private:
	/** Detects the theme and indexes it. Blocking, once. */
	void init();

	/** Reads and rasterizes an icon file (SVG or PNG) at size pixels. */
	static std::unique_ptr<Graphics::Image> loadIconFile( const std::string& path, int size );

	std::string mRequestedTheme;
	std::once_flag mInit;
	std::unique_ptr<FreedesktopIconLookup> mLookup;
};

}}} // namespace EE::UI::Private

#endif

#endif
