#include <eepp/window/backend/SFML/ccursorsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace SFML {

CursorSFML::CursorSFML( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	Cursor( tex, hotspot, name, window )
{
}

CursorSFML::CursorSFML( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	Cursor( img, hotspot, name, window )
{
}

CursorSFML::CursorSFML( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	Cursor( path, hotspot, name, window )
{
}

void CursorSFML::Create() {
}

}}}}

#endif
