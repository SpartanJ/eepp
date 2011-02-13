#include "cx11impl.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX

namespace EE { namespace Window { namespace Platform {

cX11Impl::cX11Impl( cWindow * window, eeWindowHandler display, X11Window xwindow, LockFunc lock, UnlockFunc unlock ) :
	cPlatformImpl( window ),
	mDisplay( display ),
	mWindow( xwindow ),
	mLock( lock ),
	mUnlock( unlock )
{
}

cX11Impl::~cX11Impl() {
}

void cX11Impl::MinimizeWindow() {
	mLock();
	XIconifyWindow( mDisplay, DefaultRootWindow( mDisplay ), 0 );
	XFlush(mDisplay);
	mUnlock();
}

void cX11Impl::MaximizeWindow() {
	// coded by Rafał Maj, idea from Måns Rullgård http://tinyurl.com/68mvk3
	mLock();

	XEvent xev;
	Atom wm_state =  XInternAtom( mDisplay, "_NET_WM_STATE", False);
	Atom maximizeV = XInternAtom( mDisplay, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	Atom maximizeH = XInternAtom( mDisplay, "_NET_WM_STATE_MAXIMIZED_HORZ", False);

	memset( &xev, 0, sizeof(xev) );
	xev.type = ClientMessage;
	xev.xclient.window = mWindow;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = 1;
	xev.xclient.data.l[1] = maximizeV;
	xev.xclient.data.l[2] = maximizeH;
	xev.xclient.data.l[3] = 0;
	XSendEvent( mDisplay, DefaultRootWindow(mDisplay), 0, SubstructureNotifyMask|SubstructureRedirectMask, &xev);

	XFlush(mDisplay);

	mUnlock();
}

void cX11Impl::HideWindow() {
	mLock();
	XUnmapWindow( mDisplay, mWindow );
	mUnlock();
}

void cX11Impl::RaiseWindow() {
	mLock();
	XRaiseWindow( mDisplay, mWindow );
	mUnlock();
}

void cX11Impl::ShowWindow() {
	mLock();
	XMapRaised( mDisplay, mWindow );
	mUnlock();
}

void cX11Impl::MoveWindow( int left, int top ) {
	mLock();
	XMoveWindow( mDisplay, mWindow, left, top );
	XFlush( mDisplay );
	mUnlock();
}

void cX11Impl::SetContext( eeWindowContex Context ) {
	/// FIXME: This is wrong
	/*mLock();
	glXMakeCurrent( mDisplay, mWindow, Context );
	mUnlock();*/
}

eeVector2i cX11Impl::Position() {
	XWindowAttributes Attrs;
	XGetWindowAttributes( mDisplay, mWindow, &Attrs );
	return eeVector2i( Attrs.x, Attrs.y );
}

}}}

#endif
