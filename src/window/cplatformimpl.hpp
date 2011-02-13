#ifndef EE_WINDOWCPLATFORMIMPL_HPP
#define EE_WINDOWCPLATFORMIMPL_HPP

#include "base.hpp"

namespace EE { namespace Window {
class cWindow;
}}

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cPlatformImpl {
	public:
		cPlatformImpl( cWindow * window );
		
		virtual ~cPlatformImpl();
		
		virtual void MinimizeWindow() = 0;

		virtual void MaximizeWindow() = 0;

		virtual void HideWindow() = 0;

		virtual void RaiseWindow() = 0;

		virtual void ShowWindow() = 0;

		virtual void MoveWindow( int left, int top ) = 0;
		
		virtual void SetContext( eeWindowContex Context ) = 0;

		virtual eeVector2i Position() = 0;
	protected:
		cWindow *	mWindow;
};

}}}

#endif
