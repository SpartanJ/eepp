#ifndef EEFM_MIME_HPP
#define EEFM_MIME_HPP

#include <eepp/vfs/fileinfo.hpp>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace eefm {

/** @brief Name-based MIME types, descriptions and icons.
 *
 * Uses the freedesktop.org shared-mime-info database where it exists (globs2, subclasses and the
 * per-type XML comments) and a small built-in table elsewhere. Detection is by name only: file
 * contents are never read, so it works the same for local and remote files. Thread-safe.
 */
class MimeDatabase {
  public:
	static MimeDatabase& instance();

	/** MIME type of an entry ("inode/directory" for directories). The tables are immutable once
	 * loaded, so the reference stays valid. */
	const std::string& typeOf( const EE::VFS::FileInfo& info );

	/** MIME type of a file name (not a path). */
	const std::string& typeForName( const std::string& name );

	/** Human readable description ("Markdown document"). */
	std::string description( const std::string& mime, const std::string& name = {} );

	/** Description of an entry, cached by type: cheap enough for sorting. */
	std::string describe( const EE::VFS::FileInfo& info );

	/** Name of eefm's glyph icon for an entry (the fallback icon set). */
	std::string iconName( const EE::VFS::FileInfo& info );

	/** Freedesktop icon names for a type, most specific first: the shared-mime-info icon, the
	 * type's own name ("text-x-python"), its parents', its generic icon and the media's generic
	 * icon ("text-x-generic"). Folders are "folder". */
	std::vector<std::string> iconNames( const std::string& mime );

	/** iconNames() of an entry, cached by type (never evicted: the types are a closed set). */
	const std::vector<std::string>& iconNames( const EE::VFS::FileInfo& info );

	/** The type and its parents (text/markdown -> text/plain), most specific first. */
	std::vector<std::string> withParents( const std::string& mime );

  private:
	MimeDatabase();

	void load();

	struct Glob {
		int weight{ 0 };
		std::string mime;
	};

	std::mutex mMutex;
	bool mLoaded{ false };
	bool mHasDatabase{ false };
	std::vector<std::string> mDataDirs;
	/** Lowercase extension (without the dot, may contain dots: "tar.gz") -> type. */
	std::unordered_map<std::string, Glob> mExtensions;
	/** Exact file names ("Makefile"). */
	std::unordered_map<std::string, Glob> mNames;
	/** Other glob patterns, checked last. */
	std::vector<std::pair<std::string, Glob>> mPatterns;
	std::unordered_map<std::string, std::vector<std::string>> mParents;
	/** Type -> description; empty when the type has none (not looked up again). */
	std::unordered_map<std::string, std::string> mDescriptions;
	/** shared-mime-info icons and generic-icons: type -> icon name. */
	std::unordered_map<std::string, std::string> mIcons;
	std::unordered_map<std::string, std::string> mGenericIcons;
	/** iconNames() by type. Node based: references to the values stay valid. */
	std::unordered_map<std::string, std::vector<std::string>> mIconNamesByType;
};

} // namespace eefm

#endif
