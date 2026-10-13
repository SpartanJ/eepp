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

	/** MIME type of an entry ("inode/directory" for directories). */
	std::string typeOf( const EE::VFS::FileInfo& info );

	std::string typeForName( const std::string& name );

	/** Human readable description ("Markdown document"). */
	std::string description( const std::string& mime, const std::string& name = {} );

	/** Description of an entry, cached by extension: cheap enough for sorting. */
	std::string describe( const EE::VFS::FileInfo& info );

	/** Name of the theme icon for an entry. */
	std::string iconName( const EE::VFS::FileInfo& info );

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
	std::unordered_map<std::string, std::string> mDescriptions;
	std::unordered_map<std::string, std::string> mDescriptionByExtension;
};

} // namespace eefm

#endif
