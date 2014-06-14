#ifndef EE_WINDOWcNullImpl_HPP
#define EE_WINDOWcNullImpl_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/cplatformimpl.hpp>

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cNullImpl : public PlatformImpl {
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

		Vector2i Position();

		void ShowMouseCursor();

		void HideMouseCursor();

		Cursor * CreateMouseCursor( cTexture * tex, const Vector2i& hotspot, const std::string& name );

		Cursor * CreateMouseCursor( cImage * img, const Vector2i& hotspot, const std::string& name );

		Cursor * CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name );

		void SetMouseCursor( Cursor * cursor );

		void SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor );

		void RestoreCursor();

		eeWindowContex GetWindowContext();
};

}}}

#endif
