#ifndef EE_WINDOW_X11_DRAGANDDROP_HPP
#define EE_WINDOW_X11_DRAGANDDROP_HPP

#include "dragsource.hpp"
#include <eepp/window/nativewindowcontext.hpp>
#include <memory>
#include <string>

// X11 headers are optional compile-time declarations. The client library is loaded lazily;
// neither the application nor the shared library needs to link against X11.
#if ( EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD ) && \
	__has_include( <X11/Xlib.h> ) && __has_include( <X11/Xatom.h> ) && \
	__has_include( <X11/keysym.h> )
#define EE_HAS_X11_DRAG_SOURCE

namespace EE { namespace Window { namespace Private {

class X11DragAndDrop {
  public:
	X11DragAndDrop() = delete;

	static bool isSupported();

	static std::unique_ptr<DragSource> createSource( NativeWindowContext context,
													 const DragOffer& offer, std::string payload,
													 DragCompletionCallback completion,
													 DragStartResult& result );
};

}}} // namespace EE::Window::Private

#endif

#endif
