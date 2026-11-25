#include <eepp/window/displaymanager.hpp>

#include <eepp/core/string.hpp>
#include <eepp/system/md5.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <eepp/window/platform/macos/platformhelper.hpp>
#endif

namespace EE { namespace Window {

Display::Display( int displayIndex ) : index( displayIndex ) {}

std::string Display::getPersistentId() const {
	return {};
}

std::string Display::getLabelPersistentId( DisplayManager* manager ) const {
	if ( !manager || getIndex() < 0 )
		return {};

	constexpr std::string_view whitespace = " \t\r\n";
	const std::string label = getName();
	const std::string_view name = String::trim( std::string_view( label ), whitespace );
	// SDL generates numeric names from display indices or instance IDs when no label is given.
	if ( name.empty() || String::isNumber( name ) || String::iequals( name, "unknown" ) )
		return {};

	const int count = manager->getDisplayCount();
	for ( int index = 0; index < count; ++index ) {
		const Display* other = manager->getDisplayIndex( index );
		if ( !other || other == this )
			continue;
		const std::string otherLabel = other->getName();
		if ( String::trim( std::string_view( otherLabel ), whitespace ) == name )
			return {};
	}
	return "wayland-" + System::MD5::fromString( name ).toHexString();
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
