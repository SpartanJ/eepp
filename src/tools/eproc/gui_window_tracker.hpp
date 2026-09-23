#ifndef EPROC_GUI_WINDOW_TRACKER_HPP
#define EPROC_GUI_WINDOW_TRACKER_HPP

#include <eepp/config.hpp>
#include <eepp/core/containers.hpp>
#include <eepp/graphics/drawable.hpp>
#include <utility>
#include <vector>

using namespace EE;
using namespace EE::Graphics;

namespace eproc {

/** @brief Tracks which processes own a top-level GUI window on the current display.
 *
 *  This answers the "does this process have a GUI window?" question used by the process list
 *  filters (the equivalent of ksysguard's hasGUIWindow()). The X11 backend reads the
 *  _NET_CLIENT_LIST property of the root window and the owner of each window from its _NET_WM_PID
 *  property. It also finds XEmbed windows (including tray icons) in the X11 window hierarchy.
 *  Without an X display, the tracker reports no window PIDs. X11 is loaded at runtime when
 *  available, keeping it optional for Wayland and headless systems. Where no backend is built,
 *  the tracker compiles to a stub that reports itself unavailable and lists no window PIDs.
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

	/** True when the last refresh() saw a managed or embedded GUI window owned by @p pid. */
	bool hasWindowForPid( long pid ) const;

	/** PIDs that owned a managed or embedded GUI window at the last refresh(). */
	const UnorderedSet<long>& windowPids() const { return mPids; }

	/** Returns a cached X11 window icon when this PID owns a window. MAIN THREAD ONLY. */
	DrawablePtr iconForPid( long pid );

	/** True when the backend connected to a display and can list the windows it owns. */
	bool isAvailable() const;

  private:
	UnorderedSet<long> mPids;

	// State of the X11 backend. Builds without X11 headers carry no state no backend could fill and
	// are still fully usable: they report no window for any process.
#if EE_PLATFORM == EE_PLATFORM_LINUX && defined( EE_X11_PLATFORM )
	struct CachedWindowIcon {
		DrawablePtr icon;
		Uint64 lastSeenPass{ 0 };
		Uint64 lastAttemptPass{ 0 };
		bool attempted{ false };
	};
	std::vector<std::pair<long, unsigned long>> mWindows;
	std::vector<std::pair<long, unsigned long>> mTrayWindows;
	UnorderedMap<unsigned long, CachedWindowIcon> mIconCache;
	Uint64 mRefreshPass{ 0 };
	unsigned mIconReadsThisPass{ 0 };
	struct X11Api;
	X11Api* mX11{ nullptr };
	void* mDisplay{ nullptr }; // Display*
	unsigned long mRootWindow{ 0 };
	unsigned long mIconAtom{ 0 };
	unsigned long mXEmbedAtom{ 0 };
	void recordWindow( long pid, unsigned long window );
	long xembedWindowPid( unsigned long window, unsigned long pidAtom );
	void scanXEmbedWindows( unsigned long pidAtom );
	DrawablePtr readIcon( unsigned long window );
#endif
};

} // namespace eproc

#endif // EPROC_GUI_WINDOW_TRACKER_HPP
