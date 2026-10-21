#ifndef EE_SYSTEM_APPLICATIONS_HPP
#define EE_SYSTEM_APPLICATIONS_HPP

#include <eepp/config.hpp>
#include <eepp/core/uri.hpp>
#include <optional>
#include <string>
#include <vector>

namespace EE { namespace System {

/** An installed file handler: an XDG desktop entry, macOS bundle, or Windows Shell handler. */
struct EE_API Application {
	/** Desktop file id, macOS bundle identifier/path, or opaque Windows handler identifier. */
	std::string id;
	std::string name;
	/** XDG Exec line, macOS bundle path, or Windows association extension. */
	std::string exec;
	/** XDG icon name/path or Windows icon resource location; macOS uses exec's bundle path. */
	std::string icon;
	bool terminal{ false };
};

/** Installed application discovery and launching through the desktop's native associations.
 *
 * Discovery and launching may block: call off the UI thread. Results own their strings and may
 * cross threads; native handler objects stay on the thread that queries or invokes them.
 * Icon locations describe images for asynchronous loading; this service never decodes images.
 */
class EE_API Applications {
  public:
	/** Registered handlers, default first. URI supplies a local path or a remote/archive filename;
	 * no files are downloaded. MIME types are supplied by the caller, most specific first with
	 * optional parent types. Linux/BSD use them to query desktop entries; native platforms use
	 * the filename/file URL and may fall back to the first MIME type. */
	static std::vector<Application> forFile( const URI& uri,
											 const std::vector<std::string>& mimeTypes = {} );

	static std::optional<Application> defaultFor( const URI& uri,
												  const std::vector<std::string>& mimeTypes = {} );

	/** Opens local files using the selected registered handler. */
	static bool launch( const Application& app, const std::vector<std::string>& paths );

	/** Starts a command line ("gimp" or "gimp --new-instance"), the files appended. */
	static bool launchCommand( const std::string& command, const std::vector<std::string>& paths );

	/** Starts argv detached (no zombie, no shared session). On Windows, console gives the process
	 * its own console, in the user's default terminal. Argument boundaries are preserved. */
	static bool spawn( const std::vector<std::string>& argv, const std::string& workingDir = {},
					   bool console = false );

	/** Splits a desktop entry Exec value into arguments (quoting rules of the specification). */
	static std::vector<std::string> splitExec( const std::string& exec );

  private:
	Applications() = delete;

	struct Registry;

	static Registry& registry();
};

}} // namespace EE::System

#endif
