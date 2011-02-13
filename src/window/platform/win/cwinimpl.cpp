#include "cwinimpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_WIN

#include "../../cwindow.hpp"

namespace EE { namespace Window { namespace Platform {

cWinImpl::cWinImpl( cWindow * window, eeWindowHandler handler ) :
	cPlatformImpl( window ),
	mHandler( handler )
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

}}}

#endif
