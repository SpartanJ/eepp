#include "dragpreview.hpp"
#include <algorithm>
#include <eepp/graphics/image.hpp>

namespace EE { namespace Window { namespace Private {

using Graphics::Image;

Image NativeDragPreview::makeFileImage( FileImagePixels& pixels, size_t count ) {
	Image image( pixels.data(), 112, 48, 4 );
	image.avoidFreeImage( true );
	image.fillWithColor( Color( 35, 43, 57 ) );
	const Color paper( 222, 234, 251 );
	const Color ink( 79, 118, 168 );
	const auto white = Color::White;
	auto rectangle = [&]( int x, int y, int width, int height, const Color& color ) {
		for ( int row = y; row < y + height; ++row ) {
			for ( int column = x; column < x + width; ++column )
				image.setPixel( column, row, color );
		}
	};
	rectangle( 8, 6, 28, 36, paper );
	for ( int y : { 17, 24, 31 } )
		rectangle( 14, y, 16, 1, ink );
	constexpr std::array<Uint16, 10> digits{ 0x7B6F, 0x2492, 0x73E7, 0x73CF, 0x5BC9,
											 0x79CF, 0x79EF, 0x7249, 0x7BEF, 0x7BCF };
	const unsigned value = static_cast<unsigned>( std::min<size_t>( count, 999 ) );
	const unsigned length = value >= 100 ? 3 : value >= 10 ? 2 : 1;
	unsigned divisor = length == 3 ? 100 : length == 2 ? 10 : 1;
	for ( unsigned digit = 0; digit < length; ++digit ) {
		const auto glyph = digits[value / divisor % 10];
		for ( unsigned bit = 0; bit < 15; ++bit ) {
			if ( !( glyph & ( 1 << ( 14 - bit ) ) ) )
				continue;
			rectangle( 48 + digit * 10 + bit % 3 * 2, 18 + bit / 3 * 2, 2, 2, white );
		}
		divisor /= 10;
	}
	if ( count > 999 ) {
		rectangle( 83, 22, 7, 1, white );
		rectangle( 86, 19, 1, 7, white );
	}
	return image;
}

Image NativeDragPreview::makeCopyCursorImage( CopyCursorPixels& pixels ) {
	Image image( pixels.data(), 24, 24, 4 );
	image.avoidFreeImage( true );
	image.fillWithColor( Color::Transparent );
	for ( unsigned y = 0; y < 17; ++y ) {
		for ( unsigned x = 0; x <= y / 2; ++x ) {
			image.setPixel( x, y, x == 0 || x == y / 2 || y == 16 ? Color::White : Color::Black );
		}
	}
	for ( unsigned y = 12; y < 24; ++y ) {
		for ( unsigned x = 12; x < 24; ++x ) {
			image.setPixel( x, y, Color( 31, 150, 82, 255 ) );
		}
	}
	for ( unsigned i = 15; i < 22; ++i ) {
		image.setPixel( 18, i, Color::White );
		image.setPixel( i, 18, Color::White );
	}
	return image;
}

}}} // namespace EE::Window::Private
