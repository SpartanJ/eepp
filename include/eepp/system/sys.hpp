#ifndef EE_SYSTEM_SYSTEM_HPP
#define EE_SYSTEM_SYSTEM_HPP

#include <eepp/system/time.hpp>
#include <string>
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

	/** @return The path of the directory designated for temporary files. */
	static std::string getTempPath();

	/** @return The number of milliseconds since the first call. Note that this value wraps if the
	 * program runs for more than ~49 days. */
	static Uint64 getTicks();

	/** Wait the time defined before returning. */
	static void sleep( const Time& time );

	/** @return The application path ( the executable path without the executable ) */
	static std::string getProcessPath();

	/** @return The process path ( the executable file path ) */
	static std::string getProcessFilePath();

	/** @return The System Time */
	static double getSystemTime();

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

	/** Unloads a shared object from memory.
	**	@param handle The object handle of the shared object to unload */
	static void unloadObject( void* handle );

	/** Looks up the address of the named function in the shared object and return it.
	**	@param handle A valid shared object handle returned by Sys::LoadObject()
	**	@param name The name of the function to look up
	**	@return The pointer to the function or NULL if there was an error */
	static void* loadFunction( void* handle, const std::string& name );

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
};

}} // namespace EE::System

#endif
