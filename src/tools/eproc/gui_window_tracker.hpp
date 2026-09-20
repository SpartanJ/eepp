#ifndef EPROC_GUI_WINDOW_TRACKER_HPP
#define EPROC_GUI_WINDOW_TRACKER_HPP

#include <eepp/config.hpp>
#include <eepp/core/containers.hpp>

using namespace EE;

namespace eproc {

/** @brief Tracks which processes own a top-level GUI window on the current display.
 *
 *  This answers the "does this process have a GUI window?" question used by the process list
 *  filters (the equivalent of ksysguard's hasGUIWindow()). The X11 backend reads the
 *  _NET_CLIENT_LIST property of the root window and the owner of each window from its _NET_WM_PID
 *  property. Both are EWMH extensions published by the running window manager, so on a display
 *  without a window manager, or without an X display at all, the tracker simply reports that no
 *  process owns a window. X11 is loaded at runtime when available, keeping it optional for
 *  Wayland and headless systems. Where no backend is built, the tracker compiles to a stub that
 *  reports itself unavailable and lists no window PIDs.
 *
 *  The Xlib display is held as an opaque pointer, keeping this header free of X11 headers. */
class GuiWindowTracker {
  public:
	GuiWindowTracker();
	~GuiWindowTracker();
	GuiWindowTracker( const GuiWindowTracker& ) = delete;
	GuiWindowTracker& operator=( const GuiWindowTracker& ) = delete;

	/** Re-reads the window list and the PIDs owning windows. MAIN THREAD ONLY. */
	void refresh();

	/** True when the last refresh() saw a top-level window owned by @p pid. */
	bool hasWindowForPid( long pid ) const;

	/** PIDs that owned a top-level window at the last refresh(). */
	const UnorderedSet<long>& windowPids() const { return mPids; }

	/** True when the backend connected to a display and can list the windows it owns. */
	bool isAvailable() const;

  private:
	UnorderedSet<long> mPids;

	// State of the X11 backend. Builds without X11 headers carry no state no backend could fill and
	// are still fully usable: they report no window for any process.
#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )
	struct X11Api;
	X11Api* mX11{ nullptr };
	void* mDisplay{ nullptr }; // Display*
	unsigned long mRootWindow{ 0 };
#endif
};

} // namespace eproc

#endif // EPROC_GUI_WINDOW_TRACKER_HPP
