#ifndef EE_WINDOW_WIN32_DRAGANDDROP_HPP
#define EE_WINDOW_WIN32_DRAGANDDROP_HPP

#include "dragsource.hpp"
#include <eepp/window/nativewindowcontext.hpp>
#include <memory>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace Window { namespace Private {

class Win32DragAndDrop {
  public:
	Win32DragAndDrop() = delete;

	/** Register an OLE file receiver for an SDL2 HWND. Existing OLE targets are preserved. */
	static bool attachWindow( void* window );

	static void detachWindow( void* window );

	static std::unique_ptr<DragSource> createSource( NativeWindowContext context,
													 const DragOffer& offer,
													 DragCompletionCallback completion,
													 DragStartResult& result );
};

}}} // namespace EE::Window::Private

#endif

#endif
