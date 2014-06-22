#ifndef EE_WINDOWCOSXIMPL_HPP
#define EE_WINDOWCOSXIMPL_HPP

#include <eepp/window/base.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX

#include <eepp/window/platformimpl.hpp>

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class cOSXImpl : public PlatformImpl {
	public:
		cOSXImpl( EE::Window::Window * window );

		~cOSXImpl();

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

		eeWindowContex GetWindowContext();
};

}}}

#endif

#endif
