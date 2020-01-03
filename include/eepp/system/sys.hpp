#ifndef EE_SYSTEM_SYSTEM_HPP
#define EE_SYSTEM_SYSTEM_HPP

#include <string>
#include <eepp/system/time.hpp>

namespace EE { namespace System {

class EE_API Sys {
	public:
		/** Returns the current date time */
		static std::string getDateTimeStr();

		/** @return A storage path for config files for every platform */
		static std::string getConfigPath( std::string appname );

		/** @return The path of the directory designated for temporary files. */
		static std::string getTempPath();

		/** @return The number of milliseconds since the first call. Note that this value wraps if the program runs for more than ~49 days. */
		static Uint32 getTicks();

		/** Wait a specified number of milliseconds before returning. */
		static void sleep( const Uint32& ms );

		/** Wait the time defined before returning. */
		static void sleep( const Time& time );

		/** @return The application path ( the executable path ) */
		static std::string getProcessPath();

		/** @return The System Time */
		static double getSystemTime();

		/** @return The OS Name
		 *  @param showReleaseName Instead of returning only the OS Name, it will append the release name or number. For Windows instead of "Windows" it will be "Windows 7", for "Linux" it will be "Linux 3.15" and so on.
		*/
		static std::string getOSName( bool showReleaseName = false );

		/** @return The OS Architecture */
		static std::string getOSArchitecture();

		/** @return The platform name. */
		static std::string getPlatform();

		/** @return The Number of CPUs of the system. */
		static int getCPUCount();

		/** Dynamically load a shared object and return a pointer to the object handle.
		**	@param sofile a system dependent name of the object file
		**	@return The pointer to the object handle or NULL if there was an error */
		static void * loadObject( const std::string& sofile );

		/** Unloads a shared object from memory.
		**	@param handle The object handle of the shared object to unload */
		static void unloadObject( void * handle );

		/** Looks up the address of the named function in the shared object and return it.
		**	@param handle A valid shared object handle returned by Sys::LoadObject()
		**	@param name The name of the function to look up
		**	@return The pointer to the function or NULL if there was an error */
		static void * loadFunction( void * handle, const std::string& name );
};

}}

#endif
