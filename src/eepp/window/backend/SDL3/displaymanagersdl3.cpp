#include <eepp/window/backend/SDL3/backendsdl3.hpp>
#include <eepp/window/backend/SDL3/base.hpp>
#include <eepp/window/backend/SDL3/displaymanagersdl3.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/platform/physicaldisplaymetrics.hpp>

#ifdef EE_BACKEND_SDL3

#include <cstdlib>

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

using namespace EE::Window::Private;

DisplaySDL3::DisplaySDL3( int index, SDL_DisplayID displayId ) :
	Display( index ), mDisplayId( displayId ) {}

std::string DisplaySDL3::getName() const {
	const char* name = mDisplayId ? SDL_GetDisplayName( mDisplayId ) : nullptr;
	return name ? name : "";
}

Rect DisplaySDL3::getBounds() const {
	SDL_Rect r{};
	if ( mDisplayId && SDL_GetDisplayBounds( mDisplayId, &r ) )
		return Rect( Vector2i( r.x, r.y ), Sizei( r.w, r.h ) );
	return Rect();
}

Rect DisplaySDL3::getUsableBounds() const {
	SDL_Rect r{};
	if ( mDisplayId && SDL_GetDisplayUsableBounds( mDisplayId, &r ) )
		return Rect( Vector2i( r.x, r.y ), Sizei( r.w, r.h ) );
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

std::string DisplaySDL3::getPersistentId() const {
	const char* driver = SDL_GetCurrentVideoDriver();
	if ( !driver )
		return {};
	if ( std::strcmp( driver, "wayland" ) == 0 ) {
		return PhysicalDisplayMetrics::getWaylandPersistentId(
			this, Engine::instance()->getDisplayManager() );
	}
	if ( std::strcmp( driver, "x11" ) != 0 && std::strcmp( driver, "cocoa" ) != 0 &&
		 std::strcmp( driver, "windows" ) != 0 )
		return {};
#if EE_PLATFORM == EE_PLATFORM_WIN && defined( SDL_PROP_DISPLAY_WINDOWS_HMONITOR_POINTER )
	if ( auto* monitor =
			 SDL_GetPointerProperty( SDL_GetDisplayProperties( mDisplayId ),
									 SDL_PROP_DISPLAY_WINDOWS_HMONITOR_POINTER, nullptr ) ) {
		return PhysicalDisplayMetrics::getPersistentMonitorId( monitor );
	}
#endif
	return PhysicalDisplayMetrics::getPersistentId( getBounds() );
}

Float DisplaySDL3::getPhysicalDPI() {
	const char* driver = SDL_GetCurrentVideoDriver();
	if ( !driver || ( std::strcmp( driver, "x11" ) != 0 && std::strcmp( driver, "windows" ) != 0 &&
					  std::strcmp( driver, "cocoa" ) != 0 ) )
		return 0.f;
	const auto mode = getCurrentMode();
	const Sizei resolution( mode.Width, mode.Height );
	if ( mPhysicalDPI < 0.f || resolution != mPhysicalDPIResolution ) {
		mPhysicalDPI = PhysicalDisplayMetrics::getDPI( getBounds() );
		mPhysicalDPIResolution = resolution;
	}
	return mPhysicalDPI;
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

void DisplaySDL3::refresh( int displayIndex ) {
	index = displayIndex;
	displayModes.clear();
	mPhysicalDPI = -1.f;
}

void DisplayManagerSDL3::invalidateDisplayList() {
	mDisplayListDirty = true;
}

void DisplayManagerSDL3::updateDisplayList() {
	if ( !mDisplayListDirty )
		return;
	if ( !WindowBackendSDL3::initVideo() ) {
		Log::error( "DisplayManagerSDL3: Failed to initialize SDL video: %s", SDL_GetError() );
		return;
	}
	int count = 0;
	SDL_DisplayID* ids = SDL_GetDisplays( &count );
	if ( !ids )
		return;
	// Keep Display pointers stable for clients that retained one. The active entries are
	// ordered as SDL reports them; disconnected entries remain owned at the end.
	for ( int i = 0; i < count; ++i ) {
		const auto id = ids[i];
		auto it = std::find_if( displays.begin() + i, displays.end(), [id]( Display* display ) {
			return static_cast<DisplaySDL3*>( display )->mDisplayId == id;
		} );
		if ( it == displays.end() ) {
			displays.push_back( eeNew( DisplaySDL3, ( i, id ) ) );
			it = displays.end() - 1;
		}
		std::iter_swap( displays.begin() + i, it );
		static_cast<DisplaySDL3*>( displays[i] )->refresh( i );
	}
	for ( size_t i = count; i < displays.size(); ++i )
		static_cast<DisplaySDL3*>( displays[i] )->refresh( -1 );
	mDisplayCount = count;
	mDisplayListDirty = false;
	SDL_free( ids );
}

int DisplayManagerSDL3::getDisplayCount() {
	updateDisplayList();
	return mDisplayCount;
}

Display* DisplayManagerSDL3::getDisplayIndex( int index ) {
	updateDisplayList();
	return index >= 0 && index < mDisplayCount ? displays[index] : nullptr;
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

int DisplayManagerSDL3::getDisplayIndexFromID( SDL_DisplayID id ) {
	updateDisplayList();
	for ( int i = 0; i < mDisplayCount; ++i ) {
		if ( static_cast<DisplaySDL3*>( displays[i] )->mDisplayId == id )
			return i;
	}
	return -1;
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
