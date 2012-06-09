#include <eepp/window/platform/x11/cx11impl.hpp>
#include <eepp/window/platform/x11/ccursorx11.hpp>

#if defined( EE_X11_PLATFORM )

using namespace EE::Window::Cursor;

namespace EE { namespace Window { namespace Platform {

cX11Impl::cX11Impl( cWindow * window, eeWindowHandler display, X11Window xwindow, X11Window mainwindow, LockFunc lock, UnlockFunc unlock ) :
	cPlatformImpl( window ),
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

cX11Impl::~cX11Impl() {
	if ( None != mCursorInvisible )
		XFreeCursor( mDisplay, mCursorInvisible );

	if ( None != mCursorSystemLast )
		XFreeCursor( mDisplay, mCursorSystemLast );
}

void cX11Impl::MinimizeWindow() {
	Lock();

	XIconifyWindow( mDisplay, mX11Window, 0 );

	XFlush( mDisplay );

	Unlock();
}

void cX11Impl::MaximizeWindow() {
	// coded by Rafał Maj, idea from Måns Rullgård http://tinyurl.com/68mvk3
	Lock();

	XEvent xev;
	Atom wm_state =  XInternAtom( mDisplay, "_NET_WM_STATE", False);
	Atom maximizeV = XInternAtom( mDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	Atom maximizeH = XInternAtom( mDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

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

	Unlock();
}

void cX11Impl::HideWindow() {
	Lock();

	XUnmapWindow( mDisplay, mX11Window );

	Unlock();
}

void cX11Impl::RaiseWindow() {
	Lock();

	XRaiseWindow( mDisplay, mX11Window );

	Unlock();
}

void cX11Impl::ShowWindow() {
	Lock();

	XMapRaised( mDisplay, mX11Window );

	Unlock();
}

void cX11Impl::MoveWindow( int left, int top ) {
	Lock();

	XMoveWindow( mDisplay, mX11Window, left, top );

	XFlush( mDisplay );

	Unlock();
}

void cX11Impl::SetContext( eeWindowContex Context ) {
	Lock();

	glXMakeCurrent( mDisplay, mX11Window, Context );

	Unlock();
}

eeVector2i cX11Impl::Position() {
	int x, y;
	X11Window child_return;

	XTranslateCoordinates ( mDisplay, mX11Window, DefaultRootWindow( mDisplay ), 0, 0, &x, &y, &child_return );

	return eeVector2i( x, y );
}

void cX11Impl::ShowMouseCursor() {
	if ( !mCursorHidden )
	  return;

	Lock();

	XDefineCursor( mDisplay, mMainWindow, mCursorCurrent );

	mCursorHidden = false;

	Unlock();
}

void cX11Impl::HideMouseCursor() {
	if ( mCursorHidden )
		return;

	Lock();

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

	Unlock();
}

cCursor * cX11Impl::CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorX11, ( tex, hotspot, name, mWindow ) );
}

cCursor * cX11Impl::CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorX11, ( img, hotspot, name, mWindow ) );
}

cCursor * cX11Impl::CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorX11, ( path, hotspot, name, mWindow ) );
}

void cX11Impl::SetMouseCursor( cCursor * cursor ) {
	mCursorCurrent = reinterpret_cast<cCursorX11*>( cursor )->GetCursor();

	if ( !mCursorHidden ) {
		Lock();

		XDefineCursor( mDisplay, mMainWindow,  mCursorCurrent );

		Unlock();
	}
}

void cX11Impl::RestoreCursor() {
	if ( !mCursorHidden ) {
		Lock();

		XDefineCursor( mDisplay, mMainWindow,  mCursorCurrent );

		Unlock();
	} else {
		HideMouseCursor();
	}
}

void cX11Impl::SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor ) {
	unsigned int cursor_shape;

	switch ( syscursor ) {
	  case SYS_CURSOR_DEFAULT:
	  case SYS_CURSOR_ARROW:
	  case SYS_CURSOR_PROGRESS:
		 cursor_shape = XC_left_ptr;
		 break;
	  case SYS_CURSOR_BUSY:
		 cursor_shape = XC_watch;
		 break;
	  case SYS_CURSOR_QUESTION:
		 cursor_shape = XC_question_arrow;
		 break;
	  case SYS_CURSOR_EDIT:
		 cursor_shape = XC_xterm;
		 break;
	  case SYS_CURSOR_MOVE:
		 cursor_shape = XC_fleur;
		 break;
	  case SYS_CURSOR_RESIZE_N:
		 cursor_shape = XC_top_side;
		 break;
	  case SYS_CURSOR_RESIZE_S:
		 cursor_shape = XC_bottom_side;
		 break;
	  case SYS_CURSOR_RESIZE_E:
		 cursor_shape = XC_right_side;
		 break;
	  case SYS_CURSOR_RESIZE_W:
		 cursor_shape = XC_left_side;
		 break;
	  case SYS_CURSOR_RESIZE_NE:
		 cursor_shape = XC_top_right_corner;
		 break;
	  case SYS_CURSOR_RESIZE_SW:
		 cursor_shape = XC_bottom_left_corner;
		 break;
	  case SYS_CURSOR_RESIZE_NW:
		 cursor_shape = XC_top_left_corner;
		 break;
	  case SYS_CURSOR_RESIZE_SE:
		 cursor_shape = XC_bottom_right_corner;
		 break;
	  case SYS_CURSOR_PRECISION:
		 cursor_shape = XC_crosshair;
		 break;
	  case SYS_CURSOR_LINK:
		 cursor_shape = XC_hand2;
		 break;
	  case SYS_CURSOR_ALT_SELECT:
		 cursor_shape = XC_hand1;
		 break;
	  case SYS_CURSOR_UNAVAILABLE:
		 cursor_shape = XC_X_cursor;
		 break;
	  default:
		 return;
	}

	if ( None != mCursorCurrent ) {
		XFreeCursor( mDisplay, mCursorSystemLast );
	}

	Lock();

	mCursorCurrent		= XCreateFontCursor( mDisplay, cursor_shape );
	mCursorSystemLast	= mCursorCurrent;

	if ( !mCursorHidden ) {
		XDefineCursor( mDisplay, mMainWindow, mCursorCurrent );
	}

	Unlock();
}

eeWindowHandler cX11Impl::GetDisplay() const {
	return mDisplay;
}

void cX11Impl::Lock() {
	if ( NULL != mLock )
		mLock();
}

void cX11Impl::Unlock() {
	if ( NULL != mUnlock )
		mUnlock();
}

}}}

#endif
