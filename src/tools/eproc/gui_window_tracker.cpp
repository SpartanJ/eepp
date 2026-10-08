#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )
#include <X11/Xatom.h>
#include <X11/Xlib.h>
// Xlib's None macro conflicts with eepp enum members and BlendMode::None().
#undef None
#endif

#include "gui_window_tracker.hpp"
#include "window_icon.hpp"

#include <cstdlib>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <limits>

// Which processes own a top-level window is read through a backend selected at runtime, so the
// class keeps the very same interface whether or not X11 is available on the platform.
//
//  - X11: the only backend implemented today. It relies on an X11 session, reading the EWMH
//    properties the running window manager publishes on the root window. On Wayland it therefore
//    only ever sees XWayland clients, since a native Wayland compositor does not expose its
//    toplevels over X11. A Wayland backend would have to speak a compositor protocol instead
//    (wlr-foreign-toplevel-management, or KDE's plasma-window-management) and is not implemented
//    yet.
//  - Anywhere else: the stub backend at the bottom of this file, which no X11 header or library is
//    needed for.
//
// The X11 backend is compiled where eepp's config found the X11 headers and loads libX11 at
// runtime, which keeps the eproc project independent of an installed X11 runtime.
#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )

#include <eepp/graphics/glyphdrawable.hpp>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/texturefactory.hpp>

#endif // EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )

using namespace EE::System;
using namespace EE::Graphics;
using namespace EE::Math;

namespace eproc {

#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )

// X11 backend -----------------------------------------------------------------------------------

struct GuiWindowTracker::X11Api {
	using OpenDisplayFn = Display* ( * )( const char* );
	using CloseDisplayFn = int ( * )( Display* );
	using DefaultRootWindowFn = Window ( * )( Display* );
	using InternAtomFn = Atom ( * )( Display*, const char*, Bool );
	using GetWindowPropertyFn = int ( * )( Display*, Window, Atom, long, long, Bool, Atom, Atom*,
										   int*, unsigned long*, unsigned long*, unsigned char** );
	using FreeFn = int ( * )( void* );
	using SyncFn = int ( * )( Display*, Bool );
	using SetErrorHandlerFn = XErrorHandler ( * )( XErrorHandler );
	using QueryTreeFn = Status ( * )( Display*, Window, Window*, Window*, Window**, unsigned int* );

	void* library{ nullptr };
	OpenDisplayFn openDisplay{ nullptr };
	CloseDisplayFn closeDisplay{ nullptr };
	DefaultRootWindowFn defaultRootWindow{ nullptr };
	InternAtomFn internAtom{ nullptr };
	GetWindowPropertyFn getWindowProperty{ nullptr };
	FreeFn freeMemory{ nullptr };
	SyncFn sync{ nullptr };
	SetErrorHandlerFn setErrorHandler{ nullptr };
	QueryTreeFn queryTree{ nullptr };

	~X11Api() {
		if ( library )
			Sys::unloadObject( library );
	}

	template <typename Function> Function resolve( const char* name ) {
		return reinterpret_cast<Function>( Sys::loadFunction( library, name ) );
	}

