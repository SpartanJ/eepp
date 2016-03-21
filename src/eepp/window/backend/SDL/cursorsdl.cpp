#include <eepp/window/backend/SDL/cursorsdl.hpp>

#ifdef EE_BACKEND_SDL_1_2

namespace EE { namespace Window { namespace Backend { namespace SDL {

CursorSDL::CursorSDL( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( tex, hotspot, name, window )
{
}

CursorSDL::CursorSDL( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( img, hotspot, name, window )
{
}

CursorSDL::CursorSDL( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( path, hotspot, name, window )
{
}

void CursorSDL::Create() {
}

}}}}

#endif
