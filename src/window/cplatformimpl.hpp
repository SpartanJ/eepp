#ifndef EE_WINDOWCPLATFORMIMPL_HPP
#define EE_WINDOWCPLATFORMIMPL_HPP

#include "base.hpp"
#include "cursorhelper.hpp"

namespace EE {

namespace Graphics {
class cTexture;
class cImage;
}

namespace Window {
class cWindow;
class cCursor;
}

}

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cPlatformImpl {
	public:
		cPlatformImpl( Window::cWindow * window );
		
		virtual ~cPlatformImpl();
		
		virtual void MinimizeWindow() = 0;

		virtual void MaximizeWindow() = 0;

		virtual void HideWindow() = 0;

		virtual void RaiseWindow() = 0;

		virtual void ShowWindow() = 0;

		virtual void MoveWindow( int left, int top ) = 0;
		
		virtual void SetContext( eeWindowContex Context ) = 0;

		virtual eeVector2i Position() = 0;

		virtual void ShowMouseCursor() = 0;

		virtual void HideMouseCursor() = 0;

		virtual cCursor * CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) = 0;

		virtual cCursor * CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name ) = 0;

		virtual cCursor * CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name ) = 0;

		virtual void SetMouseCursor( cCursor * cursor ) = 0;

		virtual void SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor ) = 0;

		virtual void RestoreCursor() = 0;
	protected:
		cWindow *	mWindow;
};

}}}

#endif
