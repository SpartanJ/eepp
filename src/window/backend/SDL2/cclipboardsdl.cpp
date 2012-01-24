#include "cclipboardsdl.hpp"
#include "cwindowsdl.hpp"

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

cClipboardSDL::cClipboardSDL( cWindow * window ) :
	cClipboard( window )
{
}

cClipboardSDL::~cClipboardSDL() {
}

void cClipboardSDL::Init() {
}

void cClipboardSDL::SetText( const std::string& Text ) {
	SDL_SetClipboardText( Text.c_str() );
}

std::string cClipboardSDL::GetText() {
	char * text = SDL_GetClipboardText();
	std::string str( text );
	SDL_free(text);
	return str;
}

String cClipboardSDL::GetWideText() {
	char * text = SDL_GetClipboardText();
	String str( String::FromUtf8( text ) );
	SDL_free(text);
	return str;
}

}}}}

#endif
