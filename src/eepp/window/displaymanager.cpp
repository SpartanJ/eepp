#include <eepp/window/displaymanager.hpp>

namespace EE { namespace Window {

Display::Display( int displayIndex ) : index ( displayIndex ) {}

EE_PIXEL_DENSITY Display::getPixelDensity() {
	return PixelDensity::fromDPI( getDPI() );
}

Display::~Display() {}

DisplayManager::~DisplayManager() {
	if ( !displays.empty() ) {
		for ( size_t i = 0; i < displays.size(); i++ ) {
			eeDelete( displays[i] );
		}
	}
}

}}
