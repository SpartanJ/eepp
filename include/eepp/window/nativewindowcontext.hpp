#ifndef EE_WINDOW_NATIVE_WINDOW_CONTEXT_HPP
#define EE_WINDOW_NATIVE_WINDOW_CONTEXT_HPP

#include <eepp/config.hpp>

namespace EE { namespace Window {

class Window;

/** Borrowed native handles. Never release a backend-owned connection/window/surface. */
struct NativeWindowContext {
	enum class Kind : Uint8 { Unsupported, X11, Wayland, Win32, Cocoa, Haiku };

	Window* owner{ nullptr };
	union Handles {
		struct {
			void* display;
			unsigned long window;
		} x11;
		struct {
			void* display;
			void* surface;
		} wayland;
		void* window;
		Handles() : x11{ nullptr, 0 } {}
	} handles;
	Kind kind{ Kind::Unsupported };
};

}} // namespace EE::Window

#endif
