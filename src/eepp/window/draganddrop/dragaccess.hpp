#ifndef EE_WINDOW_NATIVE_DRAG_ACCESS_HPP
#define EE_WINDOW_NATIVE_DRAG_ACCESS_HPP

#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Window { namespace Private {

/** Only native drag implementation code may operate the protected backend plumbing. */
struct NativeDragAccess {
	static NativeWindowContext getNativeDragContext( Window& window ) {
		return window.getNativeDragContext();
	}

	static bool isNativeFileTargetSupported( const Window& window ) {
		return window.isNativeFileTargetSupported();
	}

	static bool isNativeDataTargetSupported( const Window& window ) {
		return window.isNativeDataTargetSupported();
	}

	static void initializeNativeDragEvents( Window& window ) {
		window.initializeNativeDragEvents();
	}

	static void shutdownNativeDragEvents( Window& window ) { window.shutdownNativeDragEvents(); }

	static NativeDragPreview* createNativeDragPreview( Window& window, const DragOffer& offer,
													   bool manageCursor ) {
		return window.createNativeDragPreview( offer, manageCursor );
	}

	static bool acquireNativeDragCapture( Input& input ) {
		return input.acquireNativeDragCapture();
	}

	static void restoreNativeDragCapture( Input& input ) { input.restoreNativeDragCapture(); }

	static void resetNativeDragInput( Input& input ) { input.resetNativeDragInput(); }

	static void suppressNativeDragRelease( Input& input ) { input.suppressNativeDragRelease(); }

	static bool consumeNativeDragRelease( Input& input ) {
		return input.consumeNativeDragRelease();
	}
};

}}} // namespace EE::Window::Private

#endif
