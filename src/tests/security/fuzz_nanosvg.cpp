#include <cstdint>
#include <cstring>

#define NANOSVG_IMPLEMENTATION
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvg.h>
#include <nanosvg/nanosvgrast.h>

extern "C" int LLVMFuzzerTestOneInput( const uint8_t* data, size_t size ) {
	if ( size > 8192 )
		return 0;
	char input[8193];
	std::memcpy( input, data, size );
	input[size] = '\0';
	NSVGimage* image = nsvgParse( input, "px", 96, 0xFFFFFFFF );
	if ( !image )
		return 0;
	NSVGrasterizer* rasterizer = nsvgCreateRasterizer();
	if ( rasterizer ) {
		unsigned char pixels[32 * 32 * 4];
		nsvgRasterize( rasterizer, image, 0, 0, 1, pixels, 32, 32, 32 * 4 );
		nsvgDeleteRasterizer( rasterizer );
	}
	nsvgDelete( image );
	return 0;
}
