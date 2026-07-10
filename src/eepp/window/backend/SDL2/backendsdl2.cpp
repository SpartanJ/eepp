#include <eepp/window/backend/SDL2/backendsdl2.hpp>

#ifdef EE_BACKEND_SDL2

#if ( EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS ) && defined( main )
#undef main
#endif

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS &&  \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && !defined( EE_COMPILER_MSVC ) && \
	!defined( EE_SDL2_FROM_ROOTPATH )
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

WindowBackendSDL2::WindowBackendSDL2() : WindowBackendLibrary() {
	if ( !SDL_GetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS ) )
		SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );
	if ( !SDL_GetHint( SDL_HINT_RETURN_KEY_HIDES_IME ) )
		SDL_SetHint( SDL_HINT_RETURN_KEY_HIDES_IME, "1" );
#if SDL_VERSION_ATLEAST( 2, 0, 18 )
	if ( !SDL_GetHint( SDL_HINT_IME_SHOW_UI ) )
		SDL_SetHint( SDL_HINT_IME_SHOW_UI, "1" );
#endif
#if SDL_VERSION_ATLEAST( 2, 0, 22 )
	if ( !SDL_GetHint( SDL_HINT_IME_SUPPORT_EXTENDED_TEXT ) )
		SDL_SetHint( SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1" );
	if ( !SDL_GetHint( SDL_HINT_FORCE_RAISEWINDOW ) )
		SDL_SetHint( SDL_HINT_FORCE_RAISEWINDOW, "1" );
#endif
}

WindowBackendSDL2::~WindowBackendSDL2() {
#if EE_PLATFORM != EE_PLATFORM_MACOS
	SDL_Quit();
#endif
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
