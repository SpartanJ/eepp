#include <eepp/window/backend/SDL2/base.hpp>
#include <eepp/window/backend/SDL2/displaymanagersdl2.hpp>

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

DisplaySDL2::DisplaySDL2( int index ) : Display( index ) {}

std::string DisplaySDL2::getName() {
	return std::string( SDL_GetDisplayName( index ) );
}

Rect DisplaySDL2::getBounds() {
	SDL_Rect r;
	if ( SDL_GetDisplayBounds( index, &r ) == 0 )
		return Rect( r.x, r.y, r.w, r.h );
	return Rect();
}

Float DisplaySDL2::getDPI() {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return 96.f * emscripten_get_device_pixel_ratio();
#else
#if SDL_VERSION_ATLEAST( 2, 0, 4 )
	float ddpi, hdpi, vdpi;
	if ( 0 == SDL_GetDisplayDPI( 0, &ddpi, &hdpi, &vdpi ) )
		return ddpi;
#endif
	return 96.f;
#endif
}

const int& DisplaySDL2::getIndex() const {
	return index;
}

const std::vector<DisplayMode>& DisplaySDL2::getModes() const {
	if ( displayModes.empty() ) {
		int count = SDL_GetNumDisplayModes( index );

		if ( count > 0 ) {
			for ( int mode_index = 0; mode_index < count; mode_index++ ) {
				SDL_DisplayMode mode;

				if ( SDL_GetDisplayMode( index, mode_index, &mode ) == 0 ) {
					displayModes.push_back(
						DisplayMode( mode.w, mode.h, mode.refresh_rate, index ) );
				}
			}
		}
	}

	return displayModes;
}

DisplayMode DisplaySDL2::getCurrentMode() {
	SDL_DisplayMode mode;

	if ( SDL_GetCurrentDisplayMode( index, &mode ) == 0 ) {
		return DisplayMode( mode.w, mode.h, mode.refresh_rate, index );
	}

	return DisplayMode( 0, 0, 0, 0 );
}

DisplayMode DisplaySDL2::getClosestDisplayMode( DisplayMode wantedMode ) {
	SDL_DisplayMode target, mode;

	target.w = wantedMode.Width;
	target.h = wantedMode.Height;
	target.format = 0;
	target.refresh_rate = wantedMode.RefreshRate;
	target.driverdata = 0;

	if ( SDL_GetClosestDisplayMode( 0, &target, &mode ) != NULL ) {
		return DisplayMode( mode.w, mode.h, mode.refresh_rate, index );
	}

	return DisplayMode( 0, 0, 0, 0 );
}

Rect DisplaySDL2::getUsableBounds() {
#if SDL_VERSION_ATLEAST( 2, 0, 5 )
	SDL_Rect r;
	if ( SDL_GetDisplayUsableBounds( index, &r ) == 0 )
		return Rect( r.x, r.y, r.w, r.h );
#endif
	return Rect();
}

int DisplayManagerSDL2::getDisplayCount() {
	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
		SDL_Init( SDL_INIT_VIDEO );
	return SDL_GetNumVideoDisplays();
}

Display* DisplayManagerSDL2::getDisplayIndex( int index ) {
	if ( displays.empty() ) {
		int count = getDisplayCount();

		if ( count > 0 ) {
			for ( int i = 0; i < count; i++ ) {
				displays.push_back( eeNew( DisplaySDL2, ( i ) ) );
			}
		}
	}

	return index >= 0 && index < (Int32)displays.size() ? displays[index] : NULL;
}

void DisplayManagerSDL2::enableScreenSaver() {
	SDL_EnableScreenSaver();
}

void DisplayManagerSDL2::disableScreenSaver() {
	SDL_DisableScreenSaver();
}

}}}} // namespace EE::Window::Backend::SDL2