	bool load() {
		library = Sys::loadObject( "libX11.so.6" );
		if ( !library )
			library = Sys::loadObject( "libX11.so" );

		if ( !library )
			return false;

		openDisplay = resolve<OpenDisplayFn>( "XOpenDisplay" );
		closeDisplay = resolve<CloseDisplayFn>( "XCloseDisplay" );
		defaultRootWindow = resolve<DefaultRootWindowFn>( "XDefaultRootWindow" );
		internAtom = resolve<InternAtomFn>( "XInternAtom" );
		getWindowProperty = resolve<GetWindowPropertyFn>( "XGetWindowProperty" );
		freeMemory = resolve<FreeFn>( "XFree" );
		sync = resolve<SyncFn>( "XSync" );
		setErrorHandler = resolve<SetErrorHandlerFn>( "XSetErrorHandler" );
		queryTree = resolve<QueryTreeFn>( "XQueryTree" );

		return openDisplay && closeDisplay && defaultRootWindow && internAtom &&
			   getWindowProperty && freeMemory && sync && setErrorHandler && queryTree;
	}
};

// A window list holds a handful of entries in practice. The request length is capped so that a
// malformed or hostile property can never make the monitor allocate an unbounded amount of memory.
static constexpr long MAX_PROPERTY_ITEMS = 1 << 16;
// A 256x256 icon plus smaller alternatives fits comfortably inside this one MiB X11 reply cap.
static constexpr long MAX_ICON_PROPERTY_ITEMS = 1 << 18;
static constexpr unsigned MAX_ICON_READS_PER_REFRESH = 4;
static constexpr EE::Uint64 TRAY_SCAN_INTERVAL = 5;
static constexpr size_t MAX_TRAY_SCAN_WINDOWS = 4096;
static constexpr unsigned MAX_TRAY_SCAN_DEPTH = 16;

// Xlib terminates the process on any X error it is not told about, and a window listed in
// _NET_CLIENT_LIST can be destroyed by its owner between reading the list and reading that
// window's PID, in which case XGetWindowProperty reports BadWindow for a window that no longer
// exists. That race is expected here, so BadWindow is ignored, while any other error keeps the
// behaviour of the handler installed by the application (libX11's own handler when there is none).
static XErrorHandler sPreviousErrorHandler = nullptr;

static int ignoreMissingWindowError( Display* display, XErrorEvent* event ) {
	if ( event && event->error_code == BadWindow )
		return 0;

	if ( sPreviousErrorHandler )
		return sPreviousErrorHandler( display, event );

	return 0;
}

// Fetches a 32-bit X property from @p window. On success returns the raw property data, to be
// released by the caller with XFree, and stores its item count in @p itemCount. Returns null -
// leaving @p itemCount at zero - when the property is missing or empty, has another type or
// format, or is longer than MAX_PROPERTY_ITEMS, in which case the reply would be truncated into an
// incomplete list.
template <typename X11Api>
static unsigned char* fetchProperty( X11Api& api, Display* display, Window window, Atom property,
									 Atom expectedType, unsigned long& itemCount,
									 long maxItems = MAX_PROPERTY_ITEMS ) {
	itemCount = 0;

	Atom actualType = 0;
	int actualFormat = 0;
	unsigned long bytesAfter = 0;
	unsigned char* data = nullptr;

	if ( api.getWindowProperty( display, window, property, 0, maxItems, False, expectedType,
								&actualType, &actualFormat, &itemCount, &bytesAfter,
								&data ) != Success ||
		 !data ) {
		itemCount = 0;
		if ( data )
			api.freeMemory( data );
		return nullptr;
	}

	// A property stored under another type or format cannot be read as a list of ids.
	if ( actualType != expectedType || actualFormat != 32 || itemCount == 0 || bytesAfter != 0 ) {
		itemCount = 0;
		api.freeMemory( data );
		return nullptr;
	}

	return data;
}

GuiWindowTracker::GuiWindowTracker() {
	// An XWayland DISPLAY is useful too: the tracker will see only its X11 windows.
	const char* displayName = std::getenv( "DISPLAY" );
	if ( !displayName || !*displayName )
		return;

	mX11 = new X11Api();
	if ( !mX11->load() ) {
		delete mX11;
		mX11 = nullptr;
		return;
	}

	Display* display = mX11->openDisplay( nullptr );
	if ( !display ) {
		Log::warning(
			"eproc: could not open an X11 display, no process will be reported as having a GUI "
			"window" );
		return;
	}

	mDisplay = display;
	mRootWindow = mX11->defaultRootWindow( display );
	mIconAtom = mX11->internAtom( display, "_NET_WM_ICON", False );
	mXEmbedAtom = mX11->internAtom( display, "_XEMBED_INFO", False );
}

GuiWindowTracker::~GuiWindowTracker() {
	if ( mDisplay )
		mX11->closeDisplay( static_cast<Display*>( mDisplay ) );

	delete mX11;
}

void GuiWindowTracker::recordWindow( long pid, unsigned long window ) {
	mPids.insert( pid );
	mWindows.emplace_back( pid, window );
	auto cached = mIconCache.find( window );
	if ( cached != mIconCache.end() )
		cached->second.lastSeenPass = mRefreshPass;
}

long GuiWindowTracker::xembedWindowPid( unsigned long window, unsigned long pidAtom ) {
	Display* display = static_cast<Display*>( mDisplay );
	unsigned long infoCount = 0;
	unsigned long* info = reinterpret_cast<unsigned long*>(
		fetchProperty( *mX11, display, window, mXEmbedAtom, mXEmbedAtom, infoCount, 2 ) );
	if ( !info )
		return 0;
	const bool mapped = infoCount >= 2 && ( info[1] & 1 ) != 0;
	mX11->freeMemory( info );
	if ( !mapped )
		return 0;

	unsigned long pidCount = 0;
	unsigned long* pid = reinterpret_cast<unsigned long*>(
		fetchProperty( *mX11, display, window, pidAtom, XA_CARDINAL, pidCount, 1 ) );
	if ( !pid )
		return 0;
	const long owner =
		pid[0] > 0 && pid[0] <= static_cast<unsigned long>( std::numeric_limits<long>::max() )
			? static_cast<long>( pid[0] )
			: 0;
	mX11->freeMemory( pid );
	return owner;
}

void GuiWindowTracker::scanXEmbedWindows( unsigned long pidAtom ) {
	mTrayWindows.clear();
	if ( mXEmbedAtom == 0 )
		return;
	Display* display = static_cast<Display*>( mDisplay );
	std::vector<std::pair<Window, unsigned>> pending;
	pending.emplace_back( mRootWindow, 0 );
	size_t visited = 0;
	while ( !pending.empty() && visited < MAX_TRAY_SCAN_WINDOWS ) {
		const auto [window, depth] = pending.back();
		pending.pop_back();
		++visited;
		if ( depth > 0 ) {
			const long pid = xembedWindowPid( window, pidAtom );
			if ( pid > 0 )
				mTrayWindows.emplace_back( pid, window );
		}
		if ( depth >= MAX_TRAY_SCAN_DEPTH )
			continue;
		Window root = 0;
		Window parent = 0;
		Window* children = nullptr;
		unsigned int count = 0;
		if ( mX11->queryTree( display, window, &root, &parent, &children, &count ) ) {
			for ( unsigned int i = 0; i < count && pending.size() < MAX_TRAY_SCAN_WINDOWS; ++i )
				pending.emplace_back( children[i], depth + 1 );
		}
		if ( children )
			mX11->freeMemory( children );
	}
}

void GuiWindowTracker::refresh() {
	++mRefreshPass;
	mIconReadsThisPass = 0;
	mPids.clear();
	mWindows.clear();

	Display* display = static_cast<Display*>( mDisplay );
	if ( !display )
		return;

	Atom clientListAtom = mX11->internAtom( display, "_NET_CLIENT_LIST", True );
	Atom pidAtom = mX11->internAtom( display, "_NET_WM_PID", True );
	if ( pidAtom == 0 )
		return;

	XErrorHandler previousHandler = mX11->setErrorHandler( ignoreMissingWindowError );

	unsigned long windowCount = 0;
	unsigned long* windows =
		clientListAtom == 0
			? nullptr
			: reinterpret_cast<unsigned long*>( fetchProperty(
				  *mX11, display, mRootWindow, clientListAtom, XA_WINDOW, windowCount ) );

	if ( windows ) {
		for ( unsigned long i = 0; i < windowCount; i++ ) {
			if ( windows[i] == 0 )
				continue;

			unsigned long pidCount = 0;
			unsigned long* pid = reinterpret_cast<unsigned long*>(
				fetchProperty( *mX11, display, windows[i], pidAtom, XA_CARDINAL, pidCount ) );

			if ( pid ) {
				if ( pid[0] > 0 &&
					 pid[0] <= static_cast<unsigned long>( std::numeric_limits<long>::max() ) ) {
					const long owner = static_cast<long>( pid[0] );
					recordWindow( owner, windows[i] );
				}
				mX11->freeMemory( pid );
			}
		}

		mX11->freeMemory( windows );
	}
	if ( ( mRefreshPass - 1 ) % TRAY_SCAN_INTERVAL == 0 ) {
		scanXEmbedWindows( pidAtom );
	} else {
		// Revalidate cached tray icons cheaply between full scans so closed icons disappear
		// promptly.
		for ( auto it = mTrayWindows.begin(); it != mTrayWindows.end(); ) {
			if ( xembedWindowPid( it->second, pidAtom ) != it->first )
				it = mTrayWindows.erase( it );
			else
				++it;
		}
	}
	for ( const auto& [pid, window] : mTrayWindows )
		recordWindow( pid, window );

	// Flush the requests issued above so that the errors they produced are handled here instead of
	// reaching the application's handler once the previous one is restored.
	mX11->sync( display, False );
	mX11->setErrorHandler( previousHandler );
	for ( auto it = mIconCache.begin(); it != mIconCache.end(); ) {
		if ( it->second.lastSeenPass != mRefreshPass )
			it = mIconCache.erase( it );
		else
			++it;
	}
}

DrawablePtr GuiWindowTracker::readIcon( unsigned long window ) {
	if ( !mDisplay || mIconAtom == 0 )
		return {};
	Display* display = static_cast<Display*>( mDisplay );
	XErrorHandler previousHandler = mX11->setErrorHandler( ignoreMissingWindowError );
	unsigned long itemCount = 0;
	unsigned long* property = reinterpret_cast<unsigned long*>( fetchProperty(
		*mX11, display, window, mIconAtom, XA_CARDINAL, itemCount, MAX_ICON_PROPERTY_ITEMS ) );
	mX11->sync( display, False );
	mX11->setErrorHandler( previousHandler );
	if ( !property )
		return {};

	const Uint32 iconPx = static_cast<Uint32>( PixelDensity::dpToPxI( 16 ) );
	WindowIconPixels pixels = decodeWindowIcon( property, itemCount, iconPx );
	mX11->freeMemory( property );
	if ( !pixels.valid() )
		return {};

	Image image( static_cast<const Uint8*>( pixels.rgba.data() ), pixels.width, pixels.height, 4 );
	if ( image.getWidth() != iconPx || image.getHeight() != iconPx )
		image.resize( iconPx, iconPx, Image::RESAMPLER_LANCZOS4 );
	if ( !image.getPixelsPtr() )
		return {};
	const std::string name = "eproc-x11-window-" + std::to_string( window );
	TexturePtr texture = TextureFactory::instance()->loadFromPixels(
		image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false,
		Texture::ClampMode::ClampToEdge, false, false, name );
	if ( !texture )
		return {};

	auto* glyph = GlyphDrawable::New( texture, Rect( 0, 0, 1, 1 ), Sizef( iconPx, iconPx ), name );
	glyph->setDrawMode( GlyphDrawable::DrawMode::Image );
	glyph->setGlyphRenderMode( GlyphRenderMode::Color );
	glyph->setPixelDensity( PixelDensity::getPixelDensity() );
	return DrawablePtr( glyph );
}

DrawablePtr GuiWindowTracker::iconForPid( long pid ) {
	for ( const auto& [owner, window] : mWindows ) {
		if ( owner != pid )
			continue;
		auto& cached = mIconCache[window];
		cached.lastSeenPass = mRefreshPass;
		if ( !cached.attempted ||
			 ( !cached.icon && mRefreshPass - cached.lastAttemptPass >= 60 ) ) {
			if ( mIconReadsThisPass >= MAX_ICON_READS_PER_REFRESH )
				continue;
			++mIconReadsThisPass;
			cached.icon = readIcon( window );
			cached.lastAttemptPass = mRefreshPass;
			cached.attempted = true;
		}
		if ( cached.icon )
			return cached.icon;
	}
	return {};
}

bool GuiWindowTracker::isAvailable() const {
	return mDisplay != nullptr;
}

#else // X11 backend

// Stub backend ---------------------------------------------------------------------------------

// No window list can be read on this platform, so a tracker always reports itself unavailable and
// never lists a window. The bodies below exist to keep every method defined for every build.
GuiWindowTracker::GuiWindowTracker() {}

GuiWindowTracker::~GuiWindowTracker() {}

void GuiWindowTracker::refresh() {
	mPids.clear();
}

DrawablePtr GuiWindowTracker::iconForPid( long ) {
	return {};
}

bool GuiWindowTracker::isAvailable() const {
	return false;
}

#endif // X11 backend

// Shared by every backend: with no backend built, mPids is always empty.
bool GuiWindowTracker::hasWindowForPid( long pid ) const {
	return mPids.find( pid ) != mPids.end();
}

} // namespace eproc
