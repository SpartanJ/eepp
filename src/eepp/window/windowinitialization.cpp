#include "windowinitialization.hpp"

#include <eepp/system/log.hpp>
#include <eepp/window/runtime.hpp>
#include <eepp/window/window.hpp>

using namespace EE::System;

namespace EE::Window {

WindowInitialization::PendingWindow WindowInitialization::sPending;

void WindowInitialization::begin( EE::Window::Window* window, MonotonicClock::TimePoint started ) {
	const Time elapsed = MonotonicClock::now() - started;
	sPending = { window, started, elapsed, {}, {}, {} };
}

void WindowInitialization::baseUIReady( EE::Window::Window* window ) {
	if ( sPending.window == window )
		sPending.baseUI = MonotonicClock::now() - sPending.started;
}

void WindowInitialization::beginUpdate( EE::Window::Window* window ) {
	if ( sPending.window == window && !sPending.updateStarted ) {
		sPending.completeUI = MonotonicClock::now() - sPending.started;
		sPending.updateStarted = true;
	}
}

void WindowInitialization::endUpdate( EE::Window::Window* window ) {
	if ( sPending.window == window && !sPending.updateCompleted ) {
		sPending.firstUpdate = MonotonicClock::now() - sPending.started;
		sPending.updateCompleted = true;
	}
}

void WindowInitialization::presented( EE::Window::Window* window ) {
	if ( sPending.window != window )
		return;
	const auto now = MonotonicClock::now();
	const PendingWindow stats = sPending;
	cancel( window );
	// Emit only after presentation: formatting and stdout I/O must not enter these timings.
	Log::info( "UIApplication initialization: %s (runtime: %s; elapsed from construction)\n"
			   "\tWindow creation took: %.2f ms\n"
			   "\tBase UI took: %.2f ms\n"
			   "\tComplete UI took: %.2f ms\n"
			   "\tFirst update took: %.2f ms%s\n"
			   "\tFirst frame took: %.2f ms",
			   window->getTitle(), Runtime::modeName(), stats.windowCreated.asMilliseconds(),
			   stats.baseUI.asMilliseconds(), stats.completeUI.asMilliseconds(),
			   stats.firstUpdate.asMilliseconds(),
			   stats.updateCompleted ? "" : " (no UI update before presentation)",
			   ( now - stats.started ).asMilliseconds() );
}

void WindowInitialization::cancel( EE::Window::Window* window ) {
	if ( sPending.window == window )
		sPending = {};
}

} // namespace EE::Window
