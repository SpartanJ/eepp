#ifndef EE_WINDOW_X11_DROP_TARGET_HPP
#define EE_WINDOW_X11_DROP_TARGET_HPP

#include "droptarget.hpp"
#include "x11draganddrop.hpp"
#include <memory>

#ifdef EE_HAS_X11_DRAG_SOURCE

namespace EE { namespace Window { namespace Private {

/** XDND receiver for drags that SDL cannot read: it takes over a drag entering a registered
 * window only when the drag offers a registered type and no text/uri-list. */
class X11DropTarget {
  public:
	X11DropTarget() = delete;

	/** nullptr when the X11 client library is unavailable. */
	static std::unique_ptr<DropTarget> create();
};

}}} // namespace EE::Window::Private

#endif

#endif
