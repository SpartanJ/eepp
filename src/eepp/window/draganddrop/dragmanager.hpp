#ifndef EE_WINDOW_NATIVE_DRAG_MANAGER_HPP
#define EE_WINDOW_NATIVE_DRAG_MANAGER_HPP

#include "dragsource.hpp"
#include <eepp/window/nativewindowcontext.hpp>
#include <memory>
#include <string>

namespace EE { namespace Window { namespace Private {

/** Owns one process-wide session, independently of its source window. Input polling delivers
 * completion after native cleanup, even when the window was destroyed during the gesture. */
class DragManager {
  public:
	DragManager() = delete;

	static bool hasSession();

	static bool isFileDragSupported( const NativeWindowContext& context );

	static DragStartResult startFileDrag( NativeWindowContext context, const DragOffer& offer,
										  std::string payload, DragCompletionCallback completion );

	static void cancelDrag( Window& window );

	static bool isDragging( const Window& window );

	static bool handleEvent( void* event );

	static void handleInput( Uint32 windowId, NativeDragInput input );

	static void poll();

	static void invalidateWindow( Window* window );

  private:
	static std::unique_ptr<DragSource> sSource;
	static bool sPolling;
};

}}} // namespace EE::Window::Private

#endif
