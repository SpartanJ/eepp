#include <eepp/system/sys.hpp>
#include <eepp/window/backend/SDL2/base.hpp>
#include <eepp/window/backend/SDL2/displaymanagersdl2.hpp>
#include <eepp/window/platform/physicaldisplaymetrics.hpp>

#if defined( EE_X11_PLATFORM ) && defined( SDL_VIDEO_DRIVER_X11 ) && __has_include( <X11/Xresource.h> )
#define EE_SDL2_X11_DESKTOP_SCALE
#include <X11/Xatom.h>
#include <eepp/window/backend/SDL2/syswm.hpp>
#define Display SDLNativeDisplay
#define Status int
#include <X11/Xresource.h>
#undef Status
#undef Display
#include <cmath>
#include <cstdlib>
#include <memory>
#endif

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

#if EE_PLATFORM == EE_PLATFORM_WIN
static bool DISPLAY_REQUEST_DPI_AWARENESS = true;

void DisplayManagerSDL2::setDPIAwareness() {
	if ( DISPLAY_REQUEST_DPI_AWARENESS ) {
#if defined( SDL_HINT_WINDOWS_DPI_AWARENESS )
		// Set before video initialization. The old system-aware API prevents per-monitor
		// scaling once it succeeds, so let modern SDL select per-monitor-v2 awareness instead.
		SDL_SetHint( SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2" );
#if defined( SDL_HINT_WINDOWS_DPI_SCALING )
		SDL_SetHint( SDL_HINT_WINDOWS_DPI_SCALING, "1" );
#endif
		DISPLAY_REQUEST_DPI_AWARENESS = false;
#else
		void* user32 = Sys::loadObject( "user32.dll" );
		if ( user32 ) {
			int ( *SetProcessDPIAware )() =
				(int ( * )())Sys::loadFunction( user32, "SetProcessDPIAware" );
			if ( SetProcessDPIAware ) {
				SetProcessDPIAware();
				DISPLAY_REQUEST_DPI_AWARENESS = false;
			}
		}
#endif
	}
}
#endif

#if defined( EE_X11_PLATFORM )
Float DisplayManagerSDL2::getX11DesktopScale( eeWindowHandle display ) {
#ifdef EE_SDL2_X11_DESKTOP_SCALE
	if ( !display )
		return 0.f;
	// Read RESOURCE_MANAGER directly. XResourceManagerString caches the startup settings.
	using Library = std::unique_ptr<void, decltype( &Sys::unloadObject )>;
	static const Library x11(
		[] {
			auto* library = Sys::loadObject( "libX11.so.6", false );
			return library ? library : Sys::loadObject( "libX11.so", false );
		}(),
		Sys::unloadObject );
	if ( !x11 )
		return 0.f;
#define EE_X11_LOAD( fn )  \
	static const auto fn = \
		reinterpret_cast<decltype( &::fn )>( Sys::loadFunction( x11.get(), #fn, false ) );
	EE_X11_LOAD( XDefaultRootWindow )
	EE_X11_LOAD( XInternAtom )
	EE_X11_LOAD( XGetWindowProperty )
	EE_X11_LOAD( XFree )
	EE_X11_LOAD( XrmInitialize )
	EE_X11_LOAD( XrmGetStringDatabase )
	EE_X11_LOAD( XrmGetResource )
	EE_X11_LOAD( XrmDestroyDatabase )
#undef EE_X11_LOAD
	if ( !XDefaultRootWindow || !XInternAtom || !XGetWindowProperty || !XFree || !XrmInitialize ||
		 !XrmGetStringDatabase || !XrmGetResource || !XrmDestroyDatabase )
		return 0.f;
	const Atom resourceManager = XInternAtom( display, "RESOURCE_MANAGER", True );
	if ( !resourceManager )
		return 0.f;
	Atom type = 0;
	int format = 0;
	unsigned long count = 0, remaining = 0;
	unsigned char* data = nullptr;
	const int status =
		XGetWindowProperty( display, XDefaultRootWindow( display ), resourceManager, 0, 16384,
							False, XA_STRING, &type, &format, &count, &remaining, &data );
	std::unique_ptr<unsigned char, decltype( XFree )> resources( data, XFree );
	if ( status != Success || type != XA_STRING || format != 8 || !resources || remaining )
		return 0.f;
	XrmInitialize();
	std::unique_ptr<_XrmHashBucketRec, decltype( XrmDestroyDatabase )> database(
		XrmGetStringDatabase( reinterpret_cast<const char*>( resources.get() ) ),
		XrmDestroyDatabase );
	if ( !database )
		return 0.f;
	XrmValue value{};
	char* valueType = nullptr;
	if ( !XrmGetResource( database.get(), "Xft.dpi", "Xft.Dpi", &valueType, &value ) ||
		 !value.addr )
		return 0.f;
	char* end = nullptr;
	const Float dpi = std::strtof( value.addr, &end );
	return end != value.addr && *end == '\0' && std::isfinite( dpi ) && dpi > 0.f ? dpi / 96.f
																				  : 0.f;
#else
	(void)display;
	return 0.f;
#endif
}
#endif

DisplaySDL2::DisplaySDL2( int index ) : Display( index ) {}

std::string DisplaySDL2::getName() const {
	const char* name = SDL_GetDisplayName( index );
	return name ? name : "";
}

Rect DisplaySDL2::getBounds() const {
	SDL_Rect r;
	if ( SDL_GetDisplayBounds( index, &r ) == 0 )
		return Rect( Vector2i( r.x, r.y ), Sizei( r.w, r.h ) );
	return Rect();
}

Float DisplaySDL2::getDPI() {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return 96.f * emscripten_get_device_pixel_ratio();
#else
#if EE_PLATFORM == EE_PLATFORM_WIN
	DisplayManagerSDL2::setDPIAwareness();
#endif
	float ddpi, hdpi, vdpi;
	if ( 0 == SDL_GetDisplayDPI( index, &ddpi, &hdpi, &vdpi ) )
		return ddpi;
	return 96.f;
#endif
}

const int& DisplaySDL2::getIndex() const {
	return index;
}

Float DisplaySDL2::getPhysicalDPI() {
	const char* driver = SDL_GetCurrentVideoDriver();
	if ( !driver || ( std::strcmp( driver, "x11" ) != 0 && std::strcmp( driver, "windows" ) != 0 &&
					  std::strcmp( driver, "cocoa" ) != 0 ) )
		return 0.f;
	const auto mode = getCurrentMode();
	const Sizei resolution( mode.Width, mode.Height );
	if ( mPhysicalDPI < 0.f || resolution != mPhysicalDPIResolution ) {
		mPhysicalDPI = Private::PhysicalDisplayMetrics::getDPI( getBounds() );
		mPhysicalDPIResolution = resolution;
	}
	return mPhysicalDPI;
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

Uint32 DisplaySDL2::getRefreshRate() const {
	return getCurrentMode().RefreshRate;
}

Sizeu DisplaySDL2::getSize() const {
	DisplayMode mode( getCurrentMode() );
	return { mode.Width, mode.Height };
}

DisplayMode DisplaySDL2::getCurrentMode() const {
	SDL_DisplayMode mode;

	if ( SDL_GetCurrentDisplayMode( index, &mode ) == 0 ) {
		return DisplayMode( mode.w, mode.h, mode.refresh_rate, index );
	}

	return DisplayMode( 0, 0, 0, 0 );
}

DisplayMode DisplaySDL2::getClosestDisplayMode( DisplayMode wantedMode ) const {
	SDL_DisplayMode target, mode;

	target.w = wantedMode.Width;
	target.h = wantedMode.Height;
	target.format = 0;
	target.refresh_rate = wantedMode.RefreshRate;
	target.driverdata = 0;

	if ( SDL_GetClosestDisplayMode( index, &target, &mode ) != nullptr ) {
		return DisplayMode( mode.w, mode.h, mode.refresh_rate, index );
	}

	return DisplayMode( 0, 0, 0, 0 );
}

Rect DisplaySDL2::getUsableBounds() const {
	SDL_Rect r;
	if ( SDL_GetDisplayUsableBounds( index, &r ) == 0 )
		return Rect( Vector2i( r.x, r.y ), Sizei( r.w, r.h ) );
	return Rect();
}

void DisplaySDL2::invalidateMetrics() {
	displayModes.clear();
	mPhysicalDPI = -1.f;
}

void DisplayManagerSDL2::invalidateDisplayMetrics() {
	for ( auto* display : displays )
		static_cast<DisplaySDL2*>( display )->invalidateMetrics();
}

int DisplayManagerSDL2::getDisplayCount() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	setDPIAwareness();
#endif
	if ( !SDL_WasInit( SDL_INIT_VIDEO ) )
		SDL_Init( SDL_INIT_VIDEO );
	const int count = SDL_GetNumVideoDisplays();
	if ( count != mDisplayCount ) {
		invalidateDisplayMetrics();
		mDisplayCount = count;
	}
	return count;
}

Display* DisplayManagerSDL2::getDisplayIndex( int index ) {
	if ( index < 0 || index >= getDisplayCount() )
		return nullptr;
	// SDL2 can reuse an index for a different monitor. Keep wrappers stable by slot and
	// discard their cached metrics when the topology changes.
	while ( displays.size() <= static_cast<size_t>( index ) )
		displays.push_back( eeNew( DisplaySDL2, ( static_cast<int>( displays.size() ) ) ) );
	return displays[index];
}

void DisplayManagerSDL2::enableScreenSaver() {
	SDL_EnableScreenSaver();
	SDL_SetHint( SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1" );
}

void DisplayManagerSDL2::disableScreenSaver() {
	SDL_DisableScreenSaver();
	SDL_SetHint( SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0" );
}

void DisplayManagerSDL2::enableMouseFocusClickThrough() {
	SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1" );
}

void DisplayManagerSDL2::disableMouseFocusClickThrough() {
	SDL_SetHint( SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "0" );
}

void DisplayManagerSDL2::disableBypassCompositor() {
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	SDL_SetHint( SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "0" );
#endif
}

void DisplayManagerSDL2::enableBypassCompositor() {
#ifdef SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR
	SDL_SetHint( SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "1" );
#endif
}

}}}} // namespace EE::Window::Backend::SDL2
