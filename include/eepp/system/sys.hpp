#ifndef EE_SYSTEM_SYSTEM_HPP
#define EE_SYSTEM_SYSTEM_HPP

#include <eepp/system/time.hpp>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace EE { namespace System {

using ProcessID = Uint64;

class EE_API Sys {
  public:
	enum class PlatformType {
		Linux,
		Android,
		FreeBSD,
		OpenBSD,
		NetBSD,
		DragonFlyBSD,
		BSD,
		Emscripten,
		Haiku,
		iOS,
		macOS,
		Solaris,
		Windows,
		Unknown
	};

	/** @return The current process id */
	static ProcessID getProcessID();

	/** @return the current date time */
	static std::string getDateTimeStr();

	/** Converts any epoch timestamp to a formatted string. */
	static std::string epochToString( const Uint64& epochTimestamp,
									  const std::string& format = "%Y-%m-%d %H:%M" );

	/** @return A storage path for config files for every platform */
	static std::string getConfigPath( const std::string& appname );

	/** @return A storage path for cache files (data the application can recreate) for every
	 * platform, private to the user: $XDG_CACHE_HOME/appname (~/.cache/appname) on Linux and BSD,
	 * ~/Library/Caches/appname on macOS and iOS, %LOCALAPPDATA%\appname on Windows,
	 * ~/config/cache/appname on Haiku. Elsewhere, or without those variables, a directory in the
	 * temporary path. The directory is not created. */
	static std::string getCachePath( const std::string& appname );

	/** @return The user's data directory (appname inside it, when given), without a trailing
	 * slash: $XDG_DATA_HOME (~/.local/share) on Linux, BSD and Haiku, ~/Library/Application
	 * Support on macOS and iOS, %APPDATA% on Windows. Elsewhere, the config path. The directory
	 * is not created. */
	static std::string getDataPath( const std::string& appname = "" );

	/** @return The directories searched for shared data (icons, applications, MIME types), most
	 * important first: getDataPath(), then $XDG_DATA_DIRS (/usr/local/share:/usr/share) on Linux,
	 * BSD and Haiku. No trailing slashes. */
	static std::vector<std::string> getDataSearchPaths();

	/** @return The path of the directory designated for temporary files. */
	static std::string getTempPath();

	/** @return The number of milliseconds since the first call. */
	static Uint64 getTicks();

	/** Wait the time defined before returning. */
	static void sleep( const Time& time );

	/** @return The application path ( the executable path without the executable ) */
	static std::string getProcessPath();

	/** @return The application's resource base directory, with a trailing slash. Assets are
	 * relative to this directory. Resolves macOS .app Contents/Resources and Linux/BSD AppImage
	 * roots, including launchers that expose assets through the working directory. Ordinary
	 * executables use getProcessPath(). On Emscripten, appName selects an app-specific preload
	 * root when appName/assets exists, otherwise the shared assets layout is used. Emscripten
	 * paths are absolute so they remain valid after a working-directory change. Other platforms
	 * ignore appName. Does not change the working directory. */
	static std::string getResourcePath( std::string_view appName = {} );

	/** @return The process path ( the executable file path ) */
	static std::string getProcessFilePath();

	/** @return The Unix epoch time in milliseconds. */
	static Int64 getSystemTime();

	/** @return The Unix epoch time in seconds. */
	static Int64 getUnixTimestamp();

	/** @return The OS Name
	 *  @param showReleaseName Instead of returning only the OS Name, it will append the release
	 * name or number. For Windows instead of "Windows" it will be "Windows 7", for "Linux" it will
	 * be "Linux 3.15" and so on.
	 */
	static std::string getOSName( bool showReleaseName = false );

	/** @return The OS Architecture */
	static std::string getOSArchitecture();

	/** @return The platform name. */
	static std::string getPlatform();

	/** @return The platform type. */
	static Sys::PlatformType getPlatformType();

	/** @return The Number of CPUs of the system. */
	static int getCPUCount();

	/** Dynamically load a shared object and return a pointer to the object handle.
	**	@param sofile a system dependent name of the object file
	**	@return The pointer to the object handle or NULL if there was an error */
	static void* loadObject( const std::string& sofile );

	/** Loads an optional library without diagnostics when logErrors is false. */
	static void* loadObject( const std::string& sofile, bool logErrors );

	/** Unloads a shared object from memory.
	**	@param handle The object handle of the shared object to unload */
	static void unloadObject( void* handle );

	/** Looks up the address of the named function in the shared object and return it.
	**	@param handle A valid shared object handle returned by Sys::LoadObject()
	**	@param name The name of the function to look up
	**	@return The pointer to the function or NULL if there was an error */
	static void* loadFunction( void* handle, const std::string& name );

	static void* loadFunction( void* handle, const std::string& name, bool logErrors );

	/** @return the argument list in a vector of std::strings in UTF-8, ignoring the first argument
	 * (the binary name) */
	static std::vector<std::string> parseArguments( int argc, char* argv[] );

	/** @return The OS logical drives */
	static std::vector<std::string> getLogicalDrives();

	/** Finds the location of a binary / executable file.
	 *  @return The executable file path, or an empty string if not found. */
	static std::string which( const std::string& exeName,
							  const std::vector<std::string>& customSearchPaths = {} );

	/** @return An environment variable */
	static std::string getEnv( const std::string& name );

	/** Sets a null-terminated environment variable name and value for the current process.
	 * @return True if the environment was updated successfully.
	 */
	static bool setEnv( const char* name, const char* value );

	/** @return A split environment variable */
	static std::vector<std::string> getEnvSplit( const std::string& name );

	/** It will attach the console to the parent process console if any. Windows only function.
	 * Other platforms will do nothing.
	 */
	static bool windowAttachConsole();

	/** Executes a command */
	static int execute( const std::string& cmd, const std::string& workingDir = "" );

	/** @return True if current running platform / os is a mobile one */
	static bool isMobile();

	/** @return The process environment variables */
	static std::unordered_map<std::string, std::string> getEnvironmentVariables();

	/** @return The process ids found with the corresponding process / binary / executable name */
	static std::vector<ProcessID> pidof( const std::string& processName );

	/** @return A list of the current running processes */
	static std::vector<std::pair<ProcessID, std::string>> listProcesses();

	/** @returns The unix timestamp of the process creation time */
	static Int64 getProcessCreationTime( ProcessID pid );

	/** @returns The target destination of a windows shortcut path (.lnk files) */
	static std::string getShortcutTarget( const std::string& lnkFilePath );

	/** @returns The user home directory */
	static std::string getUserDirectory();

	/** @returns True if the process has any childrens */
	static bool processHasChildren( ProcessID pid );

	/** @returns True if the operating system is using a dark color scheme */
	static bool isOSUsingDarkColorScheme( bool allowUsingCached = true );
};

}} // namespace EE::System

#endif
