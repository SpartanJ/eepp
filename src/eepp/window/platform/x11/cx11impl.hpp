#ifndef EE_WINDOWCX11IMPL_HPP
#define EE_WINDOWCX11IMPL_HPP

#include <eepp/core.hpp>

#if defined( EE_X11_PLATFORM )

#include <eepp/window/cplatformimpl.hpp>

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cX11Impl : public cPlatformImpl {
	public:
		typedef void (*LockFunc)(void);
		typedef void (*UnlockFunc)(void);
		
		cX11Impl( Window::cWindow * window, eeWindowHandle display, X11Window xwindow, X11Window mainwindow, LockFunc lock, UnlockFunc unlock );
		
		~cX11Impl();
		
		void MinimizeWindow();

		void MaximizeWindow();

		bool IsWindowMaximized();

		void HideWindow();

		void RaiseWindow();

		void ShowWindow();

		void MoveWindow( int left, int top );
		
		void SetContext( eeWindowContex Context );

		eeVector2i Position();

		void ShowMouseCursor();

		void HideMouseCursor();

		cCursor * CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name );

		cCursor * CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name );

		cCursor * CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name );

		void SetMouseCursor( cCursor * cursor );

		void SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor );

		void RestoreCursor();

		eeWindowHandle GetDisplay() const;

		void Lock();

		void Unlock();
	protected:
		eeWindowHandle	mDisplay;
		X11Window		mX11Window;
		X11Window		mMainWindow;
		LockFunc		mLock;
		UnlockFunc		mUnlock;
		X11Cursor		mCursorCurrent;
		X11Cursor		mCursorInvisible;
		X11Cursor		mCursorSystemLast;
		bool			mCursorHidden;
};

}}}

#endif

#endif
