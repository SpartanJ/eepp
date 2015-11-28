#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

ClipboardSDL::ClipboardSDL( EE::Window::Window * window ) :
	Clipboard( window )
{
}

ClipboardSDL::~ClipboardSDL() {
}

void ClipboardSDL::Init() {
}

void ClipboardSDL::SetText( const std::string& Text ) {
	SDL_SetClipboardText( Text.c_str() );
}

std::string ClipboardSDL::GetText() {
	char * text = SDL_GetClipboardText();
	std::string str( text );
	SDL_free(text);
	return str;
}

String ClipboardSDL::GetWideText() {
	char * text = SDL_GetClipboardText();
	String str( String::FromUtf8( text ) );
	SDL_free(text);
	return str;
}

}}}}

#endif
