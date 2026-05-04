#include <eepp/system/log.hpp>
#include <eepp/window/backend/SDL3/clipboardsdl3.hpp>
#include <eepp/window/backend/SDL3/windowsdl3.hpp>

#ifdef EE_BACKEND_SDL3

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

ClipboardSDL::ClipboardSDL( EE::Window::Window* window ) : Clipboard( window ) {}

ClipboardSDL::~ClipboardSDL() {}

void ClipboardSDL::init() {}

void ClipboardSDL::setText( const std::string& text ) {
	SDL_SetClipboardText( text.c_str() );
}

std::string ClipboardSDL::getText() {
	char* text = SDL_GetClipboardText();
	std::string str( text ? text : "" );
	SDL_free( text );
	return str;
}

bool ClipboardSDL::hasPrimarySelection() const {
	// SDL3 may not have primary selection support
	return false;
}

std::string ClipboardSDL::getPrimarySelectionText() {
	return getText();
}

void ClipboardSDL::setPrimarySelectionText( const std::string& text ) {
	// No-op
}

String ClipboardSDL::getWideText() {
	return String::fromUtf8( getText() );
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
