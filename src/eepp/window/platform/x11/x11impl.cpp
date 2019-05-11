#include <eepp/config.hpp>

#if defined( EE_X11_PLATFORM )

#define GLEW_NO_GLU
#include <glew/glxew.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/cursorfont.h>
#include <climits>
#undef Window
#undef Display
#undef Cursor
#define XAtom(str) XInternAtom( mDisplay, str, False )

#include <eepp/window/platform/x11/x11impl.hpp>
#include <eepp/window/platform/x11/cursorx11.hpp>

namespace EE { namespace Window { namespace Platform {

X11Impl::X11Impl( EE::Window::Window * window, eeWindowHandle display, X11Window xwindow, X11Window mainwindow, LockFunc lock, UnlockFunc unlock ) :
	PlatformImpl( window ),
	mDisplay( display ),
	mX11Window( xwindow ),
	mMainWindow( mainwindow ),
	mLock( lock ),
	mUnlock( unlock ),
	mCursorCurrent( 0 ),
	mCursorInvisible( None ),
	mCursorSystemLast( None ),
	mCursorHidden( false )
{
}

X11Impl::~X11Impl() {
	if ( None != mCursorInvisible )
		XFreeCursor( mDisplay, mCursorInvisible );

	if ( None != mCursorSystemLast )
		XFreeCursor( mDisplay, mCursorSystemLast );
}

void X11Impl::minimizeWindow() {
	lock();

	XIconifyWindow( mDisplay, mX11Window, 0 );

	XFlush( mDisplay );

	unlock();
}

void X11Impl::maximizeWindow() {
	// coded by Rafał Maj, idea from Måns Rullgård http://tinyurl.com/68mvk3
	lock();

	XEvent xev;
	Atom wm_state =  XAtom( "_NET_WM_STATE" );
	Atom maximizeV = XAtom( "_NET_WM_STATE_MAXIMIZED_VERT" );
	Atom maximizeH = XAtom( "_NET_WM_STATE_MAXIMIZED_HORZ" );

	memset( &xev, 0, sizeof(xev) );
	xev.type = ClientMessage;
	xev.xclient.window = mX11Window;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = maximizeV;
	xev.xclient.data.l[2] = maximizeH;
	xev.xclient.data.l[3] = 0;
	XSendEvent( mDisplay, DefaultRootWindow(mDisplay), 0, SubstructureNotifyMask|SubstructureRedirectMask, &xev);

	XFlush(mDisplay);

	unlock();
}

bool X11Impl::isWindowMaximized() {
	lock();

	//bool minimized = false;
	bool maximizedhorz = false;
	bool maximizedvert = false;
	Atom type;
	int format;
	unsigned long numitems, bytesafter;
	unsigned char * properties = 0;

	XGetWindowProperty( mDisplay,
						mX11Window,
						XAtom("_NET_WM_STATE"),
						0,
						LONG_MAX,
						false,
						AnyPropertyType,
						&type,
						&format,
						&numitems,
						&bytesafter, &properties
	);

	if( properties && ( format == 32 ) ) {
		for(unsigned int i = 0; i < numitems; ++i) {
			const Atom prop = (reinterpret_cast<ulong *>(properties))[i];

			if (prop == XAtom("_NET_WM_STATE_MAXIMIZED_HORZ"))
				maximizedhorz = true;

			if (prop == XAtom("_NET_WM_STATE_MAXIMIZED_VERT"))
				maximizedvert = true;

			/*if (prop == XAtom("_NET_WM_STATE_HIDDEN"))
				minimized = true;*/
		}
	}

	XFree(properties);

	XFlush(mDisplay);

	unlock();

	if( maximizedhorz && maximizedvert ) {
		return true;
	}

	return false;
}

void X11Impl::hideWindow() {
	lock();

	XUnmapWindow( mDisplay, mX11Window );

	unlock();
}

void X11Impl::raiseWindow() {
	lock();

	XRaiseWindow( mDisplay, mX11Window );

	unlock();
}

void X11Impl::showWindow() {
	lock();

	XMapRaised( mDisplay, mX11Window );

	unlock();
}

void X11Impl::moveWindow( int left, int top ) {
	lock();

	XMoveWindow( mDisplay, mX11Window, left, top );

	XFlush( mDisplay );

	unlock();
}

void X11Impl::setContext( eeWindowContex Context ) {
	lock();

	glXMakeCurrent( mDisplay, mX11Window, Context );

	unlock();
}

Vector2i X11Impl::getPosition() {
	int x, y;
	X11Window child_return;

	XTranslateCoordinates ( mDisplay, mX11Window, DefaultRootWindow( mDisplay ), 0, 0, &x, &y, &child_return );

	return Vector2i( x, y );
}

void X11Impl::showMouseCursor() {
	if ( !mCursorHidden )
	  return;

	lock();

	XDefineCursor( mDisplay, mMainWindow, mCursorCurrent );

	mCursorHidden = false;

	unlock();
}

void X11Impl::hideMouseCursor() {
	if ( mCursorHidden )
		return;

	lock();

	if ( mCursorInvisible == None ) {
		unsigned long gcmask;
		XGCValues gcvalues;

		Pixmap pixmap = XCreatePixmap( mDisplay, mMainWindow, 1, 1, 1 );

		GC temp_gc;
		XColor color;

		gcmask = GCFunction | GCForeground | GCBackground;
		gcvalues.function = GXcopy;
		gcvalues.foreground = 0;
		gcvalues.background = 0;
		temp_gc = XCreateGC( mDisplay, pixmap, gcmask, &gcvalues);
		XDrawPoint( mDisplay, pixmap, temp_gc, 0, 0 );
		XFreeGC( mDisplay, temp_gc );
		color.pixel = 0;
		color.red = color.green = color.blue = 0;
		color.flags = DoRed | DoGreen | DoBlue;

		mCursorInvisible = XCreatePixmapCursor( mDisplay, pixmap, pixmap, &color, &color, 0, 0 );

		XFreePixmap( mDisplay, pixmap );
	}

	XDefineCursor( mDisplay, mX11Window, mCursorInvisible );

	mCursorHidden = true;

	unlock();
}

Cursor * X11Impl::createMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorX11, ( tex, hotspot, name, mWindow ) );
}

Cursor * X11Impl::createMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorX11, ( img, hotspot, name, mWindow ) );
}

