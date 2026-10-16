#include "freedesktopiconbackend.hpp"

#ifdef EE_SYSTEM_ICONS_FREEDESKTOP

#include <eepp/system/filesystem.hpp>

#include <algorithm>

using namespace EE::Graphics;
using namespace EE::System;

namespace EE { namespace UI { namespace Private {

FreedesktopIconBackend::FreedesktopIconBackend( std::string themeName ) :
	mRequestedTheme( std::move( themeName ) ) {}

void FreedesktopIconBackend::init() {
	std::call_once( mInit, [this] {
		std::string name =
			mRequestedTheme.empty() ? FreedesktopIconLookup::detectThemeName() : mRequestedTheme;
		mLookup = std::make_unique<FreedesktopIconLookup>( name );
	} );
}

std::string FreedesktopIconBackend::themeName() {
	init();
	return mLookup->getThemeName();
}

std::unique_ptr<Image> FreedesktopIconBackend::load( const Request& request, int size ) {
	init();
	if ( !mLookup->isValid() )
		return nullptr;
	std::string path = mLookup->find( request.names, size );
	return path.empty() ? nullptr : loadIconFile( path, size );
}

std::unique_ptr<Image> FreedesktopIconBackend::loadIconFile( const std::string& path, int size ) {
	std::unique_ptr<Image> image;
	if ( FileSystem::fileExtension( path ) == "svg" ) {
		std::string svg;
		if ( !FileSystem::fileGet( path, svg ) )
			return nullptr;
		Image::FormatConfiguration format;
		int width = 0, height = 0, channels = 0;
		if ( !Image::getInfoFromMemory( reinterpret_cast<const unsigned char*>( svg.data() ),
										svg.size(), &width, &height, &channels, format ) ||
			 width <= 0 || height <= 0 ) {
			return nullptr;
		}
		format.svgScale( size / static_cast<Float>( std::max( width, height ) ) );
		image = std::make_unique<Image>( reinterpret_cast<const Uint8*>( svg.data() ),
										 static_cast<unsigned int>( svg.size() ), 4, format );
	} else {
		image = std::make_unique<Image>( path, 4 );
	}
	if ( !image->getPixelsPtr() || image->getWidth() == 0 || image->getHeight() == 0 )
		return nullptr;
	fitImage( *image, size );
	return image;
}

}}} // namespace EE::UI::Private

#endif
