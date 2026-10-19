#ifndef EE_WINDOW_NATIVE_DRAG_MANAGER_HPP
#define EE_WINDOW_NATIVE_DRAG_MANAGER_HPP

#include "dragsource.hpp"
#include "droptarget.hpp"
#include <eepp/window/nativewindowcontext.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace EE { namespace Window { namespace Private {

/** Owns one process-wide session, independently of its source window. Input polling delivers
 * completion after native cleanup, even when the window was destroyed during the gesture.
 * Also owns the data drop target, which receives drags from other applications. */
class DragManager {
  public:
	DragManager() = delete;

	static bool hasSession();

	static bool isFileDragSupported( const NativeWindowContext& context );

	static DragStartResult startFileDrag( NativeWindowContext context, const DragOffer& offer,
										  std::string payload, DragCompletionCallback completion );

	static void cancelDrag( Window& window );

	static bool isDragging( const Window& window );

	/** An empty callback removes the window's handler. */
	static bool setDataDropHandler( NativeWindowContext context, std::vector<std::string> mimeTypes,
									DataDropCallback callback );

	/** An empty callback removes the window's handler. */
	static bool setFileSaveDropHandler( NativeWindowContext context,
										FileSaveDropCallback callback );

	static bool handleEvent( void* event );

	static void handleInput( Uint32 windowId, NativeDragInput input );

	static void poll();

	static void invalidateWindow( Window* window );

	/** Release the session without delivering its completion during backend/process teardown. */
	static void shutdown();

  private:
	/** Whether a source session or the data drop target still needs native protocol events. */
	static bool needsNativeEvents();

	static void pollSource();

	/** Applies update to the drop target (created for adding a handler), and enables native
	 * events while the target has handlers. */
	static bool updateTarget( const NativeWindowContext& context, bool adding,
							  const std::function<bool( DropTarget& )>& update );

	/** Releases the target once it has no handlers, unless its tick() is running. */
	static void releaseEmptyTarget( Window& window );

	static std::unique_ptr<DragSource> sSource;
	static std::unique_ptr<DropTarget> sTarget;
	static bool sPolling;
	static bool sTickingTarget;
};

}}} // namespace EE::Window::Private

#endif
