#include <eepp/system/log.hpp>
#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten-browser-clipboard/emscripten_browser_clipboard.h>
#endif

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
static std::string sContent;
#endif

ClipboardSDL::ClipboardSDL( EE::Window::Window* window ) : Clipboard( window ) {}

ClipboardSDL::~ClipboardSDL() {}

void ClipboardSDL::init() {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	Log::info( "Initialized emscripten clipboard" );
	emscripten_browser_clipboard::paste(
		[]( std::string const& paste_data, void* callback_data [[maybe_unused]] ) {
			Log::info( "Browser pasted: %s", paste_data.c_str() );
			sContent = std::move( paste_data );
		} );
#endif
}

void ClipboardSDL::setText( const std::string& text ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	sContent = text;
	emscripten_browser_clipboard::copy( text );
#else
	SDL_SetClipboardText( text.c_str() );
#endif
}

std::string ClipboardSDL::getText() {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return sContent;
#else
	char* text = SDL_GetClipboardText();
	std::string str( text );
	SDL_free( text );
	return str;
#endif
}

bool ClipboardSDL::hasPrimarySelection() const {
	return SDL_HasPrimarySelectionText();
}

std::string ClipboardSDL::getPrimarySelectionText() {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && SDL_VERSION_ATLEAST( 2, 26, 0 )
	if ( SDL_HasPrimarySelectionText() ) {
		char* text = SDL_GetPrimarySelectionText();
		std::string str( text );
		SDL_free( text );
		return str;
	}
#endif

	return getText();
}

void ClipboardSDL::setPrimarySelectionText( const std::string& text ) {
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && SDL_VERSION_ATLEAST( 2, 26, 0 )
	if ( SDL_HasPrimarySelectionText() ) {
		SDL_SetPrimarySelectionText( text.c_str() );
	}
#endif
}

String ClipboardSDL::getWideText() {
	return String::fromUtf8( getText() );
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
