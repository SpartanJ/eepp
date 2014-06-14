#include <eepp/window/backend/SDL/ccursorsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

namespace EE { namespace Window { namespace Backend { namespace SDL {

cCursorSDL::cCursorSDL( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( tex, hotspot, name, window )
{
}

cCursorSDL::cCursorSDL( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( img, hotspot, name, window )
{
}

cCursorSDL::cCursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( path, hotspot, name, window )
{
}

void cCursorSDL::Create() {
}

}}}}

#endif
