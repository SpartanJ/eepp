#ifndef EE_WINDOWCX11IMPL_HPP
#define EE_WINDOWCX11IMPL_HPP

#include <eepp/core.hpp>

#if defined( EE_X11_PLATFORM )

#include <eepp/window/platformimpl.hpp>

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class X11Impl : public PlatformImpl {
	public:
		typedef void (*LockFunc)(void);
		typedef void (*UnlockFunc)(void);
		
		X11Impl( EE::Window::Window * window, eeWindowHandle display, X11Window xwindow, X11Window mainwindow, LockFunc lock, UnlockFunc unlock );
		
		~X11Impl();
		
		void MinimizeWindow();

		void MaximizeWindow();

		bool IsWindowMaximized();

		void HideWindow();

		void RaiseWindow();

		void ShowWindow();

		void MoveWindow( int left, int top );
		
		void SetContext( eeWindowContex Context );

		Vector2i Position();

		void ShowMouseCursor();

		void HideMouseCursor();

		Cursor * CreateMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name );

		Cursor * CreateMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name );

		Cursor * CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name );

		void SetMouseCursor( Cursor * cursor );

		void SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor );

		void RestoreCursor();

		eeWindowHandle GetDisplay() const;

		void Lock();

		void Unlock();

		eeWindowContex GetWindowContext();
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
