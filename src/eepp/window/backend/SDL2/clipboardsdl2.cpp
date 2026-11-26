#include <eepp/system/log.hpp>
#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/SDL2/windowsdl2.hpp>

#include <cstring>

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <emscripten-browser-clipboard/emscripten_browser_clipboard.h>
#endif

#ifdef EE_BACKEND_SDL2

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

ClipboardSDL::PrimarySelectionSupport ClipboardSDL::sPrimarySelectionSupport =
	PrimarySelectionSupport::Unknown;

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
static std::string sContent;
#endif

ClipboardSDL::ClipboardSDL( EE::Window::Window* window ) : Clipboard( window ) {}

ClipboardSDL::~ClipboardSDL() {}

void ClipboardSDL::init() {
#if SDL_VERSION_ATLEAST( 2, 26, 0 )
	if ( sPrimarySelectionSupport == PrimarySelectionSupport::Unknown ) {
		const char* driver =
			nativePrimarySelectionEnabled() ? SDL_GetCurrentVideoDriver() : nullptr;
		sPrimarySelectionSupport =
			driver && ( std::strcmp( driver, "x11" ) == 0 || std::strcmp( driver, "wayland" ) == 0 )
				? PrimarySelectionSupport::Supported
				: PrimarySelectionSupport::Unsupported;
	}
#endif
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
#elif EE_PLATFORM == EE_PLATFORM_WIN
	// SDL2's Windows backend retries reads when another process owns the clipboard, but not
	// writes. Clipboard ownership is transient, so mirror the read-side retry policy here.
	static constexpr int MAX_ATTEMPTS = 3;
	for ( int attempt = 0; attempt < MAX_ATTEMPTS; ++attempt ) {
		if ( SDL_SetClipboardText( text.c_str() ) == 0 )
			return;
		if ( attempt + 1 < MAX_ATTEMPTS )
			SDL_Delay( 10 );
	}
	Log::warning( "Failed to set clipboard text: %s", SDL_GetError() );
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
#if SDL_VERSION_ATLEAST( 2, 26, 0 )
	if ( supportsPrimarySelection() )
		return SDL_HasPrimarySelectionText();
#endif
	return Clipboard::hasPrimarySelection();
}

bool ClipboardSDL::supportsPrimarySelection() const {
#if SDL_VERSION_ATLEAST( 2, 26, 0 )
	return sPrimarySelectionSupport == PrimarySelectionSupport::Supported;
#else
	return false;
#endif
}

std::string ClipboardSDL::getPrimarySelectionText() {
#if SDL_VERSION_ATLEAST( 2, 26, 0 )
	if ( supportsPrimarySelection() ) {
		char* text = SDL_GetPrimarySelectionText();
		std::string str( text ? text : "" );
		SDL_free( text );
		return str;
	}
#endif
	return Clipboard::getPrimarySelectionText();
}

void ClipboardSDL::setPrimarySelectionText( const std::string& text ) {
#if SDL_VERSION_ATLEAST( 2, 26, 0 )
	if ( supportsPrimarySelection() ) {
		SDL_SetPrimarySelectionText( text.c_str() );
		return;
	}
#endif
	Clipboard::setPrimarySelectionText( text );
}

String ClipboardSDL::getWideText() {
	return String::fromUtf8( getText() );
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
