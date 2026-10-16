#ifndef EEFM_APPS_HPP
#define EEFM_APPS_HPP

#include <eepp/config.hpp>
#include <eepp/core/uri.hpp>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace eefm {

/** An installed application that can open files (an XDG desktop entry on Linux/BSD). */
struct Application {
	/** Desktop file id ("org.kde.kate.desktop"). */
	std::string id;
	std::string name;
	/** Exec line with field codes (%f, %F, %u, %U...). */
	std::string exec;
	bool terminal{ false };
};

/** @brief Applications registered for MIME types ("Open with") and process launching. */
class Applications {
  public:
	/** What running a file means. */
	enum class ExecutableKind : EE::Uint8 {
		/** Not runnable: it is opened with an application. */
		None,
		/** A native executable (ELF / Mach-O) the user may execute. */
		Program,
		/** An executable text file with an interpreter line ("#!"). */
		Script
	};

	static Applications& instance();

	/** Applications for a MIME type (and its parent types), the default one first. */
	std::vector<Application> forMime( const std::string& mime );

	std::optional<Application> defaultFor( const std::string& mime );

	/** Starts app with the given local files; the process is detached. */
	static bool launch( const Application& app, const std::vector<std::string>& paths );

	/** Starts a command line ("gimp" or "gimp --new-instance"), the files appended. */
	static bool launchCommand( const std::string& command, const std::vector<std::string>& paths );

	/** Opens a terminal emulator in directory. */
	static bool openTerminal( const std::string& directory );

	/** Starts argv detached from eefm (no zombie, no shared session). On Windows, console
	 * gives the process its own console (shells), in the user's default terminal. */
	static bool spawn( const std::vector<std::string>& argv, const std::string& workingDir = {},
					   bool console = false );

	/** Whether the local file at path can be run, from its permissions and its first bytes (the
	 * execute permission alone is not enough: FAT and NTFS mounts give it to every file). Reads
	 * the file: call it off the UI thread. Always None on Windows, where opening a program runs
	 * it. */
	static ExecutableKind executableKind( const std::string& path );

	/** Splits a desktop entry Exec value into arguments (quoting rules of the spec). */
	static std::vector<std::string> splitExec( const std::string& exec );

  private:
	void load();

	std::mutex mMutex;
	bool mLoaded{ false };
	std::unordered_map<std::string, Application> mApps;
	/** MIME type -> desktop ids, in discovery order. */
	std::unordered_map<std::string, std::vector<std::string>> mByMime;
	/** MIME type -> preferred desktop ids (mimeapps.list defaults). */
	std::unordered_map<std::string, std::vector<std::string>> mDefaults;
	std::unordered_map<std::string, std::vector<std::string>> mRemoved;
};

} // namespace eefm

#endif
