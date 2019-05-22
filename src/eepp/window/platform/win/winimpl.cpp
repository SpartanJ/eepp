#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>
#undef RGB

static BOOL WIN_showWindow( HWND hWnd, int nCmdShow ) {
	return ShowWindow( hWnd, nCmdShow );
}

#include <eepp/window/platform/win/winimpl.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/platform/win/cursorwin.hpp>

namespace EE { namespace Window { namespace Platform {

static HCURSOR SYS_CURSORS[ Cursor::SysCursorCount ] = {0};

static HCURSOR GetLoadCursor( const Cursor::SysType& cursor, LPCSTR syscur ) {
	if ( 0 == SYS_CURSORS[ cursor ] ) {
		SYS_CURSORS[ cursor ] = LoadCursor( NULL, syscur );
	}

	return SYS_CURSORS[ cursor ];
}

WinImpl::WinImpl( EE::Window::Window * window, eeWindowHandle handler ) :
	PlatformImpl( window ),
	mHandler( handler ),
	mCursorCurrent( NULL ),
	mCursorHidden( false )
{
}

WinImpl::~WinImpl() {
}

void WinImpl::minimizeWindow() {
	WIN_showWindow( mHandler, SW_MINIMIZE );
}

void WinImpl::maximizeWindow() {
	WIN_showWindow( mHandler, SW_MAXIMIZE );
}

bool WinImpl::isWindowMaximized() {
	return 0 != IsZoomed( mHandler );
}

void WinImpl::hideWindow() {
	WIN_showWindow( mHandler, SW_HIDE );
}

void WinImpl::raiseWindow() {
	HWND top;

	if ( !mWindow->isWindowed() )
		top = HWND_TOPMOST;
	else
		top = HWND_NOTOPMOST;

	SetWindowPos( mHandler, top, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE) );
}

void WinImpl::showWindow() {
	WIN_showWindow( mHandler, SW_SHOW );
}

void WinImpl::moveWindow( int left, int top ) {
	SetWindowPos( mHandler, NULL, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

void WinImpl::setContext( eeWindowContex Context ) {
	wglMakeCurrent( (HDC)GetDC( mHandler ), (HGLRC)Context );
}

Vector2i WinImpl::getPosition() {
	RECT r;
	GetWindowRect( mHandler, &r );
	return Vector2i( r.left, r.top );
}

void WinImpl::showMouseCursor() {
	mCursorHidden = false;

	if ( !mCursorCurrent ) {
		setSystemMouseCursor( Cursor::SysArrow );
	} else {
		SetCursor( (HCURSOR)mCursorCurrent );
		SetClassLong( getHandler(), GCL_HCURSOR, (DWORD)mCursorCurrent );

		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

void WinImpl::hideMouseCursor() {
	if ( mCursorHidden )
		return;

	mCursorHidden = true;

	PostMessage( mHandler, WM_SETCURSOR, 0, 0 );
}

Cursor * WinImpl::createMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorWin, ( tex, hotspot, name, mWindow ) );
}

Cursor * WinImpl::createMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorWin, ( img, hotspot, name, mWindow ) );
}

Cursor * WinImpl::createMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) {
	return eeNew( CursorWin, ( path, hotspot, name, mWindow ) );
}

void WinImpl::setMouseCursor( Cursor * cursor ) {
	mCursorCurrent = reinterpret_cast<CursorWin*> ( cursor )->getCursor();

	if ( !mCursorHidden ) {
		SetCursor( (HCURSOR)mCursorCurrent );
		SetClassLong( getHandler(), GCL_HCURSOR, (DWORD)mCursorCurrent );

		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

void WinImpl::setSystemMouseCursor( Cursor::SysType syscursor ) {
	HCURSOR mc;

	switch ( syscursor ) {
		case Cursor::SysArrow:
			mc = GetLoadCursor(Cursor::SysArrow, IDC_ARROW); break;
		case Cursor::SysWait:
			mc = GetLoadCursor(Cursor::SysWait, IDC_WAIT); break;
		case Cursor::SysCrosshair:
			mc = GetLoadCursor(Cursor::SysCrosshair, IDC_CROSS); break;
		case Cursor::SysIBeam:
			mc = GetLoadCursor(Cursor::SysIBeam, IDC_IBEAM); break;
		case Cursor::SysWaitArrow:
			mc = GetLoadCursor(Cursor::SysWaitArrow, IDC_WAIT); break;
		case Cursor::SysSizeNWSE:
			mc = GetLoadCursor(Cursor::SysSizeNWSE, IDC_SIZENWSE); break;
		case Cursor::SysSizeNESW:
			mc = GetLoadCursor(Cursor::SysSizeNESW, IDC_SIZENESW); break;
		case Cursor::SysSizeWE:
			mc = GetLoadCursor(Cursor::SysSizeWE, IDC_SIZEWE); break;
		case Cursor::SysSizeNS:
			mc = GetLoadCursor(Cursor::SysSizeNS, IDC_SIZENS); break;
		case Cursor::SysSizeAll:
			mc = GetLoadCursor(Cursor::SysSizeAll, IDC_SIZEALL); break;
		case Cursor::SysNoCursor:
			mc = GetLoadCursor(Cursor::SysNoCursor, IDC_NO); break;
		case Cursor::SysHand:
			mc = GetLoadCursor(Cursor::SysHand, IDC_HAND); break;
		default:
			return;
	}

	mCursorCurrent = mc;

	if ( !mCursorHidden ) {
		SetCursor( mc );
		SetClassLong( getHandler(), GCL_HCURSOR, (DWORD)mc );

		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

void WinImpl::restoreCursor() {
	if ( !mCursorHidden ) {
		showMouseCursor();
	} else {
		hideMouseCursor();
	}
}

eeWindowHandle WinImpl::getHandler() const {
	return mHandler;
}

eeWindowContex WinImpl::getWindowContext() {
	return wglGetCurrentContext();
}

}}}

#endif
