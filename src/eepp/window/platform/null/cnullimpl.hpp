#ifndef EE_WINDOWcNullImpl_HPP
#define EE_WINDOWcNullImpl_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/cplatformimpl.hpp>

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cNullImpl : public cPlatformImpl {
	public:
		cNullImpl( Window::cWindow * window );

		~cNullImpl();

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

		eeWindowContex GetWindowContext();
};

}}}

#endif
