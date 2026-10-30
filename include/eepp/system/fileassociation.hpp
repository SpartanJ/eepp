#ifndef EE_SYSTEM_FILEASSOCIATION_HPP
#define EE_SYSTEM_FILEASSOCIATION_HPP

#include <eepp/config.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace EE::System {

struct Application;

struct EE_API FileAssociationApplication {
	std::string id;
	std::string name;
	std::string executablePath;
	std::string iconPath;
};

/** Registers desktop file handlers and changes per-user default applications.
 *
 * Extension registrations are per-user. On Windows this adds the application to the Open With list,
 * since current Windows versions do not allow applications to silently change the user's default
 * app. XDG desktops use a generated desktop entry and the shared MIME database. macOS uses Launch
 * Services and therefore treats a selected extension as a request to make the application its
 * editor.
 */
class EE_API FileAssociation {
  public:
	enum class DefaultResult : Uint8 {
		Failed,
		Applied,
		/** Windows opened Default Apps settings; the user must choose the app there. */
		UserActionRequired
	};

	explicit FileAssociation( FileAssociationApplication application );

	static bool isSupported();

	/** XDG platforms can additionally expose the generated desktop entry in application menus. */
	static bool supportsDesktopEntries();

	/** Makes an installed handler the per-user default for a file type. Call off the UI thread.
	 * @p extension is a filename suffix, with or without a leading dot; @p mimeType is the exact
	 * MIME type (not a parent type). XDG desktops require the MIME type and use the existing
	 * desktop id without rewriting the application's entry. macOS requires an extension and uses
	 * the selected application's bundle. Windows requires an extension and opens Default Apps
	 * settings: UserActionRequired does not mean that the association has changed.
	 * No document is opened. On failure, @p error describes the problem; otherwise it is cleared.
	 * Type associations may cover several extensions sharing a MIME type or macOS content type.
	 */
	static DefaultResult setDefaultApplication( const Application& application,
												std::string_view extension,
												const std::string& mimeType, std::string& error );

	/** Returns the extensions from @p supportedExtensions currently registered for this app. */
	std::vector<std::string>
	getRegisteredExtensions( const std::vector<std::string>& supportedExtensions ) const;

	/** Makes @p registeredExtensions the registered subset of @p supportedExtensions. */
	bool setRegisteredExtensions( const std::vector<std::string>& registeredExtensions,
								  const std::vector<std::string>& supportedExtensions );

	/** Makes @p registeredExtensions the registered subset of @p supportedExtensions and updates
	 * the desktop-entry visibility in the same operation. @p desktopEntryInstalled is ignored on
	 * platforms that do not support desktop entries. */
	bool setRegisteredExtensions( const std::vector<std::string>& registeredExtensions,
								  const std::vector<std::string>& supportedExtensions,
								  bool desktopEntryInstalled );

	bool isDesktopEntryInstalled() const;

	bool setDesktopEntryInstalled( bool installed );

	const std::string& getLastError() const { return mLastError; }

	/** Returns a lower-case extension without a leading dot, or an empty string if invalid. */
	static std::string normalizeExtension( std::string_view extension );

  private:
	FileAssociationApplication mApplication;
	mutable std::string mLastError;
};

} // namespace EE::System

#endif
