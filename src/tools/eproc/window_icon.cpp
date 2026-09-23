#include "window_icon.hpp"

#include <algorithm>
#include <limits>

namespace eproc {

WindowIconPixels decodeWindowIcon( const unsigned long* property, size_t count,
								   Uint32 desiredSize ) {
	WindowIconPixels icon;
	if ( !property || desiredSize == 0 )
		return icon;

	// The property is a sequence of width, height, then width*height ARGB pixels. Prefer the
	// smallest image at least as large as the target to avoid enlarging a tiny icon.
	size_t bestOffset = 0;
	size_t bestPixels = 0;
	Uint32 bestWidth = 0;
	Uint32 bestHeight = 0;
	Uint64 bestScore = std::numeric_limits<Uint64>::max();
	for ( size_t offset = 0; count - offset >= 2; ) {
		const unsigned long width = property[offset++];
		const unsigned long height = property[offset++];
		if ( width == 0 || height == 0 || width > ( count - offset ) / height )
			break;
		const size_t pixels = static_cast<size_t>( width * height );
		const Uint64 dimension = std::max( width, height );
		const Uint64 score = dimension >= desiredSize
									 ? dimension - desiredSize
									 : ( Uint64{ 1 } << 32 ) + desiredSize - dimension;
		if ( width <= 512 && height <= 512 && score < bestScore ) {
			bestScore = score;
			bestOffset = offset;
			bestPixels = pixels;
			bestWidth = static_cast<Uint32>( width );
			bestHeight = static_cast<Uint32>( height );
		}
		offset += pixels;
	}
	if ( bestPixels == 0 )
		return icon;

	icon.width = bestWidth;
	icon.height = bestHeight;
	icon.rgba.resize( bestPixels * 4 );
	for ( size_t i = 0; i < bestPixels; ++i ) {
		const Uint32 argb = static_cast<Uint32>( property[bestOffset + i] );
		icon.rgba[i * 4] = static_cast<Uint8>( argb >> 16 );
		icon.rgba[i * 4 + 1] = static_cast<Uint8>( argb >> 8 );
		icon.rgba[i * 4 + 2] = static_cast<Uint8>( argb );
		icon.rgba[i * 4 + 3] = static_cast<Uint8>( argb >> 24 );
	}
	return icon;
}

} // namespace eproc
