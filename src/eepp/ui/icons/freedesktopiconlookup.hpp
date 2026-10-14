#ifndef EE_UI_ICONS_FREEDESKTOPICONLOOKUP_HPP
#define EE_UI_ICONS_FREEDESKTOPICONLOOKUP_HPP

// Private: the Linux and BSD backend of UISystemIconTheme. Exported only for the unit tests.

#include <eepp/config.hpp>

// Linux, BSD and other freedesktop systems; macOS and Windows use their shells' icons.
#if defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS &&  \
	EE_PLATFORM != EE_PLATFORM_MACOS
#define EE_SYSTEM_ICONS_FREEDESKTOP
#endif

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace EE { namespace UI { namespace Private {

/** @brief Icon lookup in a freedesktop.org icon theme (Icon Theme Specification).
 *
 * Resolves icon names ("folder", "text-x-generic") to files of a theme, its parent themes
 * (Inherits) and hicolor, then the unthemed pixmaps directory. Construction reads the themes'
 * index.theme files and lists their icon directories, so it blocks: create it on a worker thread.
 * find() is thread-safe.
 */
class EE_API FreedesktopIconLookup {
  public:
	/** Base directories searched for themes, in priority order: $HOME/.icons,
	 * $XDG_DATA_HOME/icons, $XDG_DATA_DIRS/icons and /usr/share/pixmaps (unthemed icons). */
	static std::vector<std::string> defaultSearchPaths();

	/** The icon theme configured by the user's desktop (KDE, GNOME, Cinnamon, MATE, Xfce, or the
	 * GTK settings files), empty if unknown. Blocking: it may run the desktop's settings tool. */
	static std::string detectThemeName();

	/** @param themeName Theme to use; when empty or not installed, Breeze or Adwaita are tried
	 * before falling back to hicolor alone. */
	explicit FreedesktopIconLookup( const std::string& themeName,
									std::vector<std::string> searchPaths = defaultSearchPaths() );

	~FreedesktopIconLookup();

	/** False when no theme (not even hicolor) was found. */
	bool isValid() const;

	/** The theme in use (the requested one, or the fallback picked). */
	const std::string& getThemeName() const;

	/** Path of the best file for names (most specific first): every name is tried in the theme,
	 * then in its parents, then in hicolor (theme first, as the specification and GTK do, so the
	 * user's theme wins over a more specific hicolor icon), then unthemed. The closest size wins;
	 * scalable icons match any size in their range. Empty if none is found. */
	std::string find( const std::vector<std::string>& names, int size, int scale = 1 ) const;

  private:
	struct Theme;

	void addTheme( const std::string& name, std::unordered_set<std::string>& seen );

	std::vector<std::string> mSearchPaths;
	std::string mThemeName;
	/** The theme, its parents (depth first) and hicolor. */
	std::vector<std::unique_ptr<Theme>> mThemes;
	/** Unthemed icons (pixmaps directories): name -> path. */
	std::unordered_map<std::string, std::string> mUnthemed;
};

}}} // namespace EE::UI::Private

#endif
