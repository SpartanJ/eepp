#ifndef EE_UI_ICONS_SYSTEMICONS_HPP
#define EE_UI_ICONS_SYSTEMICONS_HPP

#include "freedesktopiconlookup.hpp"
#include <eepp/graphics/image.hpp>
#include <eepp/ui/uisystemicontheme.hpp>

#include <memory>

// Private: the platform backends of UISystemIconTheme.

namespace EE { namespace UI { namespace Private {

/** Fits an image into a size x size box (keeping its aspect ratio). */
void fitImage( Graphics::Image& image, int size );

#if EE_PLATFORM == EE_PLATFORM_WIN
/** Shell icon for a request, rasterized at size pixels. Any thread; initializes COM itself. */
std::unique_ptr<Graphics::Image> loadWindowsIcon( const UISystemIconTheme::Request& request,
												  int size );
#endif

}}} // namespace EE::UI::Private

#if EE_PLATFORM == EE_PLATFORM_MACOS
/** NSWorkspace icon rasterized at size x size: kind 0 = names, 1 = file type (value is the file
 * name), 2 = path. Returns malloc'ed RGBA with straight alpha (free() it), or null. Any thread. */
extern "C" unsigned char* eeppSystemIconRGBA( int kind, const char* value, int isDirectory,
											  const char* const* names, int nameCount, int size );
#endif

#endif
