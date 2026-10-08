#ifndef EPROC_PROCESS_ICON_RESOLVER_HPP
#define EPROC_PROCESS_ICON_RESOLVER_HPP

#include <eepp/core/containers.hpp>
#include <mutex>
#include <string>

using namespace EE;

namespace eproc {

/** Resolves the icon of a process to an absolute icon file path following the XDG desktop entry
 *  route: an executable name is mapped to a .desktop entry, and that entry's icon name to a file
 *  inside the installed icon themes. This resolver does not need a display connection; eproc's
 *  UI thread may use an X11 window icon when no desktop icon was found on X11.
 *
 *  The desktop index is built once, lazily, on the first iconFor() call. Every result - including
 *  the "no icon" ones - is cached under the queried exe/name pair, so the steady state is a couple
 *  of hash lookups with no filesystem access at all.
 *
 *  The resolver never throws and never fails hard: unreadable directories, missing environment
 *  variables and malformed desktop files are skipped, and an unavailable icon simply resolves to
 *  the empty string. All methods are safe to call from any thread. */
class ProcessIconResolver {
  public:
	/** Resolves the icon file for a process.
	 *  @param exePath absolute executable path, when available (may be empty)
	 *  @param name    process name (comm), used as a fallback key
	 *  @return absolute path to a PNG/SVG/XPM icon, or empty string when none found.
	 *          The returned reference stays valid for the resolver's lifetime. */
	const std::string& iconFor( const std::string& exePath, const std::string& name );

  private:
	/** Reads every desktop entry once, keying the executable name, the window class and the desktop
	 *  file id to the entry's icon name. Only ever called once, from iconFor(). */
	void buildIndex();

	/** Scans @p dir for *.desktop files. Missing or unreadable directories are skipped silently. */
	void scanDirectory( const std::string& dir );

	/** Indexes a single .desktop file. Silently ignores unreadable, hidden and icon-less entries.
	 */
	void indexDesktopFile( const std::string& path );

	/** Returns the icon name registered for @p key, or an empty string when there is none. The
	 *  executable/window-class index wins over the weaker desktop id index. */
	const std::string& lookupKey( const std::string& key ) const;

	/** Resolves an icon name to an absolute file path, caching the result (misses included). */
	const std::string& resolveIconName( const std::string& iconName );

	bool mIndexBuilt{ false };

	// Icon name per executable basename and per startup window class.
	UnorderedMap<std::string, std::string> mExecIndex;
	// Icon name per desktop file id (the file name without ".desktop"), used as a weak fallback.
	UnorderedMap<std::string, std::string> mIdIndex;

	// Icon name -> icon file path. Misses are stored as empty strings so they are not retried.
	UnorderedMap<std::string, std::string> mIconPathIndex;

	// "<exePath>|<name>" -> icon file path, negative results included.
	UnorderedMap<std::string, std::string> mResultCache;

	// Returned when nothing matched; a member so the returned reference always stays valid.
	std::string mEmpty;

	std::mutex mMutex;
};

} // namespace eproc

#endif // EPROC_PROCESS_ICON_RESOLVER_HPP
