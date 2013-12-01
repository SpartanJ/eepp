#ifndef EE_SYSTEM_SYSTEM_HPP
#define EE_SYSTEM_SYSTEM_HPP

#include <eepp/system/base.hpp>
#include <eepp/system/ctime.hpp>

namespace EE { namespace System {

class EE_API Sys {
	public:
		/** Returns the current date time */
		static std::string GetDateTimeStr();

		/** @return A storage path for config files for every platform */
		static std::string GetConfigPath( std::string appname );

		/** @return The path of the directory designated for temporary files. */
		static std::string GetTempPath();

		/** @return The number of milliseconds since the first call. Note that this value wraps if the program runs for more than ~49 days. */
		static Uint32 GetTicks();

		/** Wait a specified number of milliseconds before returning. */
		static void Sleep( const Uint32& ms );

		/** Wait the time defined before returning. */
		static void Sleep( const cTime& time );

		/** @return The application path ( the executable path ) */
		static std::string GetProcessPath();

		/** @return The System Time */
		static eeDouble GetSystemTime();

		/** @return The OS Name */
		static std::string GetOSName();

		/** @return The OS Architecture */
		static std::string GetOSArchitecture();

		/** @return The Number of CPUs of the system. */
		static eeInt GetCPUCount();

		/** @return Returns free disk space for a given path in bytes */
		static Int64 GetDiskFreeSpace(const std::string& path);

		/** Dynamically load a shared object and return a pointer to the object handle.
		**	@param sofile a system dependent name of the object file
		**	@return The pointer to the object handle or NULL if there was an error */
		static void * LoadObject( const std::string& sofile );

		/** Unloads a shared object from memory.
		**	@param The object handle of the shared object to unload */
		static void UnloadObject( void * handle );

		/** Looks up the address of the named function in the shared object and return it.
		**	@param handle A valid shared object handle returned by Sys::LoadObject()
		**	@param name The name of the function to look up
		**	@return The pointer to the function or NULL if there was an error */
		static void * LoadFunction( void * handle, const std::string& name );
};

}}

#endif
