#ifndef EE_WINDOWCCURSORWIN_HPP
#define EE_WINDOWCCURSORWIN_HPP

#include <eepp/window/cursor.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

using namespace EE::Window;

namespace EE { namespace Window { namespace Platform {

class WinImpl;

class CursorWin : public Cursor {
	public:
		void *		GetCursor() const;
	protected:
		friend class WinImpl;

		void *		mCursor;

		CursorWin( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorWin( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorWin( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		~CursorWin();

		void Create();

		WinImpl * GetPlatform();
};

}}}

#endif

#endif