Cursor * X11Impl::createMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorX11, ( path, hotspot, name, mWindow ) );
}

void X11Impl::setMouseCursor( Cursor * cursor ) {
	mCursorCurrent = reinterpret_cast<CursorX11*>( cursor )->GetCursor();

	if ( !mCursorHidden ) {
		lock();

		XDefineCursor( mDisplay, mMainWindow,  mCursorCurrent );

		unlock();
	}
}

void X11Impl::restoreCursor() {
	if ( !mCursorHidden ) {
		lock();

		XDefineCursor( mDisplay, mMainWindow,  mCursorCurrent );

		unlock();
	} else {
		hideMouseCursor();
	}
}

void X11Impl::setSystemMouseCursor( Cursor::SysType syscursor ) {
	unsigned int cursor_shape;

	switch ( syscursor ) {
		case Cursor::SysArrow:		cursor_shape = XC_arrow; break;
		case Cursor::SysWait:		cursor_shape = XC_watch; break;
		case Cursor::SysWaitArrow:	cursor_shape = XC_watch; break;
		case Cursor::SysIBeam:		cursor_shape = XC_xterm; break;
		case Cursor::SysSizeAll:	cursor_shape = XC_fleur; break;
		case Cursor::SysSizeNWSE:	cursor_shape = XC_fleur; break;
		case Cursor::SysSizeNESW:	cursor_shape = XC_fleur; break;
		case Cursor::SysSizeWE:		cursor_shape = XC_sb_h_double_arrow; break;
		case Cursor::SysSizeNS:		cursor_shape = XC_sb_v_double_arrow; break;
		case Cursor::SysCrosshair:	cursor_shape = XC_tcross; break;
		case Cursor::SysHand:		cursor_shape = XC_hand2; break;
		case Cursor::SysNoCursor:	cursor_shape = XC_pirate; break;
		default:					return;
	}

	if ( None != mCursorCurrent ) {
		XFreeCursor( mDisplay, mCursorSystemLast );
	}

	lock();

	mCursorCurrent		= XCreateFontCursor( mDisplay, cursor_shape );
	mCursorSystemLast	= mCursorCurrent;

	if ( !mCursorHidden ) {
		XDefineCursor( mDisplay, mMainWindow, mCursorCurrent );
	}

	unlock();
}

eeWindowHandle X11Impl::getDisplay() const {
	return mDisplay;
}

void X11Impl::lock() {
	if ( NULL != mLock )
		mLock();
}

void X11Impl::unlock() {
	if ( NULL != mUnlock )
		mUnlock();
}

eeWindowContex X11Impl::getWindowContext() {
	return glXGetCurrentContext();
}

}}}

#endif
