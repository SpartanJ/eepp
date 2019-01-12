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
		
		void minimizeWindow();

		void maximizeWindow();

		bool isWindowMaximized();

		void hideWindow();

		void raiseWindow();

		void showWindow();

		void moveWindow( int left, int top );
		
		void setContext( eeWindowContex Context );

		Vector2i getPosition();

		void showMouseCursor();

		void hideMouseCursor();

		Cursor * createMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name );

		Cursor * createMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name );

		Cursor * createMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name );

		void setMouseCursor( Cursor * cursor );

		void setSystemMouseCursor( Cursor::SysType syscursor );

		void restoreCursor();

		eeWindowHandle GetDisplay() const;

		void Lock();

		void Unlock();

		eeWindowContex getWindowContext();
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
