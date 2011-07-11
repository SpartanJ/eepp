#ifndef EE_WINDOWCCURSORSDL13_HPP
#define EE_WINDOWCCURSORSDL13_HPP

#include "../../ccursor.hpp"
#include "base.hpp"

#ifdef EE_BACKEND_SDL_1_3

using namespace EE::Window;

namespace EE { namespace Window { namespace Backend { namespace SDL13 {

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
