#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window {

Display::Display( int displayIndex ) : index( displayIndex ) {}

PixelDensitySize Display::getPixelDensitySize() {
	return PixelDensity::fromDPI( getDPI() );
}

Float Display::getPixelDensity() {
	return PixelDensity::toFloat( PixelDensity::fromDPI( getDPI() ) );
}

Display::~Display() {}

DisplayManager::~DisplayManager() {
	if ( !displays.empty() ) {
		for ( size_t i = 0; i < displays.size(); i++ ) {
			eeDelete( displays[i] );
		}
	}
}

void DisplayManager::enableScreenSaver() {}

void DisplayManager::disableScreenSaver() {}

void DisplayManager::enableMouseFocusClickThrough() {}

void DisplayManager::disableMouseFocusClickThrough() {}

}} // namespace EE::Window
