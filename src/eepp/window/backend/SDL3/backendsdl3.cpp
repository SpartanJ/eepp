#include <eepp/window/backend/SDL3/backendsdl3.hpp>

#ifdef EE_BACKEND_SDL3

#if ( EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS ) && defined( main )
#undef main
#endif

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS &&  \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && !defined( EE_COMPILER_MSVC ) && \
	!defined( EE_SDL3_FROM_ROOTPATH )
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL3 {

WindowBackendSDL3::WindowBackendSDL3() : WindowBackendLibrary() {
	if ( !SDL_GetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS ) )
		SDL_SetHint( SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0" );
	// The following hints are not available in SDL3 (or renamed)
	// if ( !SDL_GetHint( SDL_HINT_IME_SHOW_UI ) )
	//     SDL_SetHint( SDL_HINT_IME_SHOW_UI, "1" );
	// if ( !SDL_GetHint( SDL_HINT_IME_SUPPORT_EXTENDED_TEXT ) )
	//     SDL_SetHint( SDL_HINT_IME_SUPPORT_EXTENDED_TEXT, "1" );
	if ( !SDL_GetHint( SDL_HINT_FORCE_RAISEWINDOW ) )
		SDL_SetHint( SDL_HINT_FORCE_RAISEWINDOW, "1" );
}

WindowBackendSDL3::~WindowBackendSDL3() {
#if EE_PLATFORM != EE_PLATFORM_MACOS
	SDL_Quit();
#endif
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
