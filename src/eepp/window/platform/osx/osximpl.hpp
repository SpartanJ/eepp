#ifndef EE_WINDOWCOSXIMPL_HPP
#define EE_WINDOWCOSXIMPL_HPP

#include <eepp/window/base.hpp>

#if EE_PLATFORM == EE_PLATFORM_MACOSX

#include <eepp/window/platformimpl.hpp>

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

class OSXImpl : public PlatformImpl {
	public:
		OSXImpl( EE::Window::Window * window );

		~OSXImpl();

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

		void setSystemMouseCursor( EE_SYSTEM_CURSOR syscursor );

		void restoreCursor();

		eeWindowContex getWindowContext();
};

}}}

#endif

#endif
