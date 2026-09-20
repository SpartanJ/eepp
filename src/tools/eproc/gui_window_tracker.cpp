#include "gui_window_tracker.hpp"

#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>

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

#include <X11/Xatom.h>
#include <X11/Xlib.h>

#endif // EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )

using namespace EE::System;

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

	void* library{ nullptr };
	OpenDisplayFn openDisplay{ nullptr };
	CloseDisplayFn closeDisplay{ nullptr };
	DefaultRootWindowFn defaultRootWindow{ nullptr };
	InternAtomFn internAtom{ nullptr };
	GetWindowPropertyFn getWindowProperty{ nullptr };
	FreeFn freeMemory{ nullptr };
	SyncFn sync{ nullptr };
	SetErrorHandlerFn setErrorHandler{ nullptr };

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

		return openDisplay && closeDisplay && defaultRootWindow && internAtom &&
			   getWindowProperty && freeMemory && sync && setErrorHandler;
	}
};

// A window list holds a handful of entries in practice. The request length is capped so that a
// malformed or hostile property can never make the monitor allocate an unbounded amount of memory.
static constexpr long MAX_PROPERTY_ITEMS = 1 << 16;

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
									 Atom expectedType, unsigned long& itemCount ) {
	itemCount = 0;

	Atom actualType = None;
	int actualFormat = 0;
	unsigned long bytesAfter = 0;
	unsigned char* data = nullptr;

	if ( api.getWindowProperty( display, window, property, 0, MAX_PROPERTY_ITEMS, False,
								expectedType, &actualType, &actualFormat, &itemCount, &bytesAfter,
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
}

GuiWindowTracker::~GuiWindowTracker() {
	if ( mDisplay )
		mX11->closeDisplay( static_cast<Display*>( mDisplay ) );

	delete mX11;
}

void GuiWindowTracker::refresh() {
	mPids.clear();

	Display* display = static_cast<Display*>( mDisplay );
	if ( !display )
		return;

	Atom clientListAtom = mX11->internAtom( display, "_NET_CLIENT_LIST", True );
	Atom pidAtom = mX11->internAtom( display, "_NET_WM_PID", True );
	if ( clientListAtom == None || pidAtom == None )
		return;

	XErrorHandler previousHandler = mX11->setErrorHandler( ignoreMissingWindowError );

	unsigned long windowCount = 0;
	unsigned long* windows = reinterpret_cast<unsigned long*>(
		fetchProperty( *mX11, display, mRootWindow, clientListAtom, XA_WINDOW, windowCount ) );

	if ( windows ) {
		for ( unsigned long i = 0; i < windowCount; i++ ) {
			if ( windows[i] == None )
				continue;

			unsigned long pidCount = 0;
			unsigned long* pid = reinterpret_cast<unsigned long*>(
				fetchProperty( *mX11, display, windows[i], pidAtom, XA_CARDINAL, pidCount ) );

			if ( pid ) {
				if ( pid[0] > 0 )
					mPids.insert( static_cast<long>( pid[0] ) );
				mX11->freeMemory( pid );
			}
		}

		mX11->freeMemory( windows );
	}

	// Flush the requests issued above so that the errors they produced are handled here instead of
	// reaching the application's handler once the previous one is restored.
	mX11->sync( display, False );
	mX11->setErrorHandler( previousHandler );
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

bool GuiWindowTracker::isAvailable() const {
	return false;
}

#endif // X11 backend

// Shared by every backend: with no backend built, mPids is always empty.
bool GuiWindowTracker::hasWindowForPid( long pid ) const {
	return mPids.find( pid ) != mPids.end();
}

} // namespace eproc
