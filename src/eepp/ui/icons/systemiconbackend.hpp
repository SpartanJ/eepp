#ifndef EE_UI_ICONS_SYSTEMICONBACKEND_HPP
#define EE_UI_ICONS_SYSTEMICONBACKEND_HPP

#include <eepp/graphics/image.hpp>
#include <eepp/ui/uisystemicontheme.hpp>

#include <memory>
#include <string>

namespace EE { namespace UI { namespace Private {

/** @brief Platform icon source of UISystemIconTheme.
 *
 * Thread-safe: loads run concurrently on the theme's worker, and may block.
 */
class SystemIconBackend {
  public:
	using Request = UISystemIconTheme::Request;

	/** @return The backend of this platform (freedesktop icon themes, the Windows shell or
	 * NSWorkspace), or null where the platform offers no icons.
	 * @param themeName Freedesktop theme to use; empty detects the user's. */
	static std::unique_ptr<SystemIconBackend> create( const std::string& themeName );

	/** Fits an image into a size x size box (keeping its aspect ratio). */
	static void fitImage( Graphics::Image& image, int size );

	virtual ~SystemIconBackend() = default;

	/** @return The theme in use (freedesktop), empty when the platform has no themes. Blocking
	 * the first time (detection and indexing). */
	virtual std::string themeName() { return {}; }

	/** @return The image for request rasterized at size pixels, null when there is none. */
	virtual std::unique_ptr<Graphics::Image> load( const Request& request, int size ) = 0;

	/** @return True when the platform's shell knows file types by extension and gives special
	 * folders their own icons (Request::Kind::File and Request::Kind::Path); otherwise only
	 * names are used. */
	virtual bool hasShellIcons() const { return false; }
};

}}} // namespace EE::UI::Private

#endif
