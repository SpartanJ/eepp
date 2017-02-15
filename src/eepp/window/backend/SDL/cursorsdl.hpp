#ifndef EE_WINDOWCCURSORSDL_HPP
#define EE_WINDOWCCURSORSDL_HPP

#include <eepp/window/cursor.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL {

class CursorSDL : public Cursor {
	protected:
		friend class CursorManagerSDL;

		CursorSDL( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorSDL( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		CursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window );

		void create();
};

}}}}

#endif

#endif
