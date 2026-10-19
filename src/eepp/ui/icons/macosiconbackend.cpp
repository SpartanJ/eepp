#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS

#include "macosiconbackend.hpp"

#include <cstdlib>
#include <vector>

/** NSWorkspace icon rasterized at size x size (macosiconimage.mm): kind 0 = names, 1 = file type
 * (value is the file name), 2 = path. Returns malloc'ed RGBA with straight alpha (free() it), or
 * null. Any thread. */
extern "C" unsigned char* eeppSystemIconRGBA( int kind, const char* value, int isDirectory,
											  const char* const* names, int nameCount, int size );

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Private {

std::unique_ptr<Image> MacOSIconBackend::load( const Request& request, int size ) {
	std::vector<const char*> names;
	names.reserve( request.names.size() );
	for ( const auto& name : request.names )
		names.push_back( name.c_str() );
	unsigned char* pixels = eeppSystemIconRGBA(
		static_cast<int>( request.kind ), request.value.c_str(), request.isDirectory ? 1 : 0,
		names.data(), static_cast<int>( names.size() ), size );
	if ( !pixels )
		return nullptr;
	auto image = std::make_unique<Image>( static_cast<const Uint8*>( pixels ), size, size, 4 );
	std::free( pixels );
	return image;
}

}}} // namespace EE::UI::Private

#endif
