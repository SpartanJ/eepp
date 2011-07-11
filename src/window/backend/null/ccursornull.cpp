#include "ccursornull.hpp"

namespace EE { namespace Window { namespace Backend { namespace Null {

cCursorNull::cCursorNull( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( tex, hotspot, name, window )
{
}

cCursorNull::cCursorNull( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( img, hotspot, name, window )
{
}

cCursorNull::cCursorNull( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( path, hotspot, name, window )
{
}

void cCursorNull::Create() {
}

}}}}
