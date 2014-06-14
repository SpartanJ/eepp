#ifndef EE_WINDOWCCURSORSDL2_HPP
#define EE_WINDOWCCURSORSDL2_HPP

#include <eepp/window/ccursor.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class CursorSDL : public Cursor {
	public:
		SDL_Cursor * GetCursor() const;
	protected:
		friend class CursorManagerSDL;

		SDL_Cursor * mCursor;

		CursorSDL( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		CursorSDL( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		CursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window );

		virtual ~CursorSDL();

		void Create();
};

}}}}

#endif

#endif
