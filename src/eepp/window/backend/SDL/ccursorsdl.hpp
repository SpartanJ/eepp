#ifndef EE_WINDOWCCURSORSDL_HPP
#define EE_WINDOWCCURSORSDL_HPP

#include <eepp/window/ccursor.hpp>
#include <eepp/window/backend/SDL/base.hpp>

#ifdef EE_BACKEND_SDL_1_2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL {

class cCursorSDL : public cCursor {
	protected:
		friend class cCursorManagerSDL;

		cCursorSDL( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorSDL( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		void Create();
};

}}}}

#endif

#endif
