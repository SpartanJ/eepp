#include <eepp/window/backend/SDL3/backendsdl3.hpp>

#ifdef EE_BACKEND_SDL3

#include <eepp/window/draganddrop/waylanddraganddrop.hpp>
#include <eepp/window/runtime.hpp>

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
	// A window-close callback owns the decision to close, hide, or cancel. Keep native Quit
	// independent rather than receiving a second quit event for the same close request.
	SDL_SetHintWithPriority( SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0", SDL_HINT_OVERRIDE );
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( Runtime::isOffscreen() ) {
		// EGL dispatchers can retain thread state across unload/reload, leaving stale TLS
		// pointers on the next Engine startup. Retain one library reference for the process,
		// as SDL already does for GLX. Windows, contexts and surfaces still shut down normally.
		[[maybe_unused]] static auto* const eglLibrary = [] {
			const char* libraryPath = SDL_GetHint( SDL_HINT_EGL_LIBRARY );
			return SDL_LoadObject( libraryPath ? libraryPath : "libEGL.so.1" );
		}();
	}
#endif
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
#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
	Private::WaylandDragAndDrop::shutdown();
#endif
#if EE_PLATFORM != EE_PLATFORM_MACOS
	// Audio belongs to MojoAL, independently of the Engine, and its last resource can be
	// released on a worker. Global SDL_Quit() ignores those references and races with
	// alcCloseDevice(), leaving SDL audio shutdown to use already-cleared driver callbacks.
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
#endif
}

}}}} // namespace EE::Window::Backend::SDL3

#endif
