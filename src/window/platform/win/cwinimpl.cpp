#include "cwinimpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

#include "../../cwindow.hpp"

#include "ccursorwin.hpp"

using namespace EE::Window::Cursor;

namespace EE { namespace Window { namespace Platform {

cWinImpl::cWinImpl( cWindow * window, eeWindowHandler handler ) :
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
	wglMakeCurrent( GetDC( mHandler ), Context );
}

eeVector2i cWinImpl::Position() {
	RECT r;
	GetWindowRect( mHandler, &r );
	return eeVector2i( r.left, r.top );
}

void cWinImpl::ShowMouseCursor() {
    mCursorHidden = false;

	if ( !mCursorCurrent ) {
		SetSystemMouseCursor( Cursor::SYS_CURSOR_DEFAULT );
	} else {
        SetCursor( mCursorCurrent );
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
        SetCursor( mCursorCurrent );
        POINT p;
        GetCursorPos( &p );
        SetCursorPos( p.x, p.y );
    }
}

void cWinImpl::SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor ) {
	HCURSOR mc;

	switch ( syscursor ) {
		case SYS_CURSOR_DEFAULT:
		case SYS_CURSOR_ARROW:
			mc = LoadCursor(NULL, IDC_ARROW); break;
		case SYS_CURSOR_BUSY:
			mc = LoadCursor(NULL, IDC_WAIT); break;
		case SYS_CURSOR_QUESTION:
			mc = LoadCursor(NULL, IDC_HELP); break;
		case SYS_CURSOR_EDIT:
			mc = LoadCursor(NULL, IDC_IBEAM); break;
		case SYS_CURSOR_MOVE:
			mc = LoadCursor(NULL, IDC_SIZEALL); break;
		case SYS_CURSOR_RESIZE_N:
		case SYS_CURSOR_RESIZE_S:
			mc = LoadCursor(NULL, IDC_SIZENS); break;
		case SYS_CURSOR_RESIZE_E:
		case SYS_CURSOR_RESIZE_W:
			mc = LoadCursor(NULL, IDC_SIZEWE); break;
		case SYS_CURSOR_RESIZE_NE:
		case SYS_CURSOR_RESIZE_SW:
			mc = LoadCursor(NULL, IDC_SIZENESW); break;
		case SYS_CURSOR_RESIZE_NW:
		case SYS_CURSOR_RESIZE_SE:
			mc = LoadCursor(NULL, IDC_SIZENWSE); break;
		case SYS_CURSOR_PROGRESS:
			mc = LoadCursor(NULL, IDC_APPSTARTING); break;
		case SYS_CURSOR_PRECISION:
			mc = LoadCursor(NULL, IDC_CROSS); break;
		case SYS_CURSOR_LINK:
			mc = LoadCursor(NULL, IDC_HAND); break;
		case SYS_CURSOR_ALT_SELECT:
			mc = LoadCursor(NULL, IDC_UPARROW); break;
		case SYS_CURSOR_UNAVAILABLE:
			mc = LoadCursor(NULL, IDC_NO); break;
		default:
			return;

        mCursorCurrent = mc;

		if ( !mCursorHidden ) {
			SetCursor( mc );
			POINT p;
			GetCursorPos( &p );
			SetCursorPos( p.x, p.y );
		}
	}
}

void cWinImpl::RestoreCursor() {
	if ( !mCursorHidden ) {
		ShowMouseCursor();
	} else {
		HideMouseCursor();
	}
}

eeWindowHandler cWinImpl::GetHandler() const {
    return mHandler;
}

}}}

#endif
