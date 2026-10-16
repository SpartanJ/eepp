#ifndef EE_WINDOW_NATIVE_DROP_TARGET_HPP
#define EE_WINDOW_NATIVE_DROP_TARGET_HPP

#include <eepp/window/draganddrop.hpp>
#include <eepp/window/nativewindowcontext.hpp>
#include <string>
#include <vector>

namespace EE { namespace Window { namespace Private {

/** Receives drags from other applications for the windows registered with it, owned by
 * DragManager. Native events only record progress; the application's callbacks run from tick(),
 * outside native hooks, and may replace or remove any handler (DragManager keeps the target alive
 * while tick() runs). */
class DropTarget {
  public:
	virtual ~DropTarget() = default;

	/** Registers, replaces or (with an empty callback) removes context.owner's data handler. */
	virtual bool setDataHandler( const NativeWindowContext& context,
								 std::vector<std::string> mimeTypes,
								 DataDropCallback callback ) = 0;

	/** Registers, replaces or (with an empty callback) removes context.owner's file save
	 * handler. */
	virtual bool setFileSaveHandler( const NativeWindowContext& context,
									 FileSaveDropCallback callback ) = 0;

	virtual void removeWindow( Window* window ) = 0;

	virtual bool empty() const = 0;

	/** Consumes the protocol events of a drag this target accepted. */
	virtual bool handleEvent( void* event ) = 0;

	/** Runs the callbacks of dropped drags and abandons drops whose source stopped answering. */
	virtual void tick() = 0;
};

}}} // namespace EE::Window::Private

#endif
