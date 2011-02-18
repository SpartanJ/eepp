#include "ccursorsdl.hpp"

#ifdef EE_BACKEND_SDL_ACTIVE

namespace EE { namespace Window { namespace Backend { namespace SDL {

cCursorSDL::cCursorSDL( cTexture * tex, const eeVector2i& hotspot, const std::string& name, cWindow * window ) :
	cCursor( tex, hotspot, name, window )
{
}

cCursorSDL::cCursorSDL( cImage * img, const eeVector2i& hotspot, const std::string& name, cWindow * window ) :
	cCursor( img, hotspot, name, window )
{
}

cCursorSDL::cCursorSDL( const std::string& path, const eeVector2i& hotspot, const std::string& name, cWindow * window ) :
	cCursor( path, hotspot, name, window )
{
}

void cCursorSDL::Create() {
}

}}}}

#endif
