#include <eepp/declares.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <windows.h>
#undef CreateWindow

static BOOL WIN_ShowWindow( HWND hWnd, int nCmdShow ) {
	return ShowWindow( hWnd, nCmdShow );
}

#include <eepp/window/platform/win/cwinimpl.hpp>
#include <eepp/window/cwindow.hpp>
#include <eepp/window/platform/win/ccursorwin.hpp>

using namespace EE::Window::Cursor;

namespace EE { namespace Window { namespace Platform {

static HCURSOR SYS_CURSORS[ SYS_CURSOR_COUNT ] = {0};

static HCURSOR GetLoadCursor( const EE_SYSTEM_CURSOR& cursor, LPCSTR syscur ) {
	if ( 0 == SYS_CURSORS[ cursor ] ) {
		SYS_CURSORS[ cursor ] = LoadCursor( NULL, syscur );
	}

	return SYS_CURSORS[ cursor ];
}

cWinImpl::cWinImpl( cWindow * window, eeWindowHandle handler ) :
	cPlatformImpl( window ),
	mHandler( handler ),
	mCursorCurrent( NULL ),
	mCursorHidden( false )
{
}

cWinImpl::~cWinImpl() {
}

void cWinImpl::MinimizeWindow() {
	WIN_ShowWindow( mHandler, SW_MINIMIZE );
}

void cWinImpl::MaximizeWindow() {
	WIN_ShowWindow( mHandler, SW_MAXIMIZE );
}

bool cWinImpl::IsWindowMaximized() {
	return 0 != IsZoomed( mHandler );
}

void cWinImpl::HideWindow() {
	WIN_ShowWindow( mHandler, SW_HIDE );
}

void cWinImpl::RaiseWindow() {
	HWND top;

	if ( !mWindow->Windowed() )
		top = HWND_TOPMOST;
	else
		top = HWND_NOTOPMOST;

	SetWindowPos( mHandler, top, 0, 0, 0, 0, (SWP_NOMOVE | SWP_NOSIZE) );
}

void cWinImpl::ShowWindow() {
	WIN_ShowWindow( mHandler, SW_SHOW );
}

void cWinImpl::MoveWindow( int left, int top ) {
	SetWindowPos( mHandler, NULL, left, top, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
}

void cWinImpl::SetContext( eeWindowContex Context ) {
	wglMakeCurrent( (HDC)GetDC( mHandler ), (HGLRC)Context );
}

eeVector2i cWinImpl::Position() {
	RECT r;
	GetWindowRect( mHandler, &r );
	return eeVector2i( r.left, r.top );
}

void cWinImpl::ShowMouseCursor() {
	mCursorHidden = false;

	if ( !mCursorCurrent ) {
		SetSystemMouseCursor( Cursor::SYS_CURSOR_ARROW );
	} else {
		SetCursor( (HCURSOR)mCursorCurrent );
		SetClassLong( GetHandler(), GCL_HCURSOR, (DWORD)mCursorCurrent );

		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

void cWinImpl::HideMouseCursor() {
	if ( mCursorHidden )
		return;

	mCursorHidden = true;

	PostMessage( mHandler, WM_SETCURSOR, 0, 0 );
}

cCursor * cWinImpl::CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorWin, ( tex, hotspot, name, mWindow ) );
}

cCursor * cWinImpl::CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorWin, ( img, hotspot, name, mWindow ) );
}

cCursor * cWinImpl::CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name ) {
	return eeNew( cCursorWin, ( path, hotspot, name, mWindow ) );
}

void cWinImpl::SetMouseCursor( cCursor * cursor ) {
	mCursorCurrent = reinterpret_cast<cCursorWin*> ( cursor )->GetCursor();

	if ( !mCursorHidden ) {
		SetCursor( (HCURSOR)mCursorCurrent );
		SetClassLong( GetHandler(), GCL_HCURSOR, (DWORD)mCursorCurrent );

		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

void cWinImpl::SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor ) {
	HCURSOR mc;

	switch ( syscursor ) {
		case SYS_CURSOR_ARROW:
			mc = GetLoadCursor(SYS_CURSOR_ARROW, IDC_ARROW); break;
		case SYS_CURSOR_WAIT:
			mc = GetLoadCursor(SYS_CURSOR_WAIT, IDC_WAIT); break;
		case SYS_CURSOR_CROSSHAIR:
			mc = GetLoadCursor(SYS_CURSOR_CROSSHAIR, IDC_CROSS); break;
		case SYS_CURSOR_IBEAM:
			mc = GetLoadCursor(SYS_CURSOR_IBEAM, IDC_IBEAM); break;
		case SYS_CURSOR_WAITARROW:
			mc = GetLoadCursor(SYS_CURSOR_WAITARROW, IDC_WAIT); break;
		case SYS_CURSOR_SIZENWSE:
			mc = GetLoadCursor(SYS_CURSOR_SIZENWSE, IDC_SIZENWSE); break;
		case SYS_CURSOR_SIZENESW:
			mc = GetLoadCursor(SYS_CURSOR_SIZENESW, IDC_SIZENESW); break;
		case SYS_CURSOR_SIZEWE:
			mc = GetLoadCursor(SYS_CURSOR_SIZEWE, IDC_SIZEWE); break;
		case SYS_CURSOR_SIZENS:
			mc = GetLoadCursor(SYS_CURSOR_SIZENS, IDC_SIZENS); break;
		case SYS_CURSOR_SIZEALL:
			mc = GetLoadCursor(SYS_CURSOR_SIZEALL, IDC_SIZEALL); break;
		case SYS_CURSOR_NO:
			mc = GetLoadCursor(SYS_CURSOR_NO, IDC_NO); break;
		case SYS_CURSOR_HAND:
			mc = GetLoadCursor(SYS_CURSOR_HAND, IDC_HAND); break;
		default:
			return;
	}

	mCursorCurrent = mc;

	if ( !mCursorHidden ) {
		SetCursor( mc );
		SetClassLong( GetHandler(), GCL_HCURSOR, (DWORD)mc );

		POINT p;
		GetCursorPos( &p );
		SetCursorPos( p.x, p.y );
	}
}

void cWinImpl::RestoreCursor() {
	if ( !mCursorHidden ) {
		ShowMouseCursor();
	} else {
		HideMouseCursor();
	}
}

eeWindowHandle cWinImpl::GetHandler() const {
	return mHandler;
}

}}}

#endif
