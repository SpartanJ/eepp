#ifndef EE_WINDOW_WAYLAND_DRAGANDDROP_HPP
#define EE_WINDOW_WAYLAND_DRAGANDDROP_HPP

#include "dragsource.hpp"
#include "waylandlibrary.hpp"
#include <eepp/window/nativewindowcontext.hpp>
#include <memory>

#ifdef EE_HAS_WAYLAND_DRAG_SOURCE

namespace EE { namespace Window { namespace Private {

struct WaylandConnection;

class WaylandDragAndDrop {
  public:
	WaylandDragAndDrop() = delete;

	/** Observe the seat before its first press. Each successful attach needs one detach, before
	 * the backend destroys its window. Neither operation takes ownership of SDL's handles. */
	static void attachWindow( const NativeWindowContext& context );

	static void detachWindow( const Window& window );

	/** Release connection-wide observers before the backend quits SDL. */
	static void shutdown();

	static bool isSupported( const NativeWindowContext& context );

	static std::unique_ptr<DragSource> createSource( NativeWindowContext context,
													 const DragOffer& offer, std::string payload,
													 DragCompletionCallback completion,
													 DragStartResult& result );

  private:
	static WaylandConnection sConnection;
	static bool sExitHandlerRegistered;
};

}}} // namespace EE::Window::Private
#endif
#endif
