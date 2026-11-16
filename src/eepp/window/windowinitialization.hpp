#ifndef EE_WINDOW_WINDOWINITIALIZATION_HPP
#define EE_WINDOW_WINDOWINITIALIZATION_HPP

#include <eepp/system/monotonicclock.hpp>

using namespace EE::System;

namespace EE::Window {

class Window;

/** Internal, UI-thread-only startup measurements for UIApplication's primary window.
 * UIApplication owns the engine lifecycle; one static record follows its primary window. */
class WindowInitialization {
  public:
	static bool isPending() { return sPending.window != nullptr; }

	static void begin( Window* window, MonotonicClock::TimePoint started );

	static void baseUIReady( Window* window );

	static void beginUpdate( Window* window );

	static void endUpdate( Window* window );

	static void presented( Window* window );

	static void cancel( Window* window );

  private:
	struct PendingWindow {
		Window* window{ nullptr };
		MonotonicClock::TimePoint started;
		Time windowCreated;
		Time baseUI;
		Time completeUI;
		Time firstUpdate;
		bool updateStarted{ false };
		bool updateCompleted{ false };
	};

	static PendingWindow sPending;
};

} // namespace EE::Window

#endif
