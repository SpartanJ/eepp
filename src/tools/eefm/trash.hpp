#ifndef EEFM_TRASH_HPP
#define EEFM_TRASH_HPP

#include <eepp/core/uri.hpp>
#include <eepp/system/translator.hpp>
#include <optional>
#include <string>
#include <vector>

namespace eefm {

/** @brief The desktop trash (recycle bin) for local files.
 *
 * Linux/BSD follow the freedesktop.org Trash specification (the home trash, or a
 * $topdir/.Trash-$uid directory for files on other file systems), macOS moves to ~/.Trash and
 * Windows uses the Recycle Bin. Everything but supported(), location() and contains() blocks:
 * run it off the UI thread.
 */
class Trash {
  public:
	/** @return True when uri can be moved to the trash (a local file outside the trash). */
	static bool supported( const EE::URI& uri );

	/** @return The browsable trash folder (where trashed files live), if the platform has one. */
	static std::optional<EE::URI> location();

	/** @return True when uri is inside a browsable trash folder. */
	static bool contains( const EE::URI& uri );

	/** Moves a local file or directory to the trash. @return An error message, empty on success.
	 */
	static std::string moveToTrash( const std::string& path,
									const EE::System::Translator& translator );

	/** Moves every path to the trash. @return One "name: error" line per failure. */
	static std::string moveToTrash( const std::vector<std::string>& paths,
									const EE::System::Translator& translator );

	/** Moves a trashed entry back to its original location (spec trashes only). */
	static std::string restore( const std::string& trashedPath,
								const EE::System::Translator& translator );

	/** Restores every path. @return One "name: error" line per failure. */
	static std::string restore( const std::vector<std::string>& trashedPaths,
								const EE::System::Translator& translator );

	/** @return The original location of a trashed entry, when recorded. */
	static std::optional<std::string> originalPath( const std::string& trashedPath );

	/** Drops what the trash recorded about an entry that was deleted from it (its .trashinfo). */
	static void forget( const std::string& trashedPath );

	/** Permanently removes everything in the home trash. */
	static std::string empty( const EE::System::Translator& translator );
};

} // namespace eefm

#endif
