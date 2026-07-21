#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/backend/SDL3/displaymanagersdl3.hpp>

#ifdef EE_BACKEND_SDL3

#include <cstdlib>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

DisplaySDL3::DisplaySDL3( int index, SDL_DisplayID displayId ) :
	Display( index ), mDisplayId( displayId ) {}

std::string DisplaySDL3::getName() const {
	return std::string( mDisplayId ? SDL_GetDisplayName( mDisplayId ) : "Unknown" );
}

Rect DisplaySDL3::getBounds() const {
	SDL_Rect r{};
	if ( mDisplayId && SDL_GetDisplayBounds( mDisplayId, &r ) == 0 )
		return Rect( r.x, r.y, r.w, r.h );
	return Rect();
}

Rect DisplaySDL3::getUsableBounds() const {
	SDL_Rect r{};
	if ( mDisplayId && SDL_GetDisplayUsableBounds( mDisplayId, &r ) == 0 )
		return Rect( r.x, r.y, r.w, r.h );
	return Rect();
}

Float DisplaySDL3::getDPI() {
	float scale = 1.0f;
	if ( mDisplayId ) {
		scale = SDL_GetDisplayContentScale( mDisplayId );
		if ( scale <= 0 )
			scale = 1.0f;
	}
	return 96.0f * scale;
}

const int& DisplaySDL3::getIndex() const {
	return index;
}

DisplayMode DisplaySDL3::getCurrentMode() const {
	const SDL_DisplayMode* mode = mDisplayId ? SDL_GetCurrentDisplayMode( mDisplayId ) : nullptr;
	if ( mode )
		return DisplayMode( mode->w, mode->h, static_cast<int>( mode->refresh_rate ), index );
	return DisplayMode( 0, 0, 0, index );
}

DisplayMode DisplaySDL3::getClosestDisplayMode( DisplayMode wantedMode ) const {
	if ( !mDisplayId )
		return DisplayMode( 0, 0, 0, index );

	SDL_DisplayMode closest;
	if ( SDL_GetClosestFullscreenDisplayMode( mDisplayId, wantedMode.Width, wantedMode.Height,
											  static_cast<float>( wantedMode.RefreshRate ), true,
											  &closest ) ) {
		return DisplayMode( closest.w, closest.h, static_cast<int>( closest.refresh_rate ), index );
	}
	return DisplayMode( 0, 0, 0, index );
}

const std::vector<DisplayMode>& DisplaySDL3::getModes() const {
	if ( displayModes.empty() && mDisplayId ) {
		int count = 0;
		SDL_DisplayMode** modesArray = SDL_GetFullscreenDisplayModes( mDisplayId, &count );
		if ( modesArray ) {
			for ( int i = 0; i < count; i++ ) {
				SDL_DisplayMode* mode = modesArray[i];
				displayModes.push_back( DisplayMode(
					mode->w, mode->h, static_cast<int>( mode->refresh_rate ), index ) );
			}
			SDL_free( modesArray );
		}
	}
	return displayModes;
}

Uint32 DisplaySDL3::getRefreshRate() const {
	return getCurrentMode().RefreshRate;
}

Sizeu DisplaySDL3::getSize() const {
	DisplayMode mode = getCurrentMode();
	return { static_cast<unsigned int>( mode.Width ), static_cast<unsigned int>( mode.Height ) };
}

int DisplayManagerSDL3::getDisplayCount() {
	if ( mDisplayIds.empty() ) {
		// Initialize SDL video if not already done, similar to SDL2 backend
		if ( !SDL_WasInit( SDL_INIT_VIDEO ) && !SDL_Init( SDL_INIT_VIDEO ) ) {
			Log::error( "DisplayManagerSDL3: Failed to initialize SDL video: %s", SDL_GetError() );
			return 0;
		}
		int count = 0;
		SDL_DisplayID* ids = SDL_GetDisplays( &count );
		if ( ids ) {
			mDisplayIds.assign( ids, ids + count );
			SDL_free( ids );
		}
	}
	return static_cast<int>( mDisplayIds.size() );
}

Display* DisplayManagerSDL3::getDisplayIndex( int index ) {
	if ( displays.empty() ) {
		int count = getDisplayCount();
		if ( count > 0 && !mDisplayIds.empty() ) {
			for ( int i = 0; i < count; i++ ) {
				displays.push_back( eeNew( DisplaySDL3, ( i, mDisplayIds[i] ) ) );
			}
		}
	}
	return ( index >= 0 && index < (int)displays.size() ) ? displays[index] : nullptr;
}

void DisplayManagerSDL3::enableScreenSaver() {
	SDL_EnableScreenSaver();
	SDL_SetHint( SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1" );
}

void DisplayManagerSDL3::disableScreenSaver() {
	SDL_DisableScreenSaver();
	SDL_SetHint( SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0" );
}

void DisplayManagerSDL3::enableMouseFocusClickThrough() {
	SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1" );
}

void DisplayManagerSDL3::disableMouseFocusClickThrough() {
	SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "0" );
}

void DisplayManagerSDL3::disableBypassCompositor() {
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	SDL_SetHint( SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0" );
#endif
}

void DisplayManagerSDL3::enableBypassCompositor() {
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	SDL_SetHint( SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "1" );
#endif
}

int DisplayManagerSDL3::getDisplayIndexFromID( SDL_DisplayID id ) const {
	for ( size_t i = 0; i < mDisplayIds.size(); ++i ) {
		if ( mDisplayIds[i] == id )
			return static_cast<int>( i );
	}
	return -1;
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
