#include <eepp/window/backend/SDL2/backendsdl2.hpp>

#ifdef EE_BACKEND_SDL2

#include <cstdlib>
#include <eepp/window/backend/SDL2/clipboardsdl2.hpp>
#include <eepp/window/backend/backendhelper.hpp>
#include <eepp/window/draganddrop/waylanddraganddrop.hpp>
#include <eepp/window/runtime.hpp>

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

#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_HAS_WAYLAND_CLIENT ) && \
	SDL_VERSION_ATLEAST( 2, 0, 22 )
static bool preferWayland() {
	if ( SDL_GetHint( SDL_HINT_VIDEODRIVER ) || SDL_getenv( "SDL_VIDEO_DRIVER" ) ||
		 std::getenv( "WAYLAND_SOCKET" ) )
		return false;
	const char* x11Display = std::getenv( "DISPLAY" );
	const char* waylandDisplay = std::getenv( "WAYLAND_DISPLAY" );
	const char* session = std::getenv( "XDG_SESSION_TYPE" );
	if ( !x11Display || !*x11Display ||
		 ( ( !waylandDisplay || !*waylandDisplay ) &&
		   ( !session || SDL_strcasecmp( session, "wayland" ) != 0 ) ) )
		return false;

	// Follow SDL3's FIFO requirement for frame pacing when X11 is also available.
	// Require server-side decorations if this SDL2 build cannot load libdecor.
#ifdef HAVE_LIBDECOR_H
	if ( SDL_GetHintBoolean( SDL_HINT_VIDEO_WAYLAND_ALLOW_LIBDECOR, SDL_TRUE ) ) {
		void* libdecor = SDL_LoadObject( "libdecor-0.so.0" );
		if ( libdecor ) {
			SDL_UnloadObject( libdecor );
			return Private::WaylandLibrary::hasGlobals(
				{ "xdg_wm_base", "wl_output", "wp_fifo_manager_v1" } );
		}
	}
#endif
	return Private::WaylandLibrary::hasGlobals(
		{ "xdg_wm_base", "wl_output", "wp_fifo_manager_v1", "zxdg_decoration_manager_v1" } );
}
#endif

bool WindowBackendSDL2::initVideo() {
	if ( SDL_WasInit( SDL_INIT_VIDEO ) )
		return true;

#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( Runtime::mode() == RuntimeMode::Native ) {
		constexpr char waylandClass[] = "SDL_VIDEO_WAYLAND_WMCLASS";
#if defined( EE_HAS_WAYLAND_CLIENT ) && SDL_VERSION_ATLEAST( 2, 0, 22 )
		const bool prefer =
			preferWayland() &&
			SDL_SetHintWithPriority( SDL_HINT_VIDEODRIVER, "wayland,x11", SDL_HINT_DEFAULT );
#endif
		bool restoreClass = false;
		if ( !std::getenv( waylandClass ) && !std::getenv( "SDL_VIDEO_X11_WMCLASS" ) &&
			 !SDL_GetHint( "SDL_APP_ID" ) ) {
			const auto appId = BackendHelper::packagedAppId();
			if ( !appId.empty() ) {
				// sdl2-compat maps the class hint to SDL3's process-local app ID. Native
				// SDL2 reads the process environment during video initialization.
				SDL_SetHintWithPriority( waylandClass, appId.c_str(), SDL_HINT_DEFAULT );
				if ( !SDL_GetHint( "SDL_APP_ID" ) ) {
					SDL_SetHintWithPriority( waylandClass, nullptr, SDL_HINT_DEFAULT );
					restoreClass = setenv( waylandClass, appId.c_str(), 0 ) == 0;
				}
			}
		}
		bool initialized = SDL_Init( SDL_INIT_VIDEO ) == 0;
#if defined( EE_HAS_WAYLAND_CLIENT ) && SDL_VERSION_ATLEAST( 2, 0, 22 )
		if ( prefer ) {
			SDL_SetHintWithPriority( SDL_HINT_VIDEODRIVER, nullptr, SDL_HINT_DEFAULT );
			// SDL's list only falls back during driver creation. It can still fail later in
			// VideoInit.
			if ( !initialized )
				initialized = SDL_Init( SDL_INIT_VIDEO ) == 0;
		}
#endif
		if ( restoreClass )
			unsetenv( waylandClass );
		return initialized;
	}
#endif
	return SDL_Init( SDL_INIT_VIDEO ) == 0;
}

WindowBackendSDL2::WindowBackendSDL2() : WindowBackendLibrary() {
	// eepp handles window-close requests separately from explicit application Quit. In
	// particular, a close callback may retain the window or cancel to protect unsaved work.
	SDL_SetHintWithPriority( "SDL_QUIT_ON_LAST_WINDOW_CLOSE", "0", SDL_HINT_OVERRIDE );
#if EE_PLATFORM == EE_PLATFORM_LINUX
	if ( Runtime::isOffscreen() ) {
		// EGL dispatchers can retain thread state across unload/reload, leaving stale TLS
		// pointers on the next Engine startup. Retain one library reference for the process,
		// as SDL already does for GLX. Windows, contexts and surfaces still shut down normally.
		[[maybe_unused]] static auto* const eglLibrary = [] {
			const char* libraryPath = SDL_getenv( "SDL_VIDEO_EGL_DRIVER" );
			return SDL_LoadObject( libraryPath ? libraryPath : "libEGL.so.1" );
		}();
	}
#endif
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
	ClipboardSDL::sPrimarySelectionSupport = ClipboardSDL::PrimarySelectionSupport::Unknown;
#ifdef EE_HAS_WAYLAND_DRAG_SOURCE
	Private::WaylandDragAndDrop::shutdown();
#endif
#if EE_PLATFORM != EE_PLATFORM_MACOS
	// Audio belongs to MojoAL, independently of the Engine, and its last resource can be
	// released on a worker. Global SDL_Quit() ignores those references and races with
	// alcCloseDevice() (as with SDL3, and with sdl2-compat running on SDL3).
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
#endif
}

}}}} // namespace EE::Window::Backend::SDL2

#endif
