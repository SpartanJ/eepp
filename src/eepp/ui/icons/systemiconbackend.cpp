#include "systemiconbackend.hpp"

#include "freedesktopiconbackend.hpp"
#include "macosiconbackend.hpp"
#include "windowsiconbackend.hpp"

#include <algorithm>

using namespace EE::Graphics;

namespace EE { namespace UI { namespace Private {

std::unique_ptr<SystemIconBackend> SystemIconBackend::create( const std::string& themeName ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	(void)themeName;
	return std::make_unique<WindowsIconBackend>();
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	(void)themeName;
	return std::make_unique<MacOSIconBackend>();
#elif defined( EE_SYSTEM_ICONS_FREEDESKTOP )
	return std::make_unique<FreedesktopIconBackend>( themeName );
#else
	(void)themeName;
	return nullptr;
#endif
}

void SystemIconBackend::fitImage( Image& image, int size ) {
	unsigned int largest = std::max( image.getWidth(), image.getHeight() );
	if ( largest == 0 || largest == static_cast<unsigned int>( size ) )
		return;
	Float scale = size / static_cast<Float>( largest );
	image.resize( std::max( 1u, static_cast<unsigned int>( image.getWidth() * scale + 0.5f ) ),
				  std::max( 1u, static_cast<unsigned int>( image.getHeight() * scale + 0.5f ) ) );
}

}}} // namespace EE::UI::Private
