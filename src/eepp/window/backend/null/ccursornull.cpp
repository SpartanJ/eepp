#include <eepp/window/backend/null/ccursornull.hpp>

namespace EE { namespace Window { namespace Backend { namespace Null {

CursorNull::CursorNull( cTexture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( tex, hotspot, name, window )
{
}

CursorNull::CursorNull( cImage * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( img, hotspot, name, window )
{
}

CursorNull::CursorNull( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( path, hotspot, name, window )
{
}

void CursorNull::Create() {
}

}}}}
