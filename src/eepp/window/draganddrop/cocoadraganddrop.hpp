#ifndef EE_WINDOW_COCOA_DRAGANDDROP_HPP
#define EE_WINDOW_COCOA_DRAGANDDROP_HPP

#include "dragsource.hpp"
#include <eepp/window/nativewindowcontext.hpp>
#include <memory>

#if EE_PLATFORM == EE_PLATFORM_MACOS

namespace EE { namespace Window { namespace Private {

class CocoaDragAndDrop {
  public:
	CocoaDragAndDrop() = delete;

	static std::unique_ptr<DragSource> createSource( NativeWindowContext context,
													 const DragOffer& offer,
													 DragCompletionCallback completion,
													 DragStartResult& result );
};

}}} // namespace EE::Window::Private

#endif

#endif
