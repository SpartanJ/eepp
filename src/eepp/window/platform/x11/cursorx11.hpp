#ifndef EE_WINDOWCCURSORX11_HPP
#define EE_WINDOWCCURSORX11_HPP

#include <eepp/window/cursor.hpp>

#if defined( EE_X11_PLATFORM )

using namespace EE::Window;

namespace EE { namespace Window { namespace Platform {

class cX11Impl;

class CursorX11 : public Cursor {
	public:
		X11Cursor	GetCursor() const;
	protected:
		friend class cX11Impl;

		X11Cursor	mCursor;

		CursorX11( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorX11( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorX11( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		~CursorX11();

		void Create();

		cX11Impl * GetPlatform();
};

}}}

#endif

#endif
