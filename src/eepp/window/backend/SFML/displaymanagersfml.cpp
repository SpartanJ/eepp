#include <eepp/window/backend/SFML/displaymanagersfml.hpp>
#include <eepp/math/vector2.hpp>
#include <SFML/Window/VideoMode.hpp>
using namespace sf;

namespace EE { namespace Window { namespace Backend { namespace SFML {

DisplaySFML::DisplaySFML( int index ) :
	Display( index )
{}

std::string DisplaySFML::getName() {
	return std::string( "Display " + String::toStr( index ) );
}

Rect DisplaySFML::getBounds() {
	VideoMode mode = VideoMode::getDesktopMode();
	return Rect( 0, 0, mode.width, mode.height );
}

Float DisplaySFML::getDPI() {
	return 92.f;
}

const int& DisplaySFML::getIndex() const {
	return index;
}

const std::vector<DisplayMode>& DisplaySFML::getModes() const {
	if ( displayModes.empty() ) {
		auto modes = VideoMode::getFullscreenModes();

		for ( size_t i = 0; i < modes.size(); i++ ) {
			displayModes.push_back( DisplayMode( modes[i].width, modes[i].height, 60, index ) );
		}
	}
	
	return displayModes;
}

DisplayMode DisplaySFML::getCurrentMode() {
	VideoMode mode = VideoMode::getDesktopMode();
	return DisplayMode( mode.width, mode.height, 60, index );
}

DisplayMode DisplaySFML::getClosestDisplayMode( DisplayMode wantedMode ) {
	getModes();

	if ( !displayModes.empty() ) {
		Vector2f wanted( wantedMode.Width, wantedMode.Height );
		size_t closestIndex = 0;
		Float closestDistance = 99999999999.f;

		for ( size_t i = 0; i < displayModes.size(); i++ ) {
			Vector2f cur( displayModes[i].Width, displayModes[i].Height );
			Float dist;

			if ( ( dist = wanted.distance( cur ) ) < closestDistance ) {
				closestDistance = dist;
				closestIndex = i;
			}
		}

		return displayModes[ closestIndex ];
	}

	return DisplayMode(0,0,0,0);
}

Rect DisplaySFML::getUsableBounds() {
	return getBounds();
}

int DisplayManagerSFML::getDisplayCount() {
	return 1;
}

Display * DisplayManagerSFML::getDisplayIndex( int index ) {
	if ( displays.empty() ) {
		int count = getDisplayCount();

		if ( count > 0 ) {
			for ( int i = 0; i < count; i++ ) {
				displays.push_back( eeNew( DisplaySFML, ( i ) ) );
			}
		}
	}

	return index >= 0 && index < (Int32)displays.size() ? displays[ index ] : NULL;
}

}}}}
