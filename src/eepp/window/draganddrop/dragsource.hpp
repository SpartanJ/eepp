#ifndef EE_WINDOW_NATIVE_DRAG_SOURCE_HPP
#define EE_WINDOW_NATIVE_DRAG_SOURCE_HPP

#include <eepp/window/draganddrop.hpp>
#include <string>
#include <utility>

namespace EE { namespace Window { namespace Private {

enum class NativeDragInput : Uint8 { Motion, Release, Escape };

/** Native transport and one-shot completion state, owned by DragManager. */
class DragSource {
  public:
	DragSource( Window* owner, DragCompletionCallback completion ) :
		owner( owner ), completion( std::move( completion ) ) {}

	virtual ~DragSource() = default;

	virtual void tick() = 0;

	virtual bool handleEvent( void* event ) = 0;

	virtual void cancel() = 0;

	virtual void handleInput( NativeDragInput ) {}

	bool finish( DragEndReason reason, DragAction action = DragAction::None,
				 std::string error = {} ) {
		if ( completed )
			return false;
		result = { std::move( error ), reason,
				   reason == DragEndReason::Dropped ? action : DragAction::None };
		completed = true;
		return true;
	}

	Window* owner;
	DragCompletionCallback completion;
	DragResult result;
	bool completed{ false };
};

}}} // namespace EE::Window::Private

#endif
