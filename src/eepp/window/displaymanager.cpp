#include <eepp/window/displaymanager.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#endif

namespace EE { namespace Window {

Display::Display( int displayIndex ) : index( displayIndex ) {}

std::string Display::getPersistentId() const {
	return {};
}

Float Display::getPhysicalDPI() {
	return 0.f;
}

Sizeu Display::getNativePixelSize() const {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	const auto size = Private::getNativeDisplayPixelSize( getBounds() );
	if ( size.x && size.y )
		return size;
#endif
	return getSize();
}

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

void DisplayManager::disableBypassCompositor() {}

void DisplayManager::enableBypassCompositor() {}

}} // namespace EE::Window
