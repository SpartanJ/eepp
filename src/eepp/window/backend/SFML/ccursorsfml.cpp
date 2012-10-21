#include <eepp/window/backend/SFML/ccursorsfml.hpp>

#ifdef EE_BACKEND_SFML_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace SFML {

cCursorSFML::cCursorSFML( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( tex, hotspot, name, window )
{
}

cCursorSFML::cCursorSFML( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( img, hotspot, name, window )
{
}

cCursorSFML::cCursorSFML( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( path, hotspot, name, window )
{
}

void cCursorSFML::Create() {
}

}}}}

#endif
