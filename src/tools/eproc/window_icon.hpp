#ifndef EPROC_WINDOW_ICON_HPP
#define EPROC_WINDOW_ICON_HPP

#include <cstddef>
#include <eepp/core/core.hpp>
#include <vector>

using namespace EE;

namespace eproc {

/** Decoded RGBA pixels from the best image in an EWMH _NET_WM_ICON property. */
struct WindowIconPixels {
	Uint32 width{ 0 };
	Uint32 height{ 0 };
	std::vector<Uint8> rgba;

	bool valid() const { return width > 0 && height > 0 && !rgba.empty(); }
};

/** Xlib returns format-32 properties as unsigned long elements, even on 64-bit hosts. */
WindowIconPixels decodeWindowIcon( const unsigned long* property, size_t count,
								   Uint32 desiredSize );

} // namespace eproc

#endif // EPROC_WINDOW_ICON_HPP
