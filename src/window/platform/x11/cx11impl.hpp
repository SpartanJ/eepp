#ifndef EE_WINDOWCX11IMPL_HPP
#define EE_WINDOWCX11IMPL_HPP

#include "../../base.hpp"
#include "../../cplatformimpl.hpp"

#if EE_PLATFORM == EE_PLATFORM_LINUX

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cX11Impl : public cPlatformImpl {
	public:
		typedef void (*LockFunc)(void);
		typedef void (*UnlockFunc)(void);
		
		cX11Impl( cWindow * window, eeWindowHandler display, X11Window xwindow, LockFunc lock, UnlockFunc unlock );
		
		~cX11Impl();
		
		void MinimizeWindow();

		void MaximizeWindow();

		void HideWindow();

		void RaiseWindow();

		void ShowWindow();

		void MoveWindow( int left, int top );
		
		void SetContext( eeWindowContex Context );

		eeVector2i Position();
	protected:
		eeWindowHandler	mDisplay;
		X11Window		mWindow;
		LockFunc		mLock;
		UnlockFunc		mUnlock;
};

}}}

#endif

#endif
