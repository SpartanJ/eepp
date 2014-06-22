#ifndef EE_WINDOWCCURSORWIN_HPP
#define EE_WINDOWCCURSORWIN_HPP

#include <eepp/window/cursor.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN

using namespace EE::Window;

namespace EE { namespace Window { namespace Platform {

class cWinImpl;

class CursorWin : public Cursor {
	public:
		void *		GetCursor() const;
	protected:
		friend class cWinImpl;

		void *		mCursor;

		CursorWin( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorWin( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorWin( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		~CursorWin();

		void Create();

		cWinImpl * GetPlatform();
};

}}}

#endif

#endif
