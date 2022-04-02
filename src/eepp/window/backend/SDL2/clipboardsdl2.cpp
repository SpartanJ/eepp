#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

ClipboardSDL::ClipboardSDL( EE::Window::Window* window ) : Clipboard( window ) {}

ClipboardSDL::~ClipboardSDL() {}

void ClipboardSDL::init() {}

void ClipboardSDL::setText( const std::string& text ) {
	SDL_SetClipboardText( text.c_str() );
}

std::string ClipboardSDL::getText() {
	char* text = SDL_GetClipboardText();
	std::string str( text );
	SDL_free( text );
	return str;
}

String ClipboardSDL::getWideText() {
	char* text = SDL_GetClipboardText();
	String str( String::fromUtf8( text ) );
	SDL_free( text );
	return str;
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
