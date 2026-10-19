#ifndef EE_WINDOW_X11_ERROR_SCOPE_HPP
#define EE_WINDOW_X11_ERROR_SCOPE_HPP

// Keep Xlib's timestamp typedef out of eepp headers, where Time names EE::System::Time.
#define Time X11NativeTime
#include <X11/Xlib.h>
#undef Time

namespace EE { namespace Window { namespace Private {

/** Foreign windows may disappear between querying them and sending a protocol message. Scope the
 * process-global handler to our own synchronous request batch, and preserve SDL's error handler. */
class XErrorScope {
  public:
	template <typename Functions>
	XErrorScope( const Functions& x11, Display* display ) :
		setErrorHandler( x11.XSetErrorHandler ), synchronize( x11.XSync ), display( display ) {
		// Older queued errors are forwarded by serial range; no leading round trip is needed.
		firstRequest = NextRequest( display );
		synchronizedRequest = firstRequest - 1;
		parent = active;
		previous = setErrorHandler( handleError );
		active = this;
	}

	~XErrorScope() {
		sync();
		active = parent;
		setErrorHandler( previous );
	}

	/** Confirm property writes before sending SelectionNotify, as required by ICCCM. */
	bool sync() {
		// Explicit validation can drain the batch before scope exit. Only synchronize again
		// if another request was issued, such as the subsequent SelectionNotify reply.
		if ( NextRequest( display ) != synchronizedRequest ) {
			synchronize( display, False );
			synchronizedRequest = NextRequest( display );
		}
		return !failed;
	}

  private:
	static int handleError( Display* display, XErrorEvent* event ) {
		for ( auto* scope = active; scope; scope = scope->parent ) {
			if ( display == scope->display && event->serial >= scope->firstRequest ) {
				scope->failed = true;
				return 0;
			}
			// Nested scopes share this handler. Forward unrelated connections/requests only
			// through the outermost original handler, avoiding recursion into ourselves.
			if ( !scope->parent && scope->previous )
				return scope->previous( display, event );
		}
		return 0;
	}

	decltype( &::XSetErrorHandler ) setErrorHandler;
	decltype( &::XSync ) synchronize;
	Display* display;
	XErrorHandler previous;
	XErrorScope* parent;
	unsigned long firstRequest;
	unsigned long synchronizedRequest{ 0 };
	bool failed{ false };
	static inline XErrorScope* active{ nullptr };
};

}}} // namespace EE::Window::Private

#endif
