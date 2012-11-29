#ifndef EE_SYSTEM_SYSTEM_HPP
#define EE_SYSTEM_SYSTEM_HPP

#include <eepp/system/base.hpp>

namespace EE { namespace System {

class EE_API Sys {
	public:
		/** Returns the current date time */
		static std::string GetDateTimeStr();

		/** @return A storage path for config files for every platform */
		static std::string StoragePath( std::string appname );

		/** @return The number of milliseconds since the EE++ library initialization. Note that this value wraps if the program runs for more than ~49 days. */
		static Uint32 GetTicks();

		/** Wait a specified number of milliseconds before returning. */
		static void Sleep( const Uint32& ms );

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

		/** @return Returns free disk space for a given path */
		static Int64 GetDiskFreeSpace(const std::string& path);
};

}}

#endif
