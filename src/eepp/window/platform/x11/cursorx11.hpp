#ifndef EE_WINDOWCCURSORX11_HPP
#define EE_WINDOWCCURSORX11_HPP

#include <eepp/window/cursor.hpp>

#if defined( EE_X11_PLATFORM )

using namespace EE::Window;

namespace EE { namespace Window { namespace Platform {

class X11Impl;

class CursorX11 : public Cursor {
	public:
		X11Cursor	GetCursor() const;
	protected:
		friend class X11Impl;

		X11Cursor	mCursor;

		CursorX11( Texture * tex, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		CursorX11( Graphics::Image * img, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		CursorX11( const std::string& path, const Vector2i& hotspot, const std::string& getName, EE::Window::Window * window );

		~CursorX11();

		void create();

		X11Impl * getPlatform();
};

}}}

#endif

#endif
