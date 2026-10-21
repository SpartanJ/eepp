#ifndef EE_UI_UISYSTEMICONTHEME_HPP
#define EE_UI_UISYSTEMICONTHEME_HPP

#include <eepp/core/executor.hpp>
#include <eepp/ui/uiicon.hpp>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace EE { namespace UI {

namespace Private {
class SystemIconBackend;
}

/** @brief Icons provided by the platform, loaded asynchronously.
 *
 * - Linux and BSD: the user's freedesktop.org icon theme (Icon Theme Specification).
 * - Windows: the shell's icons (file types, special folders and stock icons).
 * - macOS: NSWorkspace icons.
 *
 * Icons are returned immediately. Finding, reading and rasterizing them happens on the worker
 * executor; until an icon's image is ready it shows its fallback icon (or nothing). Textures are
 * created on the UI executor, which then calls the icons-loaded callback so views can redraw.
 * Loaded icons are cached for the life of the theme.
 *
 * Both executors must outlive the work they were given (shut the worker down first, as a
 * ThreadPool member declared after the UI executor does). Methods are UI thread only.
 */
class EE_API UISystemIconTheme : public std::enable_shared_from_this<UISystemIconTheme> {
  public:
	/** @param worker Where lookups, file reads and rasterization run (e.g. a ThreadPool).
	 * @param ui The thread that owns the graphics context (e.g. a MainThreadExecutor).
	 * @param themeName Freedesktop theme to use; empty detects the user's (Linux and BSD only).
	 * Detection and indexing start right away on the worker. */
	static std::shared_ptr<UISystemIconTheme> New( Executor& worker, Executor& ui,
												   const std::string& themeName = {} );

	~UISystemIconTheme();

	/** Icon for standard (freedesktop) icon names, tried in order: "folder", "user-home",
	 * "folder-documents", "user-trash", "drive-harddisk", "drive-removable-media",
	 * "network-server", "text-x-generic"... Windows and macOS map these names to their own
	 * special folder and stock icons. Linux and BSD also accept absolute icon file paths.
	 * The returned icon is owned by the theme. */
	UIIcon* getIcon( const std::vector<std::string>& names, UIIcon* fallback = nullptr );

	/** Icon for a file or folder type. On Linux and BSD names are the freedesktop names of its
	 * MIME type (most specific first); Windows and macOS use the type of the file name's
	 * extension. */
	UIIcon* getFileIcon( const std::string& fileName, bool isDirectory,
						 const std::vector<std::string>& names, UIIcon* fallback = nullptr );

	/** Icon of a local path as the platform shows it (special folders, drives): Windows and macOS
	 * ask the shell for that path; Linux and BSD use names. */
	UIIcon* getPathIcon( const std::string& path, const std::vector<std::string>& names,
						 UIIcon* fallback = nullptr );

	/** Windows icon resource location (quoted path and resource index, or an indirect resource).
	 * Lookup and image decoding run on the worker, like other system icons. */
	UIIcon* getIconResource( const std::string& location, UIIcon* fallback = nullptr );

	/** The freedesktop theme in use, once detected (empty before, and on Windows and macOS). */
	const std::string& getThemeName() const;

	/** Called on the UI thread after icons received images (coalesced): redraw the views that
	 * show them. */
	void setOnIconsLoaded( std::function<void()> callback );

	/** A request for an icon image. Public for the platform backends. */
	struct Request {
		enum class Kind : Uint8 { Names, File, Path, Resource };
		std::vector<std::string> names;
		/** File name (File), path (Path), or native icon resource location (Resource). */
		std::string value;
		Kind kind{ Kind::Names };
		bool isDirectory{ false };
	};

  protected:
	class Icon;

	UISystemIconTheme( Executor& worker, Executor& ui, const std::string& themeName );

	UIIcon* findIcon( const std::string& key ) const;

	UIIcon* icon( std::string key, Request request, UIIcon* fallback );

	void load( Icon* icon, int size );

	void onLoaded();

	Executor& mWorker;
	Executor& mUI;
	std::shared_ptr<Private::SystemIconBackend> mBackend;
	std::string mThemeName;
	std::unordered_map<std::string, UIIconPtr> mIcons;
	std::function<void()> mOnIconsLoaded;
	bool mNotifyPending{ false };
};

}} // namespace EE::UI

#endif
