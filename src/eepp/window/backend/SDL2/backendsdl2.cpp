#include <eepp/window/backend/SDL2/backendsdl2.hpp>

#ifdef EE_BACKEND_SDL2

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS && !defined( EE_COMPILER_MSVC ) && !defined( EE_SDL2_FROM_ROOTPATH )
	#include <SDL2/SDL.h>
#else
	#include <SDL.h>
#endif

namespace EE { namespace Window { namespace Backend { namespace SDL2 {

WindowBackendSDL2::WindowBackendSDL2() : 
	WindowBackend()
{
}

WindowBackendSDL2::~WindowBackendSDL2()
{
	SDL_Quit();
}

}}}}

#endif
