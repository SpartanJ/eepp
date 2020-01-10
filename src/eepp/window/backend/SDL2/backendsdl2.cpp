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

WindowBackendSDL2::WindowBackendSDL2() : WindowBackend() {}

WindowBackendSDL2::~WindowBackendSDL2() {
#if EE_PLATFORM != EE_PLATFORM_MACOSX
	SDL_Quit();
#endif
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
