#ifndef EE_WINDOWCCURSORSDL2_HPP
#define EE_WINDOWCCURSORSDL2_HPP

#include <eepp/window/ccursor.hpp>
#include <eepp/window/backend/SDL2/base.hpp>

#ifdef EE_BACKEND_SDL2

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

class cCursorSDL : public cCursor {
	public:
		SDL_Cursor * GetCursor() const;
	protected:
		friend class cCursorManagerSDL;

		SDL_Cursor * mCursor;

		cCursorSDL( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorSDL( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		cCursorSDL( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window );

		virtual ~cCursorSDL();

		void Create();
};

}}}}

#endif

#endif
